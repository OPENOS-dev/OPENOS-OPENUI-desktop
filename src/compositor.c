#define _POSIX_C_SOURCE 200809L
/* OPENOS 桌面环境 — Wayland 合成器 (wlroots)
 * 提供: 窗口管理(xdg-shell) + 桌面层(layer-shell, 供面板/启动器) + NUI2 主题
 * 目标: wlroots >= 0.17, 在 OPENOS(Linux) 上构建与运行 (macOS 无法编译运行)。
 */
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wlr/backend.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/types/wlr_data_device.h>
#include <wlr/types/wlr_input_device.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wlr/types/wlr_foreign_toplevel_management.h>
#include <wlr/util/log.h>
#include "nui2.h"
#include "blur.h"
#include "workspace.h"

#define WORKSPACE_COUNT 4
/* 属于"模糊层"的 layer-shell namespace: 其下方桌面内容会被高斯模糊 (NUI2 毛玻璃) */
static bool is_blur_namespace(const char *ns) {
    return ns &&
        (strcmp(ns, "openos-panel") == 0 ||
         strcmp(ns, "openos-menu") == 0 ||
         strcmp(ns, "openos-notifyd") == 0);
}

struct openos_server {
    struct wl_display *display;
    struct wlr_backend *backend;
    struct wlr_renderer *renderer;
    struct wlr_allocator *allocator;
    struct wlr_scene *scene;
    struct wlr_scene_output_layout *scene_layout;
    struct wlr_output_layout *output_layout;
    struct wlr_xdg_shell *xdg_shell;
    struct wlr_layer_shell_v1 *layer_shell;
    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *xcursor_mgr;
    struct wlr_seat *seat;
    struct wl_list views;   /* openos_view.link */
    struct wl_list outputs; /* openos_output.link */

    /* 多工作区: 每个工作区一棵场景子树, 切换时整树开关 */
    struct wlr_scene_tree *workspaces[WORKSPACE_COUNT];
    int current_workspace;
    /* 模糊层: 面板/菜单/通知 等被动态模糊的内容挂在此树下 */
    struct wlr_scene_tree *blur_tree;

    /* 窗口列表 (任务栏) + 工作区 (切换器) 协议 */
    struct wlr_foreign_toplevel_manager_v1 *foreign_toplevel;
    struct openos_workspace_manager *workspace_mgr;
    struct wl_listener ft_request_activate;
    struct wl_listener ft_request_close;
    struct wl_listener ft_request_maximize;

    struct wlr_surface *grabbed_surface;
    double grab_x, grab_y;
    int grab_geobox_x, grab_geobox_y;
    uint32_t resize_edges;

    struct wl_listener new_output;
    struct wl_listener new_input;
    struct wl_listener new_xdg_toplevel;
    struct wl_listener new_layer_surface;
    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_abs;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;
    struct wl_listener request_cursor;
    struct wl_listener keyboard_key;
    struct wl_listener keyboard_destroy;
};

struct openos_output {
    struct wl_list link;
    struct openos_server *server;
    struct wlr_output *wlr_output;
    struct wlr_scene_output *scene_output;
    struct wl_listener frame;
};

struct openos_view {
    struct wl_list link;
    struct openos_server *server;
    struct wlr_xdg_toplevel *xdg_toplevel;
    struct wlr_scene_tree *scene_tree;
    struct wlr_scene_rect *border;
    struct wlr_scene_surface *scene_surface;
    bool mapped;
    int workspace;   /* 所属工作区索引 */
    struct wlr_foreign_toplevel_handle_v1 *ft_handle;

    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener destroy;
    struct wl_listener request_move;
    struct wl_listener request_resize;
    struct wl_listener request_maximize;
    struct wl_listener request_fullscreen;
};

struct openos_layer {
    struct openos_server *server;
    struct wlr_layer_surface_v1 *layer_surface;
    struct wlr_scene_layer_surface_v1 *scene_layer;
    struct wl_listener destroy;
    struct wl_listener map;
    struct wl_listener unmap;
    struct wl_listener configure;
};

/* 0xRRGGBB -> 归一化 float[4] (用于 wlr_scene 颜色) */
static void nui_colorf(uint32_t hex, float out[4]) {
    out[0] = ((hex >> 16) & 0xFF) / 255.0f;
    out[1] = ((hex >> 8) & 0xFF) / 255.0f;
    out[2] = (hex & 0xFF) / 255.0f;
    out[3] = 1.0f;
}

static void focus_view(struct openos_view *view, struct wlr_surface *surface) {
    if (view == NULL) return;
    struct openos_server *s = view->server;
    struct wlr_seat *seat = s->seat;
    struct wlr_surface *prev = seat->keyboard_state.focused_surface;
    if (prev == surface) return;

    if (prev) wlr_seat_keyboard_notify_clear_focus(seat);

    /* 重置所有视图边框为中性色, 当前视图设为强调色;
     * 同步任务栏 (foreign-toplevel) 激活状态 */
    struct openos_view *v;
    wl_list_for_each(v, &s->views, link) {
        float dim[4]; nui_colorf(NUI_SURFACE_4, dim);
        wlr_scene_rect_set_color(v->border, dim);
        if (v->ft_handle)
            wlr_foreign_toplevel_handle_v1_set_activated(v->ft_handle, false);
    }
    float acc[4]; nui_colorf(NUI_ACCENT, acc);
    wlr_scene_rect_set_color(view->border, acc);
    if (view->ft_handle)
        wlr_foreign_toplevel_handle_v1_set_activated(view->ft_handle, true);

    wlr_scene_node_set_enabled(&view->scene_tree->node, true);
    struct wlr_keyboard *kb = wlr_seat_get_keyboard(seat);
    if (kb)
        wlr_seat_keyboard_notify_enter(seat, surface, kb->keycodes,
                                       kb->num_keycodes, &kb->modifiers);
}

/* 切换工作区: 只显示目标工作区, 并聚焦其最上层窗口 */
static void switch_workspace(struct openos_server *s, int idx) {
    if (idx < 0 || idx >= WORKSPACE_COUNT) return;
    if (idx == s->current_workspace) return;
    s->current_workspace = idx;
    for (int i = 0; i < WORKSPACE_COUNT; i++)
        wlr_scene_node_set_enabled(&s->workspaces[i]->node, i == idx);
    /* 广播工作区状态给切换器 UI */
    if (s->workspace_mgr)
        openos_workspace_set_active(s->workspace_mgr, idx);
    /* 取消正在进行的拖拽 */
    s->grabbed_surface = NULL;
    /* 聚焦该工作区最后一个映射的窗口 */
    struct openos_view *target = NULL, *it;
    wl_list_for_each(it, &s->views, link)
        if (it->workspace == idx && it->mapped) target = it;
    if (target)
        focus_view(target, target->xdg_toplevel->base->surface);
    else
        wlr_seat_keyboard_notify_clear_focus(s->seat);
}

static void begin_move(struct openos_view *view, struct wlr_surface *surf,
                       uint32_t serial) {
    struct openos_server *s = view->server;
    s->grabbed_surface = surf;
    s->grab_x = s->cursor->x;
    s->grab_y = s->cursor->y;
    struct wlr_box geo;
    wlr_xdg_toplevel_get_geometry(view->xdg_toplevel, &geo);
    s->grab_geobox_x = geo.x;
    s->grab_geobox_y = geo.y;
    (void)serial;
}

static void begin_resize(struct openos_view *view, struct wlr_surface *surf,
                         uint32_t edges, uint32_t serial) {
    struct openos_server *s = view->server;
    s->grabbed_surface = surf;
    s->resize_edges = edges;
    s->grab_x = s->cursor->x;
    s->grab_y = s->cursor->y;
    struct wlr_box geo;
    wlr_xdg_toplevel_get_geometry(view->xdg_toplevel, &geo);
    s->grab_geobox_x = geo.x;
    s->grab_geobox_y = geo.y;
    (void)serial;
}

/* ---- view 事件 ---- */
static void on_view_map(struct wl_listener *l, void *data) {
    (void)data;
    struct openos_view *v = wl_container_of(l, v, map);
    v->mapped = true;
    struct wlr_xdg_toplevel *t = v->xdg_toplevel;
    /* 同步任务栏标题/图标标识 */
    if (v->ft_handle) {
        if (t->title)
            wlr_foreign_toplevel_handle_v1_set_title(v->ft_handle, t->title);
        if (t->app_id)
            wlr_foreign_toplevel_handle_v1_set_app_id(v->ft_handle, t->app_id);
    }
    struct wlr_box geo;
    wlr_xdg_toplevel_get_geometry(t, &geo);
    if (geo.x == 0 && geo.y == 0) {
        /* 居中放置 */
        struct wlr_box *o = wlr_output_layout_get_box(v->server->output_layout, NULL);
        wlr_scene_node_set_position(&v->scene_tree->node,
            (o->width - geo.width) / 2, (o->height - geo.height) / 2 + 24);
    }
    focus_view(v, t->base->surface);
}

static void on_view_unmap(struct wl_listener *l, void *data) {
    (void)data;
    struct openos_view *v = wl_container_of(l, v, unmap);
    v->mapped = false;
    struct wlr_seat *seat = v->server->seat;
    if (seat->keyboard_state.focused_surface == v->xdg_toplevel->base->surface)
        wlr_seat_keyboard_notify_clear_focus(seat);
}

static void on_view_destroy(struct wl_listener *l, void *data) {
    (void)data;
    struct openos_view *v = wl_container_of(l, v, destroy);
    if (v->ft_handle) {
        wlr_foreign_toplevel_handle_v1_destroy(v->ft_handle);
        v->ft_handle = NULL;
    }
    wl_list_remove(&v->link);
    free(v);
}

static void on_request_move(struct wl_listener *l, void *data) {
    struct wlr_xdg_toplevel_move_event *e = data;
    struct openos_view *v = wl_container_of(l, v, request_move);
    begin_move(v, e->surface, e->serial);
}

static void on_request_resize(struct wl_listener *l, void *data) {
    struct wlr_xdg_toplevel_resize_event *e = data;
    struct openos_view *v = wl_container_of(l, v, request_resize);
    begin_resize(v, e->surface, e->edges, e->serial);
}

static void on_request_maximize(struct wl_listener *l, void *data) {
    (void)data;
    struct openos_view *v = wl_container_of(l, v, request_maximize);
    if (v->xdg_toplevel->requested.maximized)
        wlr_xdg_toplevel_set_maximized(v->xdg_toplevel, true);
    else
        wlr_xdg_toplevel_set_maximized(v->xdg_toplevel, false);
}

static void on_request_fullscreen(struct wl_listener *l, void *data) {
    (void)data;
    struct openos_view *v = wl_container_of(l, v, request_fullscreen);
    wlr_xdg_toplevel_set_fullscreen(v->xdg_toplevel,
        v->xdg_toplevel->requested.fullscreen);
}

/* ---- xdg-shell: 新顶层窗口 ---- */
static void on_new_xdg_toplevel(struct wl_listener *l, void *data) {
    struct openos_server *s = wl_container_of(l, s, new_xdg_toplevel);
    struct wlr_xdg_toplevel *toplevel = data;

    struct openos_view *v = calloc(1, sizeof *v);
    assert(v);
    v->server = s;
    v->xdg_toplevel = toplevel;

    const int BORDER = 1;
    v->workspace = s->current_workspace;
    v->scene_tree = wlr_scene_tree_create(s->workspaces[v->workspace]);
    float bcol[4]; nui_colorf(NUI_SURFACE_4, bcol);
    v->border = wlr_scene_rect_create(&v->scene_tree->node, 100, 100, bcol);
    v->scene_surface = wlr_scene_surface_create(&v->scene_tree->node,
                                                toplevel->base->surface);
    wlr_scene_node_set_position(&v->scene_surface->node, BORDER, BORDER);

    v->map.notify = on_view_map;
    wl_signal_add(&toplevel->base->events.map, &v->map);
    v->unmap.notify = on_view_unmap;
    wl_signal_add(&toplevel->base->events.unmap, &v->unmap);
    v->destroy.notify = on_view_destroy;
    wl_signal_add(&toplevel->events.destroy, &v->destroy);
    v->request_move.notify = on_request_move;
    wl_signal_add(&toplevel->events.request_move, &v->request_move);
    v->request_resize.notify = on_request_resize;
    wl_signal_add(&toplevel->events.request_resize, &v->request_resize);
    v->request_maximize.notify = on_request_maximize;
    wl_signal_add(&toplevel->events.request_maximize, &v->request_maximize);
    v->request_fullscreen.notify = on_request_fullscreen;
    wl_signal_add(&toplevel->events.request_fullscreen, &v->request_fullscreen);

    /* 任务栏数据源: foreign-toplevel 句柄 */
    if (s->foreign_toplevel) {
        v->ft_handle = wlr_foreign_toplevel_handle_v1_create(s->foreign_toplevel);
        if (v->ft_handle) {
            v->ft_handle->data = v;
            if (toplevel->title)
                wlr_foreign_toplevel_handle_v1_set_title(v->ft_handle, toplevel->title);
            if (toplevel->app_id)
                wlr_foreign_toplevel_handle_v1_set_app_id(v->ft_handle, toplevel->app_id);
            wlr_foreign_toplevel_handle_v1_set_activated(v->ft_handle, false);
        }
    }

    wl_list_insert(&s->views, &v->link);
}

/* ---- foreign-toplevel 请求 (任务栏按钮动作) ---- */
static void ft_request_activate_handler(struct wl_listener *l, void *data) {
    struct openos_server *s = wl_container_of(l, s, ft_request_activate);
    struct wlr_foreign_toplevel_handle_v1_activated_event *ev = data;
    struct openos_view *v = ev->handle->data;
    if (!v) return;
    if (v->workspace != s->current_workspace)
        switch_workspace(s, v->workspace);
    focus_view(v, v->xdg_toplevel->base->surface);
}

static void ft_request_close_handler(struct wl_listener *l, void *data) {
    (void)l;
    struct wlr_foreign_toplevel_handle_v1_close_event *ev = data;
    struct openos_view *v = ev->handle->data;
    if (v) wlr_xdg_toplevel_send_close(v->xdg_toplevel);
}

static void ft_request_maximize_handler(struct wl_listener *l, void *data) {
    (void)l;
    struct wlr_foreign_toplevel_handle_v1_maximized_event *ev = data;
    struct openos_view *v = ev->handle->data;
    if (v) wlr_xdg_toplevel_set_maximized(v->xdg_toplevel, ev->maximized);
}

/* 工作区切换器 UI 的激活请求 */
static void on_workspace_activate(void *data, int idx) {
    struct openos_server *s = data;
    switch_workspace(s, idx);
}

/* ---- layer-shell: 面板/启动器/通知 等桌面层 ---- */
static void on_layer_destroy(struct wl_listener *l, void *data) {
    (void)data;
    struct openos_layer *lay = wl_container_of(l, lay, destroy);
    wl_list_remove(&lay->destroy.link);
    wl_list_remove(&lay->map.link);
    wl_list_remove(&lay->unmap.link);
    wl_list_remove(&lay->configure.link);
    free(lay);
}
static void on_layer_map(struct wl_listener *l, void *data) {
    (void)l; (void)data;
}
static void on_layer_unmap(struct wl_listener *l, void *data) {
    (void)l; (void)data;
}
static void on_layer_configure(struct wl_listener *l, void *data) {
    struct openos_layer *lay = wl_container_of(l, lay, configure);
    struct wlr_layer_surface_v1_configure_event *ev = data;
    wlr_layer_surface_v1_configure(lay->layer_surface, ev->width, ev->height);
}

static void on_new_layer_surface(struct wl_listener *l, void *data) {
    struct openos_server *s = wl_container_of(l, s, new_layer_surface);
    struct wlr_layer_surface_v1 *ls = data;

    if (ls->output == NULL)
        ls->output = wlr_output_layout_get_center_output(s->output_layout);

    struct openos_layer *lay = calloc(1, sizeof *lay);
    assert(lay);
    lay->server = s;
    lay->layer_surface = ls;
    /* 面板/菜单/通知 挂到 blur_tree, 其下方桌面内容将被动态模糊 */
    struct wlr_scene_tree *parent = is_blur_namespace(ls->namespace)
        ? s->blur_tree : &s->scene->tree;
    lay->scene_layer = wlr_scene_layer_surface_v1_create(parent, ls);
    if (!lay->scene_layer) {
        wlr_layer_surface_v1_destroy(ls);
        free(lay);
        return;
    }

    lay->destroy.notify = on_layer_destroy;
    wl_signal_add(&ls->events.destroy, &lay->destroy);
    lay->map.notify = on_layer_map;
    wl_signal_add(&ls->events.map, &lay->map);
    lay->unmap.notify = on_layer_unmap;
    wl_signal_add(&ls->events.unmap, &lay->unmap);
    lay->configure.notify = on_layer_configure;
    wl_signal_add(&ls->events.configure, &lay->configure);
}

/* ---- 输出 ---- */
static void on_output_frame(struct wl_listener *l, void *data) {
    (void)data;
    struct openos_output *o = wl_container_of(l, o, frame);
    struct wlr_scene_output *so = o->scene_output;
    struct openos_server *s = o->server;
    /* 有模糊层内容时走动态模糊管线, 否则常规提交 */
    float bg[4]; nui_colorf(NUI_SURFACE_0, bg);
    if (!openos_blur_render(s->renderer, s->allocator, o->wlr_output, so,
                            s->scene, s->blur_tree, bg))
        wlr_scene_output_commit(so, NULL);
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    wlr_scene_output_send_frame_done(so, &now);
}

static void on_new_output(struct wl_listener *l, void *data) {
    struct openos_server *s = wl_container_of(l, s, new_output);
    struct wlr_output *wlr_output = data;

    wlr_output_init_render(wlr_output, s->allocator, s->renderer);

    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode = wlr_output_preferred_mode(wlr_output);
        wlr_output_set_mode(wlr_output, mode);
    }
    wlr_output_enable(wlr_output, true);
    if (!wlr_output_commit(wlr_output))
        wlr_log(WLR_ERROR, "输出提交失败: %s", wlr_output->name);

    struct openos_output *o = calloc(1, sizeof *o);
    assert(o);
    o->server = s;
    o->wlr_output = wlr_output;
    o->scene_output = wlr_scene_output_layout_add_output(s->scene_layout, wlr_output);
    o->frame.notify = on_output_frame;
    wl_signal_add(&wlr_output->events.frame, &o->frame);
    wl_list_insert(&s->outputs, &o->link);
}

/* ---- 输入: 键盘 ---- */
static void on_keyboard_key(struct wl_listener *l, void *data) {
    struct openos_server *s = wl_container_of(l, s, keyboard_key);
    struct wlr_keyboard_key_event *e = data;
    struct wlr_seat *seat = s->seat;
    struct wlr_keyboard *kb = wlr_seat_get_keyboard(seat);
    /* 快捷键: Esc 关闭焦点窗口; Ctrl+1..4 切换工作区; Ctrl+Tab 下一工作区 */
    if (kb && e->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        bool ctrl = xkb_state_mod_name_is_active(kb->xkb_state,
            XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE);
        const xkb_keysym_t *syms;
        unsigned n = xkb_state_key_get_syms(kb->xkb_state, e->keycode, &syms);
        for (unsigned i = 0; i < n; i++) {
            if (ctrl && syms[i] >= XKB_KEY_1 && syms[i] <= XKB_KEY_4) {
                switch_workspace(s, syms[i] - XKB_KEY_1);
            } else if (ctrl && syms[i] == XKB_KEY_Tab) {
                switch_workspace(s, (s->current_workspace + 1) % WORKSPACE_COUNT);
            } else if (syms[i] == XKB_KEY_Escape) {
                struct wlr_surface *f = seat->keyboard_state.focused_surface;
                if (f) wlr_xdg_toplevel_send_close(
                    wlr_xdg_surface_from_wlr_surface(f)->toplevel);
            }
        }
    }
    wlr_seat_keyboard_notify_key(seat, e->time_msec, e->keycode, e->state);
}

static void on_keyboard_destroy(struct wl_listener *l, void *data) {
    (void)data;
    struct openos_server *s = wl_container_of(l, s, keyboard_destroy);
    wl_list_remove(&s->keyboard_key.link);
    wl_list_remove(&s->keyboard_destroy.link);
}

/* ---- 输入: 通用 new_input ---- */
static void on_new_input(struct wl_listener *l, void *data) {
    struct openos_server *s = wl_container_of(l, s, new_input);
    struct wlr_input_device *dev = data;
    switch (dev->type) {
    case WLR_INPUT_DEVICE_KEYBOARD: {
        struct wlr_keyboard *kb = wlr_keyboard_from_input_device(dev);
        wlr_keyboard_set_keymap(kb, wlr_xkb_keymap_from_names(NULL, NULL));
        wlr_keyboard_set_repeat_info(kb, 25, 600);
        s->keyboard_key.notify = on_keyboard_key;
        wl_signal_add(&kb->events.key, &s->keyboard_key);
        s->keyboard_destroy.notify = on_keyboard_destroy;
        wl_signal_add(&dev->events.destroy, &s->keyboard_destroy);
        wlr_seat_set_keyboard(s->seat, kb);
        break;
    }
    case WLR_INPUT_DEVICE_POINTER:
        wlr_cursor_attach_input_device(s->cursor, dev);
        break;
    default:
        break;
    }
    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&s->seat->keyboards))
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    wlr_seat_set_capabilities(s->seat, caps);
}

/* ---- 指针: 移动 / 拖拽 ---- */
static void on_cursor_motion(struct wl_listener *l, void *data) {
    struct openos_server *s = wl_container_of(l, s, cursor_motion);
    struct wlr_pointer_motion_event *e = data;
    wlr_cursor_move(s->cursor, &e->pointer->base, e->dx, e->dy);
    on_cursor_update(s);
}

static void on_cursor_motion_abs(struct wl_listener *l, void *data) {
    struct openos_server *s = wl_container_of(l, s, cursor_motion_abs);
    struct wlr_pointer_motion_absolute_event *e = data;
    wlr_cursor_warp_absolute(s->cursor, &e->pointer->base, e->x, e->y);
    on_cursor_update(s);
}

static void on_cursor_button(struct wl_listener *l, void *data) {
    struct openos_server *s = wl_container_of(l, s, cursor_button);
    struct wlr_pointer_button_event *e = data;
    wlr_seat_pointer_notify_button(s->seat, e->time_msec, e->button, e->state);
    if (e->state == WL_POINTER_BUTTON_STATE_RELEASED)
        s->grabbed_surface = NULL;
    on_cursor_update(s);
}

static void on_cursor_axis(struct wl_listener *l, void *data) {
    struct openos_server *s = wl_container_of(l, s, cursor_axis);
    struct wlr_pointer_axis_event *e = data;
    wlr_seat_pointer_notify_axis(s->seat, e->time_msec, e->orientation,
        e->delta, e->delta_discrete, e->source, e->relative_direction);
}

static void on_cursor_frame(struct wl_listener *l, void *data) {
    (void)l; (void)data;
    struct openos_server *s = wl_container_of(l, s, cursor_frame);
    wlr_seat_pointer_notify_frame(s->seat);
}

static void on_request_cursor(struct wl_listener *l, void *data) {
    (void)l;
    struct wlr_seat_pointer_request_set_cursor_event *e = data;
    struct openos_server *s = wl_container_of(e->seat, s, seat);
    if (e->seat_client == s->seat->pointer_state.focused_client)
        wlr_cursor_set_surface(s->cursor, e->surface, e->hotspot_x, e->hotspot_y);
}

/* 指针移动后: 命中测试 + 拖拽窗口 */
static void on_cursor_update(struct openos_server *s) {
    if (s->grabbed_surface) {
        struct wlr_surface *surf = s->grabbed_surface;
        struct openos_view *v = NULL;
        struct openos_view *it;
        wl_list_for_each(it, &s->views, link)
            if (it->xdg_toplevel->base->surface == surf) { v = it; break; }
        if (v) {
            double dx = s->cursor->x - s->grab_x;
            double dy = s->cursor->y - s->grab_y;
            wlr_scene_node_set_position(&v->scene_tree->node,
                s->grab_geobox_x + dx, s->grab_geobox_y + dy);
        }
        return;
    }
    struct wlr_scene_node *node = wlr_scene_node_at(&s->scene->tree,
        s->cursor->x, s->cursor->y, NULL, NULL);
    if (node && node->type == WLR_SCENE_NODE_SURFACE) {
        struct wlr_surface *surf = wlr_scene_surface_from_node(node)->surface;
        wlr_seat_pointer_notify_enter(s->seat, surf, s->cursor->x, s->cursor->y);
    } else {
        wlr_seat_pointer_notify_enter(s->seat, NULL, s->cursor->x, s->cursor->y);
    }
    wlr_xcursor_manager_set_cursor_image(s->xcursor_mgr, "left_ptr", s->cursor);
}

int main(int argc, char *argv[]) {
    (void)argc; (void)argv;
    struct openos_server server = {0};
    wl_list_init(&server.views);
    wl_list_init(&server.outputs);

    server.display = wl_display_create();
    server.backend = wlr_backend_autocreate(server.display, NULL);
    server.renderer = wlr_renderer_autocreate(server.backend);
    server.allocator = wlr_allocator_autocreate(server.backend, server.renderer);
    server.output_layout = wlr_output_layout_create();
    wlr_renderer_init_wl_display(server.renderer, server.display);

    server.scene = wlr_scene_create();
    server.scene_layout = wlr_scene_output_layout_create(server.scene, server.output_layout);

    /* 多工作区: 4 棵子树, 默认只启用第 0 个 */
    for (int i = 0; i < WORKSPACE_COUNT; i++) {
        server.workspaces[i] = wlr_scene_tree_create(&server.scene->tree);
        wlr_scene_node_set_enabled(&server.workspaces[i]->node, i == 0);
    }
    server.current_workspace = 0;
    /* 模糊层树: 后创建, 渲染时位于窗口之上 */
    server.blur_tree = wlr_scene_tree_create(&server.scene->tree);

    float bg[4]; nui_colorf(NUI_SURFACE_0, bg);
    wlr_scene_set_background_color(server.scene, bg);

    wlr_compositor_create(server.display, server.renderer);
    wlr_data_device_manager_create(server.display);
    server.xdg_shell = wlr_xdg_shell_create(server.display);
    server.layer_shell = wlr_layer_shell_v1_create(server.display);
    /* 任务栏窗口列表 (标准协议) + 工作区切换器 (自研协议) */
    server.foreign_toplevel = wlr_foreign_toplevel_manager_v1_create(server.display);
    server.workspace_mgr = openos_workspace_manager_create(server.display,
        WORKSPACE_COUNT, 0, on_workspace_activate, &server);
    server.ft_request_activate.notify = ft_request_activate_handler;
    wl_signal_add(&server.foreign_toplevel->events.request_activate,
        &server.ft_request_activate);
    server.ft_request_close.notify = ft_request_close_handler;
    wl_signal_add(&server.foreign_toplevel->events.request_close,
        &server.ft_request_close);
    server.ft_request_maximize.notify = ft_request_maximize_handler;
    wl_signal_add(&server.foreign_toplevel->events.request_maximize,
        &server.ft_request_maximize);

    server.cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(server.cursor, server.output_layout);
    server.xcursor_mgr = wlr_xcursor_manager_create(NULL, 24);
    wlr_xcursor_manager_load(server.xcursor_mgr, 1);
    server.seat = wlr_seat_create(server.display, "openos-seat");

    server.new_output.notify = on_new_output;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);
    server.new_input.notify = on_new_input;
    wl_signal_add(&server.backend->events.new_input, &server.new_input);
    server.new_xdg_toplevel.notify = on_new_xdg_toplevel;
    wl_signal_add(&server.xdg_shell->events.new_toplevel, &server.new_xdg_toplevel);
    server.new_layer_surface.notify = on_new_layer_surface;
    wl_signal_add(&server.layer_shell->events.new_surface, &server.new_layer_surface);

    server.cursor_motion.notify = on_cursor_motion;
    wl_signal_add(&server.cursor->events.motion, &server.cursor_motion);
    server.cursor_motion_abs.notify = on_cursor_motion_abs;
    wl_signal_add(&server.cursor->events.motion_absolute, &server.cursor_motion_abs);
    server.cursor_button.notify = on_cursor_button;
    wl_signal_add(&server.cursor->events.button, &server.cursor_button);
    server.cursor_axis.notify = on_cursor_axis;
    wl_signal_add(&server.cursor->events.axis, &server.cursor_axis);
    server.cursor_frame.notify = on_cursor_frame;
    wl_signal_add(&server.cursor->events.frame, &server.cursor_frame);
    server.request_cursor.notify = on_request_cursor;
    wl_signal_add(&server.seat->events.request_set_cursor, &server.request_cursor);

    const char *sock = wl_display_add_socket_auto(server.display);
    if (!sock) {
        wlr_backend_destroy(server.backend);
        return 1;
    }
    wlr_log(WLR_INFO, "OPENOS DE (Wayland) 已启动. WAYLAND_DISPLAY=%s", sock);
    wlr_log(WLR_INFO, "NUI2 主题: 20 层深色 / 强调色 Cyan");

    wlr_backend_start(server.backend);
    wl_display_run(server.display);

    wl_display_destroy(server.display);
    return 0;
}
