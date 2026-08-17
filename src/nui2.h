#ifndef OPENOS_NUI2_H
#define OPENOS_NUI2_H

/* NUI2 (NOTHING UI 2) — OPENOS 默认界面设计规范, C 版设计令牌
 * 源自 OPENOS-DEV/NUI2: 深色 / 20 层叠层 / 动态模糊 / 文字符号(禁 Emoji) / 键盘优先
 * 这里把设计文档中的 CSS 变量翻译为 C 常量,供 Wayland 合成器与面板客户端共用。
 */

#include <stdint.h>

/* ---- 20 层表面色板 (L* 步长 4.0, 2.5 -> 78.5) ---- */
#define NUI_SURFACE_0  0x090909u
#define NUI_SURFACE_1  0x141414u
#define NUI_SURFACE_2  0x1C1C1Cu
#define NUI_SURFACE_3  0x252525u
#define NUI_SURFACE_4  0x2D2D2Du
#define NUI_SURFACE_5  0x363636u
#define NUI_SURFACE_6  0x3F3F3Fu
#define NUI_SURFACE_7  0x484848u
#define NUI_SURFACE_8  0x515151u
#define NUI_SURFACE_9  0x5B5B5Bu
#define NUI_SURFACE_10 0x646464u
#define NUI_SURFACE_11 0x6E6E6Eu
#define NUI_SURFACE_12 0x787878u
#define NUI_SURFACE_13 0x828282u
#define NUI_SURFACE_14 0x8D8D8Du
#define NUI_SURFACE_15 0x979797u
#define NUI_SURFACE_16 0xA2A2A2u
#define NUI_SURFACE_17 0xACACACu
#define NUI_SURFACE_18 0xB7B7B7u
#define NUI_SURFACE_19 0xC2C2C2u

/* ---- 强调色 (默认 Cyan) ---- */
#define NUI_ACCENT       0x00BCD4u
#define NUI_ACCENT_HOVER 0x26C6DAu
#define NUI_ACCENT_ACTIVE 0x0097A7u

/* ---- 文本色 ---- */
#define NUI_TEXT_PRIMARY   0xF5F5F5u
#define NUI_TEXT_SECONDARY 0xB0B0B0u
#define NUI_TEXT_TERTIARY  0x707070u
#define NUI_TEXT_ON_LIGHT       0x1A1A1Au
#define NUI_TEXT_ON_LIGHT_SECOND 0x4A4A4Au

/* ---- 状态色 ---- */
#define NUI_STATUS_SUCCESS 0x4AA26Fu
#define NUI_STATUS_WARNING 0xFF9800u
#define NUI_STATUS_ERROR   0xF44336u
#define NUI_STATUS_INFO    0x2196F3u

/* ---- 形状 (NUI2 极微圆角) ---- */
#define NUI_RADIUS_NONE 0
#define NUI_RADIUS_SM   2
#define NUI_RADIUS_MD   4
#define NUI_RADIUS_FULL 9999

/* ---- 文字符号图标 (严格禁止 Emoji, 仅用文本符号) ---- */
#define NUI_ICON_MENU    "☰"   /* U+2630 三横线 */
#define NUI_ICON_SEARCH  "⌕"   /* U+2315 搜索 */
#define NUI_ICON_SETTINGS "⚙"  /* U+2699 齿轮(文本变体) */
#define NUI_ICON_CLOSE   "×"   /* U+00D7 乘号 */
#define NUI_ICON_HOME    "⌂"   /* U+2302 房屋 */
#define NUI_ICON_SYSTEM  "⚛"   /* U+269B 原子 */
#define NUI_ICON_ARROW_R "→"   /* U+2192 */
#define NUI_ICON_CHECK   "✓"   /* U+2713 */
#define NUI_ICON_CROSS   "✕"   /* U+2715 */

/* ---- 颜色打包: wl_shm ARGB8888 与 cairo ARGB32 均为小端 B,G,R,A ----
 * 32 位值 = (A<<24)|(R<<16)|(G<<8)|B */
static inline uint32_t nui_argb(uint8_t a, uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)a << 24) | ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}
static inline uint32_t nui_rgb(uint8_t r, uint8_t g, uint8_t b) {
    return nui_argb(0xFF, r, g, b);
}
/* 由 0xRRGGBB 令牌生成不透明 ARGB32 */
static inline uint32_t nui_token(uint32_t hex) {
    return nui_argb(0xFF, (hex >> 16) & 0xFF, (hex >> 8) & 0xFF, hex & 0xFF);
}
/* 半透明白叠加 (NUI2 毛玻璃层), 供客户端估算背景色 */
static inline uint32_t nui_glass(uint8_t alpha) {
    return nui_argb(alpha, 0xFF, 0xFF, 0xFF);
}

#endif /* OPENOS_NUI2_H */
