#include "tiling.h"
#include "compositor.h"
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>

/* 获取可用输出盒 */
static struct wlr_box get_usable_box(struct openos_server *server) {
    struct wlr_box box = {0};
    wlr_output_layout_get_box(server->output_layout, NULL, &box);
    /* 减去面板高度 (顶部任务栏) */
    box.y += 32;
    box.height -= 32;
    return box;
}

/* 分屏一个视图 */
void tile_view(struct openos_server *server, void *view_data,
               enum openos_tile_dir dir) {
    struct openos_view *view = view_data;
    if (!view || !view->scene_tree) return;

    struct wlr_box box = get_usable_box(server);
    int x = 0, y = 0, w = 0, h = 0;

    switch (dir) {
    case TILE_LEFT:
        x = box.x;
        y = box.y;
        w = box.width / 2;
        h = box.height;
        break;
    case TILE_RIGHT:
        x = box.x + box.width / 2;
        y = box.y;
        w = box.width / 2;
        h = box.height;
        break;
    case TILE_UP:
        x = box.x;
        y = box.y;
        w = box.width;
        h = box.height * 2 / 3;
        break;
    case TILE_DOWN:
        x = box.x;
        y = box.y + box.height * 2 / 3;
        w = box.width;
        h = box.height / 3;
        break;
    case TILE_FULL:
        x = box.x;
        y = box.y;
        w = box.width;
        h = box.height;
        break;
    case TILE_CENTER:
        x = box.x + box.width / 4;
        y = box.y + box.height / 4;
        w = box.width / 2;
        h = box.height / 2;
        break;
    }

    wlr_scene_node_set_position(&view->scene_tree->node, x, y);
    if (view->xdg_toplevel) {
        wlr_xdg_toplevel_set_size(view->xdg_toplevel, w, h);
    }
}

/* 对当前聚焦窗口执行分屏 */
void tile_focused(struct openos_server *server, enum openos_tile_dir dir) {
    if (!server->seat || !server->seat->keyboard_state.focused_surface)
        return;

    struct wlr_surface *focused = server->seat->keyboard_state.focused_surface;
    struct openos_view *view;
    wl_list_for_each(view, &server->views, link) {
        if (view->xdg_toplevel &&
            view->xdg_toplevel->base->surface == focused) {
            tile_view(server, view, dir);
            return;
        }
    }
}