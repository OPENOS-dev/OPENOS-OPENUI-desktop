#define _POSIX_C_SOURCE 200809L
/* OPENOS 通知守护进程 — 桌面右上角通知层 (Wayland, layer-shell overlay)
 * - 通过 FIFO /tmp/openos-notifyd.fifo 接收通知, 每行 "标题|正文"
 * - NUI2 风格: 深色卡片 surface-6 + 半透明(毛玻璃底由合成器模糊) + accent 竖条 + 文字符号
 * - 自动过期 (6s), 点击卡片可立即消除
 * 在 OPENOS(Linux) 上随合成器运行; 依赖 wayland-client, wlr-layer-shell, cairo。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cairo/cairo.h>
#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "nui2.h"

#define NOTIF_FIFO   "/tmp/openos-notifyd.fifo"
#define MAX_NOTIF    4
#define NOTIF_LIFETIME 6   /* 秒 */
#define CARD_W       320
#define CARD_H       68
#define CARD_GAP     6
#define TOP_MARGIN   8

struct notif {
    char title[128];
    char body[256];
    time_t added;
};

static struct notif notifs[MAX_NOTIF];
static int notif_count = 0;

static struct wl_display *display;
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct zwlr_layer_shell_v1 *layer_shell;

static struct wl_surface *surface;
static struct zwlr_layer_surface_v1 *layer;
static cairo_surface_t *cairo_surf;
static struct wl_buffer *buffer;
static void *shm_data;
static int shm_fd = -1;
static size_t shm_size;
static int width = CARD_W;
static int height = TOP_MARGIN + MAX_NOTIF * (CARD_H + CARD_GAP);
static double last_px = -1, last_py = -1;
static bool dirty = true;

/* ---------- SHM 缓冲 ---------- */
static int set_buffer(int w, int h) {
    if (buffer) wl_buffer_destroy(buffer);
    if (shm_fd >= 0) close(shm_fd);
    width = w; height = h;
    shm_size = (size_t)w * h * 4;
    shm_fd = memfd_create("openos-notifyd", 0);
    ftruncate(shm_fd, (off_t)shm_size);
    shm_data = mmap(NULL, shm_size, PROT_READ | PROT_WRITE,
                    MAP_SHARED, shm_fd, 0);
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, shm_fd, (int)shm_size);
    buffer = wl_shm_pool_create_buffer(pool, 0, w, h, w * 4,
                                       WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    cairo_surface_destroy(cairo_surf);
    cairo_surf = cairo_image_surface_create_for_data(shm_data,
        CAIRO_FORMAT_ARGB32, w, h, w * 4);
    return 0;
}

/* ---------- cairo 小工具 ---------- */
static void set_rgb(cairo_t *cr, uint32_t hex, double alpha) {
    cairo_set_source_rgba(cr, ((hex >> 16) & 255) / 255.0,
        ((hex >> 8) & 255) / 255.0, (hex & 255) / 255.0, alpha);
}
static void rounded_rect(cairo_t *cr, double x, double y, double w, double h,
                        double r) {
    cairo_new_path(cr);
    cairo_arc(cr, x + r, y + r, r, M_PI, 1.5 * M_PI);
    cairo_arc(cr, x + w - r, y + r, r, 1.5 * M_PI, 2 * M_PI);
    cairo_arc(cr, x + w - r, y + h - r, r, 0, 0.5 * M_PI);
    cairo_arc(cr, x + r, y + h - r, r, 0.5 * M_PI, M_PI);
    cairo_close_path(cr);
}

/* ---------- 绘制 ---------- */
static void draw(void) {
    if (!cairo_surf) return;   /* 尚未收到 configure, 缓冲未就绪 */
    cairo_t *cr = cairo_create(cairo_surf);
    cairo_set_source_rgba(cr, 0, 0, 0, 0);  /* 全透明底 */
    cairo_set_operator(cr, CAIRO_OPERATOR_SOURCE);
    cairo_paint(cr);
    cairo_set_operator(cr, CAIRO_OPERATOR_OVER);

    cairo_select_font_face(cr, "Sans", CAIRO_FONT_SLANT_NORMAL,
                           CAIRO_FONT_WEIGHT_NORMAL);
    for (int i = 0; i < notif_count; i++) {
        double cy = TOP_MARGIN + i * (CARD_H + CARD_GAP);
        /* 卡片背景: surface-6 半透明 (下方由合成器模糊) */
        rounded_rect(cr, 4, cy, CARD_W - 8, CARD_H, NUI_RADIUS_MD);
        set_rgb(cr, NUI_SURFACE_6, 0.88);
        cairo_fill(cr);
        /* accent 竖条 */
        cairo_rectangle(cr, 4, cy + 6, 3, CARD_H - 12);
        set_rgb(cr, NUI_ACCENT, 1.0);
        cairo_fill(cr);
        /* 标题 (白色粗体) */
        set_rgb(cr, NUI_TEXT_PRIMARY, 1.0);
        cairo_set_font_size(cr, 13);
        cairo_move_to(cr, 16, cy + 20);
        cairo_show_text(cr, notifs[i].title);
        /* 正文 (次级色) */
        set_rgb(cr, NUI_TEXT_SECONDARY, 1.0);
        cairo_set_font_size(cr, 12);
        cairo_move_to(cr, 16, cy + 40);
        cairo_show_text(cr, notifs[i].body);
    }
    cairo_destroy(cr);

    wl_surface_attach(surface, buffer, 0, 0);
    wl_surface_damage(surface, 0, 0, width, height);
    wl_surface_commit(surface);
}

/* ---------- 通知队列 ---------- */
static void push_notif(const char *title, const char *body) {
    if (notif_count >= MAX_NOTIF) {
        /* 丢掉最旧的 */
        memmove(notifs, notifs + 1,
                (MAX_NOTIF - 1) * sizeof(struct notif));
        notif_count = MAX_NOTIF - 1;
    }
    struct notif *n = &notifs[notif_count++];
    memset(n, 0, sizeof *n);
    snprintf(n->title, sizeof n->title, "%s", title);
    snprintf(n->body, sizeof n->body, "%s", body);
    n->added = time(NULL);
    dirty = true;
}

static void expire_old(void) {
    time_t now = time(NULL);
    bool changed = false;
    int i = 0;
    while (i < notif_count) {
        if (now - notifs[i].added >= NOTIF_LIFETIME) {
            memmove(notifs + i, notifs + i + 1,
                    (size_t)(notif_count - i - 1) * sizeof(struct notif));
            notif_count--;
            changed = true;
        } else {
            i++;
        }
    }
    if (changed) dirty = true;
}

static void read_fifo(void) {
    static char line[512];
    static size_t keep = 0;
    char buf[512];
    ssize_t n = read(0, buf, sizeof buf - 1);
    if (n <= 0) return;
    for (ssize_t i = 0; i < n; i++) {
        if (buf[i] == '\n' || keep >= sizeof line - 1) {
            line[keep] = '\0';
            if (keep > 0) {
                char *sep = strchr(line, '|');
                if (sep) {
                    *sep = '\0';
                    push_notif(line, sep + 1);
                } else {
                    push_notif("OPENOS", line);
                }
            }
            keep = 0;
        } else {
            line[keep++] = buf[i];
        }
    }
}

/* ---------- layer 配置 ---------- */
static void layer_configure(void *data, struct zwlr_layer_surface_v1 *s,
                            uint32_t serial, uint32_t w, uint32_t h) {
    (void)data;
    if (w != (uint32_t)width || h != (uint32_t)height) {
        set_buffer(w, h);
        dirty = true;
    }
    zwlr_layer_surface_v1_ack_configure(s, serial);
}
static void layer_closed(void *data, struct zwlr_layer_surface_v1 *s) {
    (void)data; (void)s;
}
static const struct zwlr_layer_surface_v1_listener layer_listener = {
    .configure = layer_configure,
    .closed = layer_closed,
};

/* ---------- 指针 (点击消除通知) ---------- */
static void pointer_motion(void *data, struct wl_pointer *p, uint32_t t,
                           wl_fixed_t sx, wl_fixed_t sy) {
    (void)data; (void)p; (void)t;
    last_px = wl_fixed_to_double(sx);
    last_py = wl_fixed_to_double(sy);
}
static void pointer_button(void *data, struct wl_pointer *p, uint32_t serial,
                           uint32_t time, struct wl_surface *surf,
                           struct wl_surface *r, uint32_t button,
                           uint32_t state) {
    (void)data; (void)p; (void)serial; (void)time; (void)r; (void)button;
    if (state != WL_POINTER_BUTTON_STATE_PRESSED) return;
    if (surf != surface) return;
    if (last_py < 0) return;
    int idx = (int)((last_py - TOP_MARGIN) / (CARD_H + CARD_GAP));
    if (idx >= 0 && idx < notif_count) {
        memmove(notifs + idx, notifs + idx + 1,
                (size_t)(notif_count - idx - 1) * sizeof(struct notif));
        notif_count--;
        dirty = true;
    }
}
static const struct wl_pointer_listener pointer_listener = {
    .enter = NULL, .leave = NULL, .motion = pointer_motion,
    .button = pointer_button, .axis = NULL,
};

/* ---------- registry ---------- */
static void registry_global(void *data, struct wl_registry *reg, uint32_t id,
                            const char *iface, uint32_t ver) {
    (void)data;
    if (strcmp(iface, wl_compositor_interface.name) == 0)
        compositor = wl_registry_bind(reg, id, &wl_compositor_interface, 1);
    else if (strcmp(iface, wl_shm_interface.name) == 0)
        shm = wl_registry_bind(reg, id, &wl_shm_interface, 1);
    else if (strcmp(iface, zwlr_layer_shell_v1_interface.name) == 0)
        layer_shell = wl_registry_bind(reg, id, &zwlr_layer_shell_v1_interface, 1);
    else if (strcmp(iface, wl_seat_interface.name) == 0) {
        struct wl_seat *seat = wl_registry_bind(reg, id, &wl_seat_interface, 1);
        struct wl_pointer *ptr = wl_seat_get_pointer(seat);
        if (ptr)
            wl_pointer_add_listener(ptr, &pointer_listener, NULL);
    }
}
static void registry_remove(void *data, struct wl_registry *reg, uint32_t id) {
    (void)data; (void)reg; (void)id;
}
static const struct wl_registry_listener registry_listener = {
    .global = registry_global, .global_remove = registry_remove,
};

int main(void) {
    display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "openos-notifyd: 无法连接 Wayland 合成器\n");
        return 1;
    }
    struct wl_registry *reg = wl_display_get_registry(display);
    wl_registry_add_listener(reg, &registry_listener, NULL);
    wl_display_roundtrip(display);
    if (!compositor || !shm || !layer_shell) {
        fprintf(stderr, "openos-notifyd: 缺少必要协议\n");
        return 1;
    }

    /* FIFO */
    mkfifo(NOTIF_FIFO, 0644);
    int fifo = open(NOTIF_FIFO, O_RDONLY | O_NONBLOCK);
    if (fifo >= 0) dup2(fifo, 0);  /* read_fifo 从 stdin 读 */

    /* overlay 层: 右上角 */
    surface = wl_compositor_create_surface(compositor);
    layer = zwlr_layer_shell_v1_get_layer_surface(layer_shell, surface, NULL,
        ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "openos-notifyd");
    zwlr_layer_surface_v1_add_listener(layer, &layer_listener, NULL);
    zwlr_layer_surface_v1_set_anchor(layer,
        ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    zwlr_layer_surface_v1_set_exclusive_zone(layer, 0);
    zwlr_layer_surface_v1_set_size(layer, width, height);
    wl_surface_commit(surface);

    push_notif("OPENOS", "通知中心已就绪 · DEV2026.1");
    wl_display_flush(display);

    /* 事件循环 */
    int fd = wl_display_get_fd(display);
    struct pollfd fds[2] = {
        { fd, POLLIN, 0 },
        { 0, POLLIN, 0 },   /* FIFO stdin */
    };
    while (1) {
        while (wl_display_prepare_read(display) != 0)
            wl_display_dispatch_pending(display);
        wl_display_flush(display);
        poll(fds, 2, 500);
        if (fds[0].revents & POLLIN) {
            wl_display_read_events(display);
            wl_display_dispatch_pending(display);
        } else {
            wl_display_cancel_read(display);
        }
        if (fds[1].revents & POLLIN)
            read_fifo();
        if (fds[1].revents & (POLLHUP | POLLERR)) {
            /* FIFO 写入端断开: 重开继续等待 */
            close(0);
            int nf = open(NOTIF_FIFO, O_RDONLY | O_NONBLOCK);
            if (nf >= 0) dup2(nf, 0);
        }
        expire_old();
        if (dirty) {
            dirty = false;
            draw();
        }
    }
    return 0;
}
