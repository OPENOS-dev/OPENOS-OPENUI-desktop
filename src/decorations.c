#include "decorations.h"
#include "openui.h"

/* OPENOS 窗口装饰 — 悬浮设计
 *
 * 布局:
 *   ┌──────────────┐                ┌──────────────┐
 *   │  App Name     │                │ ─  □  ✕      │
 *   └──────────────┘                └──────────────┘
 *   ┌──────────────────────────────────────────────┐
 *   │                Content Area                   │
 *   │               (window surface)                │
 *   └──────────────────────────────────────────────┘
 *
 * 标题栏(左) 和 控制块(右) 是两个独立悬浮块, 中间透明
 * 按钮: Windows 风格, 明确方形按钮, 从右到左: 关闭/最大化/最小化
 */

/* 从 OUI 颜色宏提取 float 分量 */
static void colorf(uint32_t hex, float out[4], float alpha) {
    out[0] = ((hex >> 16) & 0xFF) / 255.0f;
    out[1] = ((hex >> 8) & 0xFF) / 255.0f;
    out[2] = (hex & 0xFF) / 255.0f;
    out[3] = alpha;
}

/* 创建悬浮标题栏 (左) 和控制块 (右) */
void deco_create(struct openos_deco *deco, struct wlr_scene_tree *parent,
                 int width) {
    deco->tree = wlr_scene_tree_create(parent);
    deco->hover_close = false;
    deco->hover_max = false;
    deco->hover_min = false;
    deco->window_width = width;

    /* ---- 1. 悬浮标题栏 (左) ---- */
    float title_bg[4];
    colorf(OUI_SURFACE, title_bg, 0.85f);
    deco->title_bar = wlr_scene_rect_create(&deco->tree->node,
                                             DECO_TITLE_BAR_W, DECO_TITLEBAR_H,
                                             title_bg);
    wlr_scene_node_set_position(&deco->title_bar->node,
                                DECO_FLOATING_MARGIN, DECO_FLOATING_MARGIN);

    /* ---- 2. 悬浮控制块 (右) ---- */
    float ctrl_bg[4];
    colorf(OUI_SURFACE, ctrl_bg, 0.85f);
    deco->controls_bg = wlr_scene_rect_create(&deco->tree->node,
                                               DECO_CONTROLS_W, DECO_TITLEBAR_H,
                                               ctrl_bg);

    /* 按钮默认色: 轻微亮色 */
    float btn_default[4];
    colorf(OUI_ON_SURFACE, btn_default, 0.08f);
    float close_default[4];
    colorf(OUI_ON_SURFACE, close_default, 0.06f);

    /* 计算控制块位置: 右侧 */
    int cx = width - DECO_CONTROLS_W - DECO_FLOATING_MARGIN;
    wlr_scene_node_set_position(&deco->controls_bg->node,
                                cx, DECO_FLOATING_MARGIN);

    /* 按钮从右到左排列: 关闭 -> 最大化 -> 最小化 */
    int btn_right = DECO_CONTROLS_W - DECO_FLOATING_MARGIN;
    int btn_center_y = (DECO_TITLEBAR_H - DECO_BUTTON_H) / 2;

    /* 关闭按钮 (最右) */
    float close_color[4];
    colorf(OUI_ERROR, close_color, 0.15f);
    deco->close_btn = wlr_scene_rect_create(&deco->tree->node,
                                            DECO_BUTTON_W, DECO_BUTTON_H,
                                            close_color);
    wlr_scene_node_set_position(&deco->close_btn->node,
                               cx + btn_right - DECO_BUTTON_W, btn_center_y);

    /* 最大化按钮 (中间) */
    deco->max_btn = wlr_scene_rect_create(&deco->tree->node,
                                          DECO_BUTTON_W, DECO_BUTTON_H,
                                          btn_default);
    wlr_scene_node_set_position(&deco->max_btn->node,
                               cx + btn_right - DECO_BUTTON_W - DECO_BUTTON_GAP - DECO_BUTTON_W,
                               btn_center_y);

    /* 最小化按钮 (左) */
    deco->min_btn = wlr_scene_rect_create(&deco->tree->node,
                                          DECO_BUTTON_W, DECO_BUTTON_H,
                                          btn_default);
    wlr_scene_node_set_position(&deco->min_btn->node,
                               cx + btn_right - 2 * (DECO_BUTTON_W + DECO_BUTTON_GAP) - DECO_BUTTON_W,
                               btn_center_y);
}

/* 更新装饰尺寸 */
void deco_resize(struct openos_deco *deco, int width) {
    deco->window_width = width;

    /* 标题栏固定在左 */
    wlr_scene_node_set_position(&deco->title_bar->node,
                                DECO_FLOATING_MARGIN, DECO_FLOATING_MARGIN);

    /* 控制块固定在右 */
    int cx = width - DECO_CONTROLS_W - DECO_FLOATING_MARGIN;
    wlr_scene_node_set_position(&deco->controls_bg->node,
                                cx, DECO_FLOATING_MARGIN);

    /* 重新定位按钮 */
    int btn_right = DECO_CONTROLS_W - DECO_FLOATING_MARGIN;
    int btn_center_y = (DECO_TITLEBAR_H - DECO_BUTTON_H) / 2;

    wlr_scene_node_set_position(&deco->close_btn->node,
                               cx + btn_right - DECO_BUTTON_W, btn_center_y);
    wlr_scene_node_set_position(&deco->max_btn->node,
                               cx + btn_right - DECO_BUTTON_W - DECO_BUTTON_GAP - DECO_BUTTON_W,
                               btn_center_y);
    wlr_scene_node_set_position(&deco->min_btn->node,
                               cx + btn_right - 2 * (DECO_BUTTON_W + DECO_BUTTON_GAP) - DECO_BUTTON_W,
                               btn_center_y);
}

/* 销毁装饰 */
void deco_destroy(struct openos_deco *deco) {
    if (deco->tree) {
        wlr_scene_node_destroy(&deco->tree->node);
    }
}

/* 命中测试 */
int deco_hit_test(struct openos_deco *deco, double sx, double sy) {
    /* 是否在标题栏区域 */
    if (sy < 0 || sy > DECO_TITLEBAR_H + DECO_FLOATING_MARGIN * 2)
        return 0;

    int cx = deco->window_width - DECO_CONTROLS_W - DECO_FLOATING_MARGIN;

    /* 检查控制块按钮 (相对窗口坐标) */
    int btn_right = DECO_CONTROLS_W - DECO_FLOATING_MARGIN;
    int btn_center_y = (DECO_TITLEBAR_H - DECO_BUTTON_H) / 2 + DECO_FLOATING_MARGIN;

    /* 关闭按钮 */
    int cbx = cx + btn_right - DECO_BUTTON_W;
    if (sx >= cbx && sx < cbx + DECO_BUTTON_W &&
        sy >= btn_center_y && sy < btn_center_y + DECO_BUTTON_H) {
        return 1; /* 关闭 */
    }

    /* 最大化按钮 */
    int mbx = cx + btn_right - DECO_BUTTON_W - DECO_BUTTON_GAP - DECO_BUTTON_W;
    if (sx >= mbx && sx < mbx + DECO_BUTTON_W &&
        sy >= btn_center_y && sy < btn_center_y + DECO_BUTTON_H) {
        return 2; /* 最大化 */
    }

    /* 最小化按钮 */
    int mnbx = cx + btn_right - 2 * (DECO_BUTTON_W + DECO_BUTTON_GAP) - DECO_BUTTON_W;
    if (sx >= mnbx && sx < mnbx + DECO_BUTTON_W &&
        sy >= btn_center_y && sy < btn_center_y + DECO_BUTTON_H) {
        return 3; /* 最小化 */
    }

    /* 标题栏拖拽区 (左块) */
    if (sx >= DECO_FLOATING_MARGIN &&
        sx < DECO_FLOATING_MARGIN + DECO_TITLE_BAR_W &&
        sy >= DECO_FLOATING_MARGIN &&
        sy < DECO_FLOATING_MARGIN + DECO_TITLEBAR_H) {
        return 4;
    }

    /* 控制块拖拽区 (按钮之间的空白区域) */
    if (sx >= cx && sx < cx + DECO_CONTROLS_W &&
        sy >= DECO_FLOATING_MARGIN &&
        sy < DECO_FLOATING_MARGIN + DECO_TITLEBAR_H) {
        /* 在按钮之间的空白区域, 允许拖拽 */
        return 5;
    }

    return 0;
}

/* 按钮颜色更新 */
static void update_button_color(struct wlr_scene_rect *btn,
                                 bool hover, bool is_close) {
    float r, g, b, a;
    if (hover) {
        if (is_close) {
            colorf(OUI_ERROR, (float[]){0,0,0,0}, 0.85f);
            r = ((OUI_ERROR >> 16) & 0xFF) / 255.0f;
            g = ((OUI_ERROR >> 8) & 0xFF) / 255.0f;
            b = (OUI_ERROR & 0xFF) / 255.0f;
            a = 0.85f;
        } else {
            r = ((OUI_ON_SURFACE >> 16) & 0xFF) / 255.0f;
            g = ((OUI_ON_SURFACE >> 8) & 0xFF) / 255.0f;
            b = (OUI_ON_SURFACE & 0xFF) / 255.0f;
            a = 0.25f;
        }
    } else {
        if (is_close) {
            r = ((OUI_ERROR >> 16) & 0xFF) / 255.0f;
            g = ((OUI_ERROR >> 8) & 0xFF) / 255.0f;
            b = (OUI_ERROR & 0xFF) / 255.0f;
            a = 0.15f;
        } else {
            r = ((OUI_ON_SURFACE >> 16) & 0xFF) / 255.0f;
            g = ((OUI_ON_SURFACE >> 8) & 0xFF) / 255.0f;
            b = (OUI_ON_SURFACE & 0xFF) / 255.0f;
            a = 0.08f;
        }
    }
    float color[4] = {r, g, b, a};
    wlr_scene_rect_set_color(btn, color);
}

void deco_set_hover(struct openos_deco *deco, double sx, double sy) {
    deco_clear_hover(deco);

    int hit = deco_hit_test(deco, sx, sy);
    switch (hit) {
    case 1:
        deco->hover_close = true;
        update_button_color(deco->close_btn, true, true);
        break;
    case 2:
        deco->hover_max = true;
        update_button_color(deco->max_btn, true, false);
        break;
    case 3:
        deco->hover_min = true;
        update_button_color(deco->min_btn, true, false);
        break;
    default:
        break;
    }
}

void deco_clear_hover(struct openos_deco *deco) {
    if (deco->hover_close) {
        deco->hover_close = false;
        update_button_color(deco->close_btn, false, true);
    }
    if (deco->hover_max) {
        deco->hover_max = false;
        update_button_color(deco->max_btn, false, false);
    }
    if (deco->hover_min) {
        deco->hover_min = false;
        update_button_color(deco->min_btn, false, false);
    }
}

int deco_controls_x(struct openos_deco *deco, int width) {
    (void)deco;
    return width - DECO_CONTROLS_W - DECO_FLOATING_MARGIN;
}

int deco_titlebar_height(void) {
    return DECO_TITLEBAR_H + DECO_FLOATING_MARGIN * 2;
}