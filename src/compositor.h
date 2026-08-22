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

#ifndef OPENOS_COMPOSITOR_H
#define OPENOS_COMPOSITOR_H

/* OPENOS 合成器 — 公共类型 (供 compositor.c / tiling.c / gestures.c 等共用) */

#include <wayland-server-core.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_foreign_toplevel_management_v1.h>
#include "decorations.h"
#include "workspace.h"

#define WORKSPACE_COUNT 4

struct openos_server;

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
    int workspace;

    /* 窗口装饰 */
    struct openos_deco deco;

    /* 窗口动画 */
    double anim_opacity;     /* 0.0 ~ 1.0 */
    double anim_scale;       /* 0.0 ~ 1.0 */
    bool animating_in;
    bool animating_out;

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
    struct wl_list views;
    struct wl_list outputs;

    struct wlr_scene_tree *workspaces[WORKSPACE_COUNT];
    int current_workspace;
    struct wlr_scene_tree *blur_tree;

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
    struct wl_listener keyboard_modifiers;
    struct wl_listener gesture_swipe_begin;
    struct wl_listener gesture_swipe_update;
    struct wl_listener gesture_swipe_end;
    struct wl_listener gesture_pinch_begin;
    struct wl_listener gesture_pinch_update;
    struct wl_listener gesture_pinch_end;

    int gesture_swipe_fingers;
    double gesture_swipe_dx;
    double gesture_swipe_dy;
};

/* 前向声明: compositor.c 中的函数, 供 tiling.c / gestures.c 引用 */
void focus_view(struct openos_view *view, struct wlr_surface *surface);
void switch_workspace(struct openos_server *s, int idx);

#endif /* OPENOS_COMPOSITOR_H */