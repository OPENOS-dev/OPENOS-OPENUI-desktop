#ifndef OPENOS_DECORATIONS_H
#define OPENOS_DECORATIONS_H

/* OPENOS 窗口装饰 (服务器端标题栏)
 * 悬浮设计: 标题栏(左) + 窗口控制按钮(右) 两个独立圆角块
 * Windows 风格: 关闭/最大化/最小化按钮在右侧, 明确方形按钮
 * 使用 wlr_scene_rect 绘制 + wlr_scene_buffer 圆角纹理
 */

#include <wlr/types/wlr_scene.h>
#include <wlr/types/wlr_xdg_shell.h>

#define DECO_TITLEBAR_H     28   /* 标题栏块高度 */
#define DECO_TITLE_BAR_W    120  /* 标题栏块宽度 (左) */
#define DECO_CONTROLS_W     80   /* 控制块宽度 (右) */
#define DECO_BUTTON_W       22   /* 单个按钮宽度 */
#define DECO_BUTTON_H       22   /* 单个按钮高度 */
#define DECO_FLOATING_MARGIN 6   /* 悬浮块与窗口边缘间距 */
#define DECO_BUTTON_GAP     4    /* 按钮间距 */
#define DECO_RADIUS         8    /* 圆角半径 */

/* 装饰状态 */
struct openos_deco {
    struct wlr_scene_tree *tree;       /* 装饰根节点 */

    /* 悬浮标题栏 (左) */
    struct wlr_scene_rect *title_bar;  /* 标题栏背景 */

    /* 悬浮控制块 (右) */
    struct wlr_scene_rect *controls_bg; /* 控制块背景 */
    struct wlr_scene_rect *close_btn;   /* 关闭按钮 */
    struct wlr_scene_rect *max_btn;     /* 最大化按钮 */
    struct wlr_scene_rect *min_btn;     /* 最小化按钮 */

    bool hover_close;
    bool hover_max;
    bool hover_min;

    int window_width;  /* 当前窗口宽度, 用于 resize */
};

/* 创建装饰 */
void deco_create(struct openos_deco *deco, struct wlr_scene_tree *parent,
                 int width);

/* 更新装饰尺寸 (窗口 resize 时) */
void deco_resize(struct openos_deco *deco, int width);

/* 销毁装饰 */
void deco_destroy(struct openos_deco *deco);

/* 命中测试: 返回 0=无, 1=关闭, 2=最大化, 3=最小化, 4=标题栏拖拽区, 5=控制块拖拽区 */
int deco_hit_test(struct openos_deco *deco, double sx, double sy);

/* 更新按钮 hover 状态 */
void deco_set_hover(struct openos_deco *deco, double sx, double sy);

/* 重置 hover */
void deco_clear_hover(struct openos_deco *deco);

/* 获取控制块右侧起始 x 坐标 (供 compositor 定位) */
int deco_controls_x(struct openos_deco *deco, int width);

/* 获取标题栏高度 (供 compositor 定位内容区) */
int deco_titlebar_height(void);

#endif /* OPENOS_DECORATIONS_H */