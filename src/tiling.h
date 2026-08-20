#ifndef OPENOS_TILING_H
#define OPENOS_TILING_H

/* OPENOS 分屏平铺管理
 * 基本分屏: 左半 / 右半 / 上三分之一 / 下三分之二 / 全屏
 * 通过键盘快捷键触发 (Meta + Left/Right/Up/Down)
 */

#include <wayland-server-core.h>
#include <wlr/types/wlr_output_layout.h>

struct openos_server;

/* 分屏方向 */
enum openos_tile_dir {
    TILE_LEFT,
    TILE_RIGHT,
    TILE_UP,
    TILE_DOWN,
    TILE_FULL,
    TILE_CENTER,
};

/* 分屏一个视图: 根据 direction 计算目标位置并设置 */
void tile_view(struct openos_server *server, void *view_data,
               enum openos_tile_dir dir);

/* 对当前聚焦窗口执行分屏 */
void tile_focused(struct openos_server *server, enum openos_tile_dir dir);

#endif /* OPENOS_TILING_H */