#ifndef OPENOS_OPENUI_H
#define OPENOS_OPENUI_H

/* OPENUI — OPENOS 统一界面设计令牌 (C 版)
 *
 * 在 NUI2 视觉基调 (深色 / 20 层叠层 / 文字符号 / 键盘优先) 之上,
 * 参考 Material Design 3 的完整设计体系补充:
 *   语义色彩 + 调色板 | 状态层叠 | 文字排印阶梯 | 形状阶梯 |
 *   动效时长与缓动 | 间距网格 | 对比度层级 | 触控目标
 * 供合成器与所有桌面客户端 (面板/通知/启动器) 共用。
 */

#include <stdint.h>

/* ================= 基础色板 ================= */
/* 中性灰 (NUI2 20 层表面色, 保留) */
#define OUI_NEUTRAL_0   0x090909u
#define OUI_NEUTRAL_10  0x141414u
#define OUI_NEUTRAL_20  0x1C1C1Cu
#define OUI_NEUTRAL_30  0x252525u
#define OUI_NEUTRAL_40  0x2D2D2Du
#define OUI_NEUTRAL_50  0x363636u
#define OUI_NEUTRAL_60  0x3F3F3Fu
#define OUI_NEUTRAL_70  0x484848u
#define OUI_NEUTRAL_80  0x515151u
#define OUI_NEUTRAL_90  0x5B5B5Bu
#define OUI_NEUTRAL_100 0x646464u
#define OUI_NEUTRAL_110 0x6E6E6Eu
#define OUI_NEUTRAL_120 0x787878u
#define OUI_NEUTRAL_130 0x828282u
#define OUI_NEUTRAL_140 0x8D8D8Du
#define OUI_NEUTRAL_150 0x979797u
#define OUI_NEUTRAL_160 0xA2A2A2u
#define OUI_NEUTRAL_170 0xACACACu
#define OUI_NEUTRAL_180 0xB7B7B7u
#define OUI_NEUTRAL_190 0xC2C2C2u
#define OUI_NEUTRAL_200 0xF5F5F5u

/* ================= 语义色彩 (MD3 风格: primary/secondary/tertiary/error) === */
#define OUI_PRIMARY         0x00BCD4u   /* 主色 = NUI2 强调色 Cyan */
#define OUI_ON_PRIMARY      0x001014u
#define OUI_PRIMARY_CONTAINER 0x006A7Au
#define OUI_ON_PRIMARY_CONTAINER 0xC9F5FFu
#define OUI_SECONDARY       0x6EB3C0u
#define OUI_ON_SECONDARY    0x001317u
#define OUI_SECONDARY_CONTAINER 0x3C6B75u
#define OUI_ON_SECONDARY_CONTAINER 0xD8F2F7u
#define OUI_TERTIARY        0x9FC85Fu   /* 强调色之二的浅绿 */
#define OUI_ON_TERTIARY     0x111A00u
#define OUI_ERROR           0xF44336u
#define OUI_ON_ERROR        0xFFFFFFu
#define OUI_ERROR_CONTAINER 0x8C1D18u
#define OUI_ON_ERROR_CONTAINER 0xFFDAD6u

/* 表面/背景 (deepest -> 最浅) */
#define OUI_BACKGROUND      0x090909u   /* 桌面根背景 (= surface-0) */
#define OUI_SURFACE         0x141414u   /* 默认表面 (= surface-1) */
#define OUI_SURFACE_DIM     0x0F0F0Fu
#define OUI_SURFACE_BRIGHT  0x1F1F1Fu
#define OUI_SURFACE_CONTAINER_HIGHEST 0x515151u

/* 文本 (MD3 对比度层级) */
#define OUI_ON_SURFACE        0xF5F5F5u  /* 主文本, 87% */
#define OUI_ON_SURFACE_VARIANT 0xB0B0B0u /* 次级文本, 60% */
#define OUI_ON_SURFACE_DISABLED 0x707070u /* 禁用/占位, 38% */
#define OUI_OUTLINE          0x8D8D8Du
#define OUI_OUTLINE_VARIANT  0x484848u

/* ================= 状态层叠 (MD3 state layers) =====
 * 状态叠加色: 在容器色之上叠加一层半透明 (通常为 on-surface),
 * 默认 alpha 参考 MD3: hover 8% / focus 12% / pressed 12% / dragged 16% */
#define OUI_STATE_HOVER    (uint8_t)(255 * 0.08)
#define OUI_STATE_FOCUS    (uint8_t)(255 * 0.12)
#define OUI_STATE_PRESSED  (uint8_t)(255 * 0.12)
#define OUI_STATE_DRAGGED  (uint8_t)(255 * 0.16)

/* ================= 文字排印阶梯 (MD3 type scale, px) ================= */
#define OUI_FONT_SANS   "Sans"        /* 实际应为 OPENOS 字体族; 此处占位 */
#define OUI_FONT_MONO   "Monospace"

typedef struct { int size; int line_height; int weight; } oui_type;
#define OUI_TYPE_DISPLAY_L   {57, 64, 400}
#define OUI_TYPE_DISPLAY_M   {45, 52, 400}
#define OUI_TYPE_DISPLAY_S   {36, 44, 400}
#define OUI_TYPE_HEADLINE_L  {32, 40, 400}
#define OUI_TYPE_HEADLINE_M  {28, 36, 400}
#define OUI_TYPE_HEADLINE_S  {24, 32, 400}
#define OUI_TYPE_TITLE_L     {22, 28, 400}
#define OUI_TYPE_TITLE_M     {16, 24, 500}
#define OUI_TYPE_TITLE_S     {14, 20, 500}
#define OUI_TYPE_BODY_L      {16, 24, 400}
#define OUI_TYPE_BODY_M      {14, 20, 400}
#define OUI_TYPE_BODY_S      {12, 16, 400}
#define OUI_TYPE_LABEL_L     {14, 20, 500}
#define OUI_TYPE_LABEL_M     {12, 16, 500}
#define OUI_TYPE_LABEL_S     {11, 16, 500}

/* ================= 形状阶梯 (圆角, px) =================
 * 圆角加大 (更柔和现代, 普通用户更习惯):
 * 基础组件 8, 卡片/菜单 12, 对话框 16, 大浮窗 24 */
#define OUI_SHAPE_NONE       0
#define OUI_SHAPE_XS         8    /* 按钮/输入/任务栏 */
#define OUI_SHAPE_SM         12   /* 卡片/菜单/通知 */
#define OUI_SHAPE_MD         16   /* 对话框 */
#define OUI_SHAPE_LG         24   /* 大卡片/浮窗 */
#define OUI_SHAPE_FULL       9999 /* 胶囊/指示器 */

/* 毛玻璃透明度 (表面 alpha 0-255; 越高越透明, 配合合成器动态模糊) */
#define OUI_GLASS_PANEL_ALPHA   (uint8_t)(255 * 0.72)
#define OUI_GLASS_CARD_ALPHA    (uint8_t)(255 * 0.75)
#define OUI_GLASS_MENU_ALPHA    (uint8_t)(255 * 0.78)

/* ================= 动效 (时长 ms + 缓动) ================= */
#define OUI_DUR_50     50
#define OUI_DUR_100    100
#define OUI_DUR_150    150
#define OUI_DUR_200    200
#define OUI_DUR_250    250
#define OUI_DUR_300    300
#define OUI_DUR_400    400
#define OUI_DUR_500    500

/* 缓动曲线 (cubic-bezier 控制点, 供动画插值参考) */
#define OUI_EASE_STANDARD     "0.2,0.0,0.0,1.0"
#define OUI_EASE_DECELERATED  "0.0,0.0,0.0,1.0"
#define OUI_EASE_ACCELERATING "0.3,0.0,1.0,1.0"
#define OUI_EASE_EMPHASIZED   "0.2,0.0,0.0,1.0"

/* ================= 间距网格 (4dp) ================= */
#define OUI_SP_1   4
#define OUI_SP_2   8
#define OUI_SP_3   12
#define OUI_SP_4   16
#define OUI_SP_5   20
#define OUI_SP_6   24
#define OUI_SP_8   32

/* ================= 触控/命中目标 ================= */
#define OUI_TOUCH_TARGET  48   /* 触控最小目标 (px) */
#define OUI_MIN_TARGET    24   /* 指针最小目标 (px) */
#define OUI_PANEL_HEIGHT  32   /* 顶部面板高度 */

/* ================= 工具: 颜色打包 (同 nui2.h, ARGB8888) ================= */
static inline uint32_t oui_argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
static inline uint32_t oui_token(uint32_t hex) {
    return oui_argb(0xFF, (hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
}
/* 在 hex 色上叠加 on-surface 状态层 (alpha), 返回新 ARGB8888 */
static inline uint32_t oui_state_overlay(uint32_t hex, uint8_t alpha) {
    uint32_t r = (hex >> 16) & 0xFF, g = (hex >> 8) & 0xFF, b = hex & 0xFF;
    uint32_t sr = (OUI_ON_SURFACE >> 16) & 0xFF;
    uint32_t sg = (OUI_ON_SURFACE >> 8) & 0xFF;
    uint32_t sb = OUI_ON_SURFACE & 0xFF;
    uint32_t a = alpha;
    r = (r * (255 - a) + sr * a) / 255;
    g = (g * (255 - a) + sg * a) / 255;
    b = (b * (255 - a) + sb * a) / 255;
    return oui_argb(0xFF, (uint8_t)r, (uint8_t)g, (uint8_t)b);
}
/* 文本对比度变体: 主/次/禁 (87% / 60% / 38%) */
static inline uint32_t oui_text(uint32_t hex, int level) {
    uint32_t r = (hex >> 16) & 0xFF, g = (hex >> 8) & 0xFF, b = hex & 0xFF;
    uint32_t bg = OUI_BACKGROUND;
    uint32_t br = (bg >> 16) & 0xFF, bg2 = (bg >> 8) & 0xFF, bb = bg & 0xFF;
    uint32_t a = level == 2 ? (uint32_t)(255 * 0.38)
              : level == 1 ? (uint32_t)(255 * 0.60)
                           : (uint32_t)(255 * 0.87);
    r = (r * a + br * (255 - a)) / 255;
    g = (g * a + bg2 * (255 - a)) / 255;
    b = (b * a + bb * (255 - a)) / 255;
    return oui_argb(0xFF, (uint8_t)r, (uint8_t)g, (uint8_t)b);
}

#endif /* OPENOS_OPENUI_H */
