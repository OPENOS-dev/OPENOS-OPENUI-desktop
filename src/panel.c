/*
 * Copyright (C) 2026 OPENOS-dev
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the OPENOS-PROJECT-LICENSE (OPL) v1.2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OPL for more details.
 *
 * You should have received a copy of the OPL along with this program.
 * If not, see <https://github.com/OPENOS-dev/OPL>.
 */

#define _POSIX_C_SOURCE 200809L
/* OPENOS 面板 — 任务栏 + 工作区切换器 + 启动器 (Wayland 客户端)
 * - 任务栏: 经 wlr-foreign-toplevel-management 协议列出全部窗口,
 *   左键激活 / 右键关闭 (OPENUI 令牌绘制)
 * - 工作区切换器: 经自研 openos-workspace 协议展示 4 个工作区胶囊, 点击切换
 * - Menu 启动器: layer-shell 弹出应用列表 (NUI2 文字符号, 禁 Emoji)
 * 依赖: wayland-client, wlr-layer-shell / wlr-foreign-toplevel /
 *       openos-workspace 协议(生成), cairo
 * 在 OPENOS(Linux) 上随合成器运行。
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <poll.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/input-event-codes.h>
#include <cairo/cairo.h>
#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-client-protocol.h"
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
#include "openos-workspace-unstable-v1-client-protocol.h"
#include "openui.h"

/* ---- 布局常量 (OPENUI) ---- */
#define PANEL_H   OUI_PANEL_HEIGHT          /* 32 */
#define MENU_W    200
#define MENU_H    120
#define WS_CAP_W  16   /* 工作区胶囊 */
#define WS_CAP_H  4
#define WS_GAP    4
#define TB_H      24   /* 任务栏按钮 */
#define TB_GAP    4
#define TB_PAD    10   /* 按钮文本左右留白 */
#define TB_MIN_W  48
#define TB_MAX_W  160

struct app { const char *label; const char *cmd; };
static struct app apps[] = {
    {"Terminal",  "openos-term"},
    {"Files",     "openos-term -e mc"},
    {"Editor",    "openos-term -e nano"},
    {"Settings",  "openos-term -e openos-settings"},
    {NULL, NULL}
};

static struct wl_display *display;
static struct wl_compositor *compositor;
static struct wl_shm *shm;
static struct zwlr_layer_shell_v1 *layer_shell;
static struct zwlr_foreign_toplevel_manager_v1 *ft_manager;
static struct openos_workspace_manager_v1 *ws_manager;
static struct wl_seat *seat;
static struct wl_pointer *pointer;

/* ---- 窗口列表 (任务栏) ---- */
struct ft_win {
    struct wl_list link;
    struct zwlr_foreign_toplevel_handle_v1 *handle;
    char *title;
    char *app_id;
    bool active;
    bool removed;   /* 已收到 closed, 待清理 */
    int bx, bw;     /* 任务栏按钮几何 (绘制后) */
};
static struct wl_list ft_wins;   /* ft_win.link */

/* ---- 工作区列表 ---- */
struct ws_item {
    struct wl_list link;
    struct openos_workspace_handle_v1 *handle;
    char *name;
    bool active;
};
static struct wl_list ws_items;  /* ws_item.link */

struct panel {
    struct wl_surface *surface;
    struct zwlr_layer_surface_v1 *layer;
    int width, height;
    cairo_surface_t *cairo_surf;
    struct wl_buffer *buffer;
    void *shm_data;
    int shm_fd;
    size_t shm_size;
    bool is_menu;
};

static struct panel panel, menu;
static bool menu_open = false;
static bool dirty = true;
static double last_px = -1, last_py = -1;
static double g_brand_w = 0;   /* 品牌标识宽度缓存 (命中测试用) */

/* ---------- SHM 缓冲 ---------- */
static int set_buffer(struct panel *p, int w, int h) {
    if (p->buffer) wl_buffer_destroy(p->buffer);
    if (p->shm_fd >= 0) close(p->shm_fd);
    p->width = w; p->height = h;
    p->shm_size = (size_t)w * h * 4;
    p->shm_fd = memfd_create("openos-panel", 0);
    ftruncate(p->shm_fd, (off_t)p->shm_size);
    p->shm_data = mmap(NULL, p->shm_size, PROT_READ | PROT_WRITE,
                       MAP_SHARED, p->shm_fd, 0);
    struct wl_shm_pool *pool = wl_shm_create_pool(shm, p->shm_fd, (int)p->shm_size);
    p->buffer = wl_shm_pool_create_buffer(pool, 0, w, h, w * 4,
                                          WL_SHM_FORMAT_ARGB8888);
    wl_shm_pool_destroy(pool);
    cairo_surface_destroy(p->cairo_surf);
    p->cairo_surf = cairo_image_surface_create_for_data(p->shm_data,
        CAIRO_FORMAT_ARGB32, w, h, w * 4);
    return 0;
}

/* ---------- cairo 工具 (OPENUI) ---------- */
static void set_rgb(cairo_t *cr, uint32_t hex, double alpha) {
    cairo_set_source_rgba(cr, ((hex >> 16) & 255) / 255.0,
        ((hex >> 8) & 255) / 255.0, (hex & 255) / 255.0, alpha);
}
static void rounded_rect(cairo_t *cr, double x, double y, double w, double h,
                         double r) {
    double rr = r > h / 2 ? h / 2 : r;
    cairo_new_path(cr);
    cairo_arc(cr, x + rr, y + rr, rr, M_PI, 1.5 * M_PI);
    cairo_arc(cr, x + w - rr, y + rr, rr, 1.5 * M_PI, 2 * M_PI);
    cairo_arc(cr, x + w - rr, y + h - rr, rr, 0, 0.5 * M_PI);
    cairo_arc(cr, x + rr, y + h - rr, rr, 0.5 * M_PI, M_PI);
    cairo_close_path(cr);
}
static void draw_text(cairo_t *cr, const char *s, double x, double y,
                      int size, int weight, uint32_t color) {
    cairo_select_font_face(cr, OUI_FONT_SANS, CAIRO_FONT_SLANT_NORMAL,
        weight >= 500 ? CAIRO_FONT_WEIGHT_BOLD : CAIRO_FONT_WEIGHT_NORMAL);
    cairo_set_font_size(cr, size);
    set_rgb(cr, color, 1.0);
    cairo_move_to(cr, x, y);
    cairo_show_text(cr, s);
}
/* 截断文本到最大像素宽, 返回实际文本 (静态缓冲) */
static const char *fit_text(cairo_t *cr, const char *s, int max_w) {
    static char buf[128];
    cairo_text_extents_t ex;
    cairo_text_extents(cr, s, &ex);
    if (ex.width <= max_w) return s;
    int len = (int)strlen(s);
    for (; len > 0; len--) {
        if (len >= 3) {
            memcpy(buf, s, (size_t)len);
            memcpy(buf + len, "...", 4);   /* 含 NUL */
        } else {
            strcpy(buf, "...");
        }
        cairo_text_extents(cr, buf, &ex);
        if (ex.width <= max_w) return buf;
    }
    return "...";
}

/* ---------- 任务栏布局: 计算每个窗口按钮几何 ---------- */
static double layout_taskbar(cairo_t *cr, double x0, double x1) {
    struct ft_win *w;
    double x = x0;
    wl_list_for_each(w, &ft_wins, link) {
        if (w->removed) continue;
        const char *label = w->title ? w->title : (w->app_id ? w->app_id : "?");
        cairo_select_font_face(cr, OUI_FONT_SANS, CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, OUI_TYPE_LABEL_L.size);
        cairo_text_extents_t ex;
        cairo_text_extents(cr, label, &ex);
        int bw = (int)ex.width + TB_PAD * 2;
        if (bw < TB_MIN_W) bw = TB_MIN_W;
        if (bw > TB_MAX_W) bw = TB_MAX_W;
        if (x + bw > x1) bw = (int)(x1 - x);   /* 截断到可用区 */
        if (bw < TB_MIN_W) bw = TB_MIN_W;
        w->bx = (int)x; w->bw = bw;
        x += bw + TB_GAP;
    }
    return x;
}

/* ---------- 绘制面板 (任务栏 + 工作区 + 时钟 + Menu) ---------- */
static void draw_panel(struct panel *p) {
    cairo_t *cr = cairo_create(p->cairo_surf);
    /* 背景: surface-1 (半透明毛玻璃底, 由合成器模糊) */
    set_rgb(cr, OUI_SURFACE, 0.88);
    cairo_paint(cr);

    double x = OUI_SP_2;
    /* 品牌标识 */
    draw_text(cr, "OPENOS DEV2026.1", x, 21, 12, 400, OUI_ON_SURFACE);
    cairo_text_extents_t ex;
    cairo_text_extents(cr, "OPENOS DEV2026.1", &ex);
    g_brand_w = ex.width;
    x += ex.width + OUI_SP_3;

    /* 工作区指示器 */
    int ws_x0 = (int)x;
    double cy = PANEL_H / 2.0;
    struct ws_item *ws;
    wl_list_for_each(ws, &ws_items, link) {
        uint32_t col = ws->active ? OUI_PRIMARY : OUI_OUTLINE_VARIANT;
        rounded_rect(cr, x, cy - WS_CAP_H / 2.0, WS_CAP_W, WS_CAP_H,
                     OUI_SHAPE_FULL);
        set_rgb(cr, col, 1.0);
        cairo_fill(cr);
        x += WS_CAP_W + WS_GAP;
    }
    x += OUI_SP_2;

    /* 任务栏窗口列表 */
    double tb_x1 = p->width - 150.0;
    layout_taskbar(cr, x, tb_x1);
    struct ft_win *w;
    wl_list_for_each(w, &ft_wins, link) {
        if (w->removed || w->bw <= 0) continue;
        double by = (PANEL_H - TB_H) / 2.0;
        rounded_rect(cr, w->bx, by, w->bw, TB_H, OUI_SHAPE_XS);
        if (w->active) {
            set_rgb(cr, OUI_PRIMARY, 0.18);
            cairo_fill(cr);
        } else {
            set_rgb(cr, OUI_SURFACE_BRIGHT, 0.35);
            cairo_fill(cr);
        }
        const char *label = w->title ? w->title : (w->app_id ? w->app_id : "?");
        cairo_select_font_face(cr, OUI_FONT_SANS, CAIRO_FONT_SLANT_NORMAL,
                               CAIRO_FONT_WEIGHT_NORMAL);
        cairo_set_font_size(cr, OUI_TYPE_LABEL_L.size);
        const char *fit = fit_text(cr, label, w->bw - TB_PAD);
        cairo_text_extents(cr, fit, &ex);
        uint32_t tcol = w->active ? OUI_PRIMARY : OUI_ON_SURFACE_VARIANT;
        draw_text(cr, fit, w->bx + (w->bw - ex.width) / 2.0,
                  by + TB_H / 2.0 + 5, OUI_TYPE_LABEL_L.size,
                  w->active ? 500 : 400, tcol);
    }

    /* 时钟 */
    time_t t = time(NULL);
    char clk[16];
    strftime(clk, sizeof clk, "%H:%M:%S", localtime(&t));
    cairo_text_extents(cr, clk, &ex);
    draw_text(cr, clk, p->width - 150 + 10, 21, 12, 400, OUI_ON_SURFACE_VARIANT);

    /* Menu 按钮 (强调色, 文字符号) */
    rounded_rect(cr, p->width - 84, 4, 78, PANEL_H - 8, OUI_SHAPE_XS);
    set_rgb(cr, OUI_PRIMARY, 1.0);
    cairo_fill(cr);
    draw_text(cr, "\xe2\x98\xb0 Menu", p->width - 76, 21, 12, 500, OUI_ON_PRIMARY);

    cairo_destroy(cr);
    wl_surface_attach(p->surface, p->buffer, 0, 0);
    wl_surface_damage(p->surface, 0, 0, p->width, p->height);
    wl_surface_commit(p->surface);
    (void)ws_x0;
}

/* ---------- 绘制菜单 ---------- */
static void draw_menu(struct panel *p) {
    cairo_t *cr = cairo_create(p->cairo_surf);
    rounded_rect(cr, 0, 0, p->width, p->height, OUI_SHAPE_SM);
    set_rgb(cr, OUI_SURFACE_6, 0.9);
    cairo_fill(cr);
    for (int i = 0; apps[i].label; i++) {
        draw_text(cr, apps[i].label, OUI_SP_2, 22 + i * 26, 13, 400,
                  OUI_ON_SURFACE);
    }
    cairo_destroy(cr);
    wl_surface_attach(p->surface, p->buffer, 0, 0);
    wl_surface_damage(p->surface, 0, 0, p->width, p->height);
    wl_surface_commit(p->surface);
}

/* ---------- layer 配置 ---------- */
static void panel_configure(void *data, struct zwlr_layer_surface_v1 *s,
                            uint32_t serial, uint32_t w, uint32_t h) {
    struct panel *p = data;
    (void)s;
    if (w == 0 || h == 0) {
        w = p->width > 0 ? p->width : 1280;
        h = p->height > 0 ? p->height : (p->is_menu ? MENU_H : PANEL_H);
    }
    if (w != (uint32_t)p->width || h != (uint32_t)p->height) {
        set_buffer(p, w, h);
        dirty = true;
    }
    zwlr_layer_surface_v1_ack_configure(s, serial);
}
static void panel_closed(void *data, struct zwlr_layer_surface_v1 *s) {
    (void)data; (void)s;
}
static const struct zwlr_layer_surface_v1_listener layer_listener = {
    .configure = panel_configure,
    .closed = panel_closed,
};

/* ---------- foreign-toplevel (任务栏数据) ---------- */
static void ft_on_title(void *data, struct zwlr_foreign_toplevel_handle_v1 *h,
                        const char *title) {
    struct ft_win *w = data;
    free(w->title);
    w->title = strdup(title);
    dirty = true;
}
static void ft_on_app_id(void *data, struct zwlr_foreign_toplevel_handle_v1 *h,
                         const char *app_id) {
    struct ft_win *w = data;
    free(w->app_id);
    w->app_id = strdup(app_id);
    dirty = true;
}
static void ft_on_state(void *data, struct zwlr_foreign_toplevel_handle_v1 *h,
                        struct wl_array *state) {
    struct ft_win *w = data;
    (void)h;
    w->active = false;
    uint32_t *s;
    wl_array_for_each(s, state)
        if (*s == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED)
            w->active = true;
    dirty = true;
}
static void ft_on_done(void *data, struct zwlr_foreign_toplevel_handle_v1 *h) {
    (void)data; (void)h;
    dirty = true;
}
static void ft_on_closed(void *data, struct zwlr_foreign_toplevel_handle_v1 *h) {
    struct ft_win *w = data;
    (void)h;
    w->removed = true;
    dirty = true;
}
static void ft_on_parent(void *data, struct zwlr_foreign_toplevel_handle_v1 *h,
                         struct zwlr_foreign_toplevel_handle_v1 *parent) {
    (void)data; (void)h; (void)parent;
}

static const struct zwlr_foreign_toplevel_handle_v1_listener ft_handle_listener = {
    .title = ft_on_title,
    .app_id = ft_on_app_id,
    .output_enter = NULL,
    .output_leave = NULL,
    .state = ft_on_state,
    .done = ft_on_done,
    .closed = ft_on_closed,
    .parent = ft_on_parent,
};

static void ft_on_toplevel(void *data, struct zwlr_foreign_toplevel_manager_v1 *m,
                           struct zwlr_foreign_toplevel_handle_v1 *h) {
    (void)data; (void)m;
    struct ft_win *w = calloc(1, sizeof *w);
    if (!w) return;
    w->handle = h;
    wl_list_insert(&ft_wins, &w->link);
    zwlr_foreign_toplevel_handle_v1_add_listener(h, &ft_handle_listener, w);
    dirty = true;
}
static void ft_on_finished(void *data, struct zwlr_foreign_toplevel_manager_v1 *m) {
    (void)data; (void)m;
}
static const struct zwlr_foreign_toplevel_manager_v1_listener ft_manager_listener = {
    .toplevel = ft_on_toplevel,
    .finished = ft_on_finished,
};

/* ---------- openos-workspace (工作区切换器) ---------- */
static void ws_on_name(void *data, struct openos_workspace_handle_v1 *h,
                       const char *name) {
    struct ws_item *ws = data;
    (void)h;
    free(ws->name);
    ws->name = strdup(name);
    dirty = true;
}
static void ws_on_state(void *data, struct openos_workspace_handle_v1 *h,
                        struct wl_array *state) {
    struct ws_item *ws = data;
    (void)h;
    ws->active = false;
    uint32_t *s;
    wl_array_for_each(s, state)
        if (*s == OPENOS_WORKSPACE_HANDLE_V1_STATE_ACTIVE)
            ws->active = true;
    dirty = true;
}
static void ws_on_done(void *data, struct openos_workspace_handle_v1 *h) {
    (void)data; (void)h;
    dirty = true;
}
static const struct openos_workspace_handle_v1_listener ws_handle_listener = {
    .name = ws_on_name,
    .state = ws_on_state,
    .done = ws_on_done,
};
static void ws_on_workspace(void *data, struct openos_workspace_manager_v1 *m,
                            struct openos_workspace_handle_v1 *h) {
    (void)data; (void)m;
    struct ws_item *ws = calloc(1, sizeof *ws);
    if (!ws) return;
    ws->handle = h;
    wl_list_insert(&ws_items, &ws->link);
    openos_workspace_handle_v1_add_listener(h, &ws_handle_listener, ws);
    dirty = true;
}
static void ws_on_finished(void *data, struct openos_workspace_manager_v1 *m) {
    (void)data; (void)m;
}
static const struct openos_workspace_manager_v1_listener ws_manager_listener = {
    .workspace = ws_on_workspace,
    .finished = ws_on_finished,
};

/* ---------- 交互 ---------- */
static void spawn_app(const char *cmd) {
    if (fork() == 0) {
        execl("/bin/sh", "sh", "-c", cmd, (char *)NULL);
        _exit(127);
    }
}

static void pointer_motion(void *data, struct wl_pointer *p, uint32_t t,
                           wl_fixed_t sx, wl_fixed_t sy) {
    (void)data; (void)p; (void)t;
    last_px = wl_fixed_to_double(sx);
    last_py = wl_fixed_to_double(sy);
}

static void panel_button(struct panel *p, double px, double py, uint32_t button,
                         uint32_t state) {
    if (state != WL_POINTER_BUTTON_STATE_PRESSED) return;
    /* Menu 按钮区 */
    if (px >= p->width - 84 && px < p->width - 6) {
        menu_open = !menu_open;
        if (menu_open) {
            zwlr_layer_surface_v1_set_size(menu.layer, MENU_W, MENU_H);
            wl_surface_commit(menu.surface);
        } else {
            wl_surface_attach(menu.surface, NULL, 0, 0);
            wl_surface_commit(menu.surface);
        }
        return;
    }
    /* 工作区胶囊 */
    double x = OUI_SP_2 + g_brand_w + OUI_SP_3;
    struct ws_item *ws;
    wl_list_for_each(ws, &ws_items, link) {
        if (px >= x && px < x + WS_CAP_W && py >= (PANEL_H - WS_CAP_H) / 2.0 &&
            py < (PANEL_H + WS_CAP_H) / 2.0) {
            openos_workspace_handle_v1_activate(ws->handle);
            return;
        }
        x += WS_CAP_W + WS_GAP;
    }
    /* 任务栏窗口按钮: 左键激活, 右键关闭 */
    struct ft_win *w;
    wl_list_for_each(w, &ft_wins, link) {
        if (w->removed || w->bw <= 0) continue;
        double by = (PANEL_H - TB_H) / 2.0;
        if (px >= w->bx && px < w->bx + w->bw && py >= by && py < by + TB_H) {
            if (button == BTN_RIGHT) {
                if (ft_manager && w->handle)
                    zwlr_foreign_toplevel_manager_v1_close(ft_manager, w->handle);
            } else if (button == BTN_LEFT) {
                if (ft_manager && w->handle && seat)
                    zwlr_foreign_toplevel_manager_v1_activate(ft_manager,
                        w->handle, seat);
            }
            return;
        }
    }
}

static void pointer_button(void *data, struct wl_pointer *p, uint32_t serial,
                           uint32_t time, struct wl_surface *surface,
                           struct wl_surface *r, uint32_t button, uint32_t state) {
    (void)data; (void)p; (void)serial; (void)time; (void)r;
    if (last_px < 0 || last_py < 0) return;
    if (surface == panel.surface)
        panel_button(&panel, last_px, last_py, button, state);
    else if (surface == menu.surface && button == BTN_LEFT &&
             state == WL_POINTER_BUTTON_STATE_PRESSED) {
        int napps = (int)(sizeof apps / sizeof apps[0]) - 1;   /* 去掉 NULL 尾 */
        int idx = (int)((last_py - 10) / 26);
        if (idx >= 0 && idx < napps) spawn_app(apps[idx].cmd);
        menu_open = false;
        wl_surface_attach(menu.surface, NULL, 0, 0);
        wl_surface_commit(menu.surface);
    }
}

static const struct wl_pointer_listener pointer_listener = {
    .enter = NULL, .leave = NULL, .motion = pointer_motion,
    .button = pointer_button, .axis = NULL,
};

static void seat_capabilities(void *data, struct wl_seat *s, uint32_t caps) {
    (void)data;
    if ((caps & WL_SEAT_CAPABILITY_POINTER) && !pointer) {
        pointer = wl_seat_get_pointer(s);
        wl_pointer_add_listener(pointer, &pointer_listener, NULL);
    }
}
static const struct wl_seat_listener seat_listener = {
    .capabilities = seat_capabilities,
    .name = NULL,
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
    else if (strcmp(iface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0)
        ft_manager = wl_registry_bind(reg, id,
            &zwlr_foreign_toplevel_manager_v1_interface, 3);
    else if (strcmp(iface, openos_workspace_manager_v1_interface.name) == 0)
        ws_manager = wl_registry_bind(reg, id,
            &openos_workspace_manager_v1_interface, 1);
    else if (strcmp(iface, wl_seat_interface.name) == 0) {
        seat = wl_registry_bind(reg, id, &wl_seat_interface, 1);
        wl_seat_add_listener(seat, &seat_listener, NULL);
    }
}
static void registry_remove(void *data, struct wl_registry *reg, uint32_t id) {
    (void)data; (void)reg; (void)id;
}
static const struct wl_registry_listener registry_listener = {
    .global = registry_global, .global_remove = registry_remove,
};

static void make_panel(struct panel *p, bool is_menu) {
    p->is_menu = is_menu;
    p->shm_fd = -1;
    p->surface = wl_compositor_create_surface(compositor);
    p->layer = zwlr_layer_shell_v1_get_layer_surface(layer_shell, p->surface,
        NULL, ZWLR_LAYER_SHELL_V1_LAYER_TOP, is_menu ? "openos-menu" : "openos-panel");
    zwlr_layer_surface_v1_add_listener(p->layer, &layer_listener, p);
    if (is_menu) {
        zwlr_layer_surface_v1_set_anchor(p->layer,
            ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
        zwlr_layer_surface_v1_set_exclusive_zone(p->layer, 0);
    } else {
        zwlr_layer_surface_v1_set_anchor(p->layer,
            ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP | ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT |
            ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
        zwlr_layer_surface_v1_set_exclusive_zone(p->layer, PANEL_H);
    }
    p->width = 0; p->height = is_menu ? MENU_H : PANEL_H;
    zwlr_layer_surface_v1_set_size(p->layer, 0, p->height);
    wl_surface_commit(p->surface);
}

static void cleanup_closed_wins(void) {
    struct ft_win *w, *tmp;
    wl_list_for_each_safe(w, tmp, &ft_wins, link) {
        if (w->removed) {
            zwlr_foreign_toplevel_handle_v1_destroy(w->handle);
            wl_list_remove(&w->link);
            free(w->title);
            free(w->app_id);
            free(w);
            dirty = true;
        }
    }
}

int main(void) {
    wl_list_init(&ft_wins);
    wl_list_init(&ws_items);

    display = wl_display_connect(NULL);
    if (!display) {
        fprintf(stderr, "openos-panel: 无法连接 Wayland 合成器\n");
        return 1;
    }
    struct wl_registry *reg = wl_display_get_registry(display);
    wl_registry_add_listener(reg, &registry_listener, NULL);
    wl_display_roundtrip(display);
    if (!compositor || !shm || !layer_shell) {
        fprintf(stderr, "openos-panel: 缺少必要协议\n");
        return 1;
    }
    if (ft_manager)
        zwlr_foreign_toplevel_manager_v1_add_listener(ft_manager,
            &ft_manager_listener, NULL);
    if (ws_manager)
        openos_workspace_manager_v1_add_listener(ws_manager,
            &ws_manager_listener, NULL);
    wl_display_roundtrip(display);

    make_panel(&panel, false);
    make_panel(&menu, true);

    wl_display_flush(display);

    int fd = wl_display_get_fd(display);
    struct pollfd fds = { fd, POLLIN, 0 };
    while (1) {
        while (wl_display_prepare_read(display) != 0)
            wl_display_dispatch_pending(display);
        wl_display_flush(display);
        poll(&fds, 1, 1000);
        if (fds.revents & POLLIN) {
            wl_display_read_events(display);
            wl_display_dispatch_pending(display);
        } else {
            wl_display_cancel_read(display);
        }
        cleanup_closed_wins();
        dirty = true;   /* 每秒刷新时钟 */
        if (dirty && panel.cairo_surf) {
            dirty = false;
            if (!menu_open) draw_panel(&panel);
        }
    }
    return 0;
}
