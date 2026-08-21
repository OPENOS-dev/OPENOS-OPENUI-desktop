#define _POSIX_C_SOURCE 200809L
/* OPENOS 桌面环境 — Wayland 合成器 (wlroots)
 * 提供: 窗口管理(xdg-shell) + 桌面层(layer-shell) + NUI2 主题
 *      + 窗口装饰(SSD) + 窗口动画 + 触摸板手势 + 分屏平铺
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
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include <wlr/types/wlr_pointer.h>
#include <wlr/util/log.h>
#include "compositor.h"
#include "nui2.h"
#include "blur.h"
#include "decorations.h"
#include "tiling.h"
#include "gestures.h"

/* 属于"模糊层"的 layer-shell namespace */
static bool is_blur_namespace(const char *ns) {
    return ns &&
        (strcmp(ns, "openos-panel") == 0 ||
         strcmp(ns, "openos-menu") == 0 ||
         strcmp(ns, "openos-notifyd") == 0);
}

/* 0xRRGGBB -> 归一化 float[4] */
static void nui_colorf(uint32_t hex, float out[4]) {
    out[0] = ((hex >> 16) & 0xFF) / 255.0f;
    out[1] = ((hex >> 8) & 0xFF) / 255.0f;
    out[2] = (hex & 0xFF) / 255.0f;
    out[3] = 1.0f;
}

/* ---- 窗口动画 ---- */
static void view_animate_in(struct openos_view *view) {
    view->anim_opacity = 0.0;
    view->anim_scale = 0.85;
    view->animating_in = true;
    view->animating_out = false;
}

static void view_animate_out(struct openos_view *view) {
    view->anim_opacity = 1.0;
    view->anim_scale = 1.0;
    view->animating_in = false;
    view->animating_out = true;
}

static void update_animations(struct openos_server *server) {
    struct openos_view *view;
    wl_list_for_each(view, &server->views, link) {
        if (view->animating_in) {
            view->anim_opacity += 0.08f;
            view->anim_scale += 0.015f;
            if (view->anim_opacity >= 1.0f) {
                view->anim_opacity = 1.0f;
                view->anim_scale = 1.0f;
                view->animating_in = false;
            }
        }
        if (view->animating_out) {
            view->anim_opacity -= 0.08f;
            view->anim_scale -= 0.015f;
            if (view->anim_opacity <= 0.0f) {
                view->anim_opacity = 0.0f;
                view->anim_scale = 0.0f;
                view->animating_out = false;
                /* 动画完成后真正销毁 */
                if (view->scene_tree)
                    wlr_scene_node_set_enabled(&view->scene_tree->node, false);
            }
        }
    }
}

/* ---- 聚焦 ---- */
void focus_view(struct openos_view *view, struct wlr_surface *surface) {
    if (view == NULL) return;
    struct openos_server *s = view->server;
    struct wlr_seat *seat = s->seat;
    struct wlr_surface *prev = seat->keyboard_state.focused_surface;
    if (prev == surface) return;

    if (prev) wlr_seat_keyboard_notify_clear_focus(seat);

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

/* ---- 工作区切换 ---- */
void switch_workspace(struct openos_server *s, int idx) {
    if (idx < 0 || idx >= WORKSPACE_COUNT) return;
    if (idx == s->current_workspace) return;
    s->current_workspace = idx;
    for (int i = 0; i < WORKSPACE_COUNT; i++)
        wlr_scene_node_set_enabled(&s->workspaces[i]->node, i == idx);
    if (s->workspace_mgr)
        openos_workspace_set_active(s->workspace_mgr, idx);
    s->grabbed_surface = NULL;
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

    /* 启动进入动画 */
    view_animate_in(v);

    if (v->ft_handle) {
        if (t->title)
            wlr_foreign_toplevel_handle_v1_set_title(v->ft_handle, t->title);
        if (t->app_id)
            wlr_foreign_toplevel_handle_v1_set_app_id(v->ft_handle, t->app_id);
    }
    struct wlr_box geo;
    wlr_xdg_toplevel_get_geometry(t, &geo);
    if (geo.x == 0 && geo.y == 0) {
        struct wlr_box *o = wlr_output_layout_get_box(v->server->output_layout, NULL);
        wlr_scene_node_set_position(&v->scene_tree->node,
            (o->width - geo.width) / 2,
            (o->height - geo.height) / 2 + 24);
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
    deco_destroy(&v->deco);
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
    v->anim_opacity = 1.0;
    v->anim_scale = 1.0;

    const int BORDER = 1;
    v->workspace = s->current_workspace;
    v->scene_tree = wlr_scene_tree_create(s->workspaces[v->workspace]);
    float bcol[4]; nui_colorf(NUI_SURFACE_4, bcol);
    v->border = wlr_scene_rect_create(&v->scene_tree->node, 100, 100, bcol);

    /* 创建窗口装饰 (标题栏 + 按钮) */
    deco_create(&v->deco, v->scene_tree, 100);

    /* 场景表面在装饰下方 (标题栏之下) */
    v->scene_surface = wlr_scene_surface_create(&v->scene_tree->node,
                                                toplevel->base->surface);
    wlr_scene_node_set_position(&v->scene_surface->node,
                                DECO_BORDER_W, deco_titlebar_height());

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

    /* 任务栏句柄 */
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

/* ---- foreign-toplevel 请求 ---- */
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

/* 工作区切换器 UI 激活请求 */
static void on_workspace_activate(void *data, int idx) {
    struct openos_server *s = data;
    switch_workspace(s, idx);
}

/* ---- layer-shell ---- */
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

    /* 更新窗口动画 */
    update_animations(s);

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

/* ---- 键盘 ---- */
static void on_keyboard_key(struct wl_listener *l, void *data) {
    struct openos_server *s = wl_container_of(l, s, keyboard_key);
    struct wlr_keyboard_key_event *e = data;
    struct wlr_seat *seat = s->seat;
    struct wlr_keyboard *kb = wlr_seat_get_keyboard(seat);

    if (kb && e->state == WL_KEYBOARD_KEY_STATE_PRESSED) {
        bool ctrl = xkb_state_mod_name_is_active(kb->xkb_state,
            XKB_MOD_NAME_CTRL, XKB_STATE_MODS_EFFECTIVE);
        bool alt = xkb_state_mod_name_is_active(kb->xkb_state,
            XKB_MOD_NAME_ALT, XKB_STATE_MODS_EFFECTIVE);
        bool meta = xkb_state_mod_name_is_active(kb->xkb_state,
            XKB_MOD_NAME_LOGO, XKB_STATE_MODS_EFFECTIVE);
        const xkb_keysym_t *syms;
        unsigned n = xkb_state_key_get_syms(kb->xkb_state, e->keycode, &syms);
        for (unsigned i = 0; i < n; i++) {
            if (ctrl && syms[i] >= XKB_KEY_1 && syms[i] <= XKB_KEY_4) {
                switch_workspace(s, syms[i] - XKB_KEY_1);
            } else if (ctrl && syms[i] == XKB_KEY_Tab) {
                switch_workspace(s, (s->current_workspace + 1) % WORKSPACE_COUNT);
            } else if (syms[i] == XKB_KEY_Escape) {
                struct wlr_surface *f = seat->keyboard_state.focused_surface;
                if (f) {
                    struct wlr_xdg_surface *xdg =
                        wlr_xdg_surface_from_wlr_surface(f);
                    if (xdg && xdg->toplevel)
                        wlr_xdg_toplevel_send_close(xdg->toplevel);
                }
            }
            /* 分屏平铺: Meta + Left/Right = 左/右半屏 */
            if (meta && syms[i] == XKB_KEY_Left) {
                tile_focused(s, TILE_LEFT);
            } else if (meta && syms[i] == XKB_KEY_Right) {
                tile_focused(s, TILE_RIGHT);
            } else if (meta && syms[i] == XKB_KEY_Up) {
                tile_focused(s, TILE_FULL);
            } else if (meta && syms[i] == XKB_KEY_Down) {
                tile_focused(s, TILE_CENTER);
            }
            /* 窗口装饰: Meta + W = 关闭 */
            if (meta && syms[i] == XKB_KEY_w) {
                struct wlr_surface *f = seat->keyboard_state.focused_surface;
                if (f) {
                    struct wlr_xdg_surface *xdg =
                        wlr_xdg_surface_from_wlr_surface(f);
                    if (xdg && xdg->toplevel)
                        wlr_xdg_toplevel_send_close(xdg->toplevel);
                }
            }
            /* Meta + M = 最大化 */
            if (meta && syms[i] == XKB_KEY_m) {
                struct wlr_surface *f = seat->keyboard_state.focused_surface;
                if (f) {
                    struct wlr_xdg_surface *xdg =
                        wlr_xdg_surface_from_wlr_surface(f);
                    if (xdg && xdg->toplevel) {
                        wlr_xdg_toplevel_set_maximized(xdg->toplevel,
                            !xdg->toplevel->requested.maximized);
                    }
                }
            }
        }
    }
    wlr_seat_keyboard_notify_key(seat, e->time_msec, e->keycode, e->state);
}

static void on_keyboard_modifiers(struct wl_listener *l, void *data) {
    (void)data;
    struct openos_server *s = wl_container_of(l, s, keyboard_modifiers);
    struct wlr_keyboard *kb = wlr_seat_get_keyboard(s->seat);
    if (kb)
        wlr_seat_keyboard_notify_modifiers(s->seat, &kb->modifiers);
}

static void on_keyboard_destroy(struct wl_listener *l, void *data) {
    (void)data;
    struct openos_server *s = wl_container_of(l, s, keyboard_destroy);
    wl_list_remove(&s->keyboard_key.link);
    wl_list_remove(&s->keyboard_destroy.link);
    wl_list_remove(&s->keyboard_modifiers.link);
}

/* ---- 输入 ---- */
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
        s->keyboard_modifiers.notify = on_keyboard_modifiers;
        wl_signal_add(&kb->events.modifiers, &s->keyboard_modifiers);
        s->keyboard_destroy.notify = on_keyboard_destroy;
        wl_signal_add(&dev->events.destroy, &s->keyboard_destroy);
        wlr_seat_set_keyboard(s->seat, kb);
        break;
    }
    case WLR_INPUT_DEVICE_POINTER: {
        struct wlr_pointer *ptr = wlr_pointer_from_input_device(dev);
        wlr_cursor_attach_input_device(s->cursor, dev);
        /* 注册触摸板手势事件 */
        s->gesture_swipe_begin.notify = handle_gesture_swipe_begin;
        wl_signal_add(&ptr->events.gesture_swipe_begin,
                      &s->gesture_swipe_begin);
        s->gesture_swipe_update.notify = handle_gesture_swipe_update;
        wl_signal_add(&ptr->events.gesture_swipe_update,
                      &s->gesture_swipe_update);
        s->gesture_swipe_end.notify = handle_gesture_swipe_end;
        wl_signal_add(&ptr->events.gesture_swipe_end,
                      &s->gesture_swipe_end);
        break;
    }
    default:
        break;
    }
    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&s->seat->keyboards))
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    wlr_seat_set_capabilities(s->seat, caps);
}

/* ---- 指针 ---- */
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

    /* 处理装饰按钮点击 */
    if (e->state == WL_POINTER_BUTTON_STATE_PRESSED) {
        struct wlr_surface *f = s->seat->pointer_state.focused_surface;
        if (f) {
            struct openos_view *v;
            wl_list_for_each(v, &s->views, link) {
                if (v->xdg_toplevel &&
                    v->xdg_toplevel->base->surface == f) {
                    /* 命中测试装饰 */
                    double sx = s->cursor->x;
                    double sy = s->cursor->y;
                    double node_x = 0, node_y = 0;
                    if (wlr_scene_node_coords(&v->scene_tree->node,
                                              &node_x, &node_y)) {
                        int hit = deco_hit_test(&v->deco,
                            sx - node_x, sy - node_y);
                        if (hit == 1) {
                            wlr_xdg_toplevel_send_close(v->xdg_toplevel);
                        } else if (hit == 2) {
                            wlr_xdg_toplevel_set_maximized(v->xdg_toplevel,
                                !v->xdg_toplevel->requested.maximized);
                        } else if (hit == 4 || hit == 5) {
                            /* 标题栏/控制块拖拽: 模拟 request_move */
                            if (v->xdg_toplevel->base->surface) {
                                s->grabbed_surface = v->xdg_toplevel->base->surface;
                                s->grab_x = s->cursor->x;
                                s->grab_y = s->cursor->y;
                                struct wlr_box geo;
                                wlr_xdg_toplevel_get_geometry(
                                    v->xdg_toplevel, &geo);
                                s->grab_geobox_x = geo.x;
                                s->grab_geobox_y = geo.y;
                            }
                        }
                    }
                    break;
                }
            }
        }
    }

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

/* 指针更新: 命中测试 + 拖拽 + 装饰 hover */
void on_cursor_update(struct openos_server *s) {
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

        /* 更新装饰 hover */
        struct openos_view *v;
        wl_list_for_each(v, &s->views, link) {
            if (v->scene_surface &&
                &v->scene_surface->node == node) {
                double nx = 0, ny = 0;
                if (wlr_scene_node_coords(&v->scene_tree->node, &nx, &ny)) {
                    deco_set_hover(&v->deco,
                        s->cursor->x - nx, s->cursor->y - ny);
                }
                break;
            } else {
                deco_clear_hover(&v->deco);
            }
        }
    } else {
        wlr_seat_pointer_notify_enter(s->seat, NULL, s->cursor->x, s->cursor->y);
        /* 清除所有装饰 hover */
        struct openos_view *v;
        wl_list_for_each(v, &s->views, link) {
            deco_clear_hover(&v->deco);
        }
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

    /* 多工作区 */
    for (int i = 0; i < WORKSPACE_COUNT; i++) {
        server.workspaces[i] = wlr_scene_tree_create(&server.scene->tree);
        wlr_scene_node_set_enabled(&server.workspaces[i]->node, i == 0);
    }
    server.current_workspace = 0;
    server.blur_tree = wlr_scene_tree_create(&server.scene->tree);

    float bg[4]; nui_colorf(NUI_SURFACE_0, bg);
    wlr_scene_set_background_color(server.scene, bg);

    wlr_compositor_create(server.display, server.renderer);
    wlr_data_device_manager_create(server.display);
    server.xdg_shell = wlr_xdg_shell_create(server.display);
    server.layer_shell = wlr_layer_shell_v1_create(server.display);
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

    /* 初始化手势 */
    gestures_init(&server);

    const char *sock = wl_display_add_socket_auto(server.display);
    if (!sock) {
        wlr_backend_destroy(server.backend);
        return 1;
    }
    wlr_log(WLR_INFO, "OPENOS DE (Wayland) 已启动. WAYLAND_DISPLAY=%s", sock);
    wlr_log(WLR_INFO, "NUI2 主题: 20 层深色 / 强调色 Cyan / 窗口装饰 / 手势 / 平铺");

    wlr_backend_start(server.backend);
    wl_display_run(server.display);

    wl_display_destroy(server.display);
    return 0;
}