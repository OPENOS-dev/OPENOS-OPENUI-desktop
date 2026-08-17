#define _POSIX_C_SOURCE 200809L
/* OPENOS 动态模糊 (NUI2 毛玻璃) 实现
 * 两阶段管线: 渲染清晰场景 -> 读回像素 -> CPU 高斯模糊 -> 二次合成提交。
 * 依赖 wlroots 0.17 公开 API (wlr_scene_output_build_state 等)。
 * best-effort: 失败自动回退普通提交。
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <wayland-server-core.h>
#include <wlr/render/allocator.h>
#include <wlr/render/wlr_render_pass.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/types/wlr_buffer.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_scene.h>
#include <wlr/util/log.h>
#include "blur.h"

/* ---- CPU 高斯模糊 (分离的两趟盒式模糊, 近似高斯) ----
 * 每次对给定区域做一次水平 + 一次垂直滑窗均值。
 */
static void blur_axis(uint32_t *px, int w, int h, int stride_px,
                      int radius, int axis) {
    size_t n = (size_t)w * h;
    uint32_t *out = malloc(n * sizeof(uint32_t));
    if (!out) return;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int rs = 0, gs = 0, bs = 0, as = 0, cnt = 0;
            for (int k = -radius; k <= radius; k++) {
                int xx = axis == 0 ? x + k : x;
                int yy = axis == 0 ? y : y + k;
                if (xx < 0 || xx >= w || yy < 0 || yy >= h) continue;
                uint32_t p = px[(size_t)yy * stride_px + xx];
                as += (p >> 24) & 0xFF;
                rs += (p >> 16) & 0xFF;
                gs += (p >> 8) & 0xFF;
                bs += p & 0xFF;
                cnt++;
            }
            if (cnt == 0) {
                out[(size_t)y * w + x] = px[(size_t)y * stride_px + x];
                continue;
            }
            out[(size_t)y * w + x] =
                ((uint32_t)(as / cnt) << 24) |
                ((uint32_t)(rs / cnt) << 16) |
                ((uint32_t)(gs / cnt) << 8) |
                (uint32_t)(bs / cnt);
        }
    }
    for (size_t i = 0; i < n; i++) {
        int x = (int)(i % w), y = (int)(i / w);
        px[(size_t)y * stride_px + x] = out[i];
    }
    free(out);
}

/* 对 buffer 中 box 区域做模糊 (ARGB8888, 小端)。 */
static void blur_region(void *data, size_t stride, const struct wlr_box *box) {
    int w = box->width, h = box->height;
    if (w <= 0 || h <= 0) return;
    uint32_t *base = data;
    int stride_px = (int)(stride / 4);
    uint32_t *region = base + (size_t)box->y * stride_px + box->x;
    /* 两次水平 + 两次垂直, 半径 8, 得到更强的柔化 */
    for (int i = 0; i < 2; i++) {
        blur_axis(region, w, h, stride_px, OPENOS_BLUR_RADIUS, 0);
        blur_axis(region, w, h, stride_px, OPENOS_BLUR_RADIUS, 1);
    }
}

/* ---- 包围盒收集 ---- */
static void box_union(struct wlr_box *b, int x, int y, int w, int h) {
    if (b->width <= 0 || b->height <= 0) {
        b->x = x; b->y = y; b->width = w; b->height = h;
        return;
    }
    int x2 = b->x + b->width, y2 = b->y + b->height;
    int nx = x < b->x ? x : b->x;
    int ny = y < b->y ? y : b->y;
    int nx2 = (x + w) > x2 ? (x + w) : x2;
    int ny2 = (y + h) > y2 ? (y + h) : y2;
    b->x = nx; b->y = ny; b->width = nx2 - nx; b->height = ny2 - ny;
}

static void collect_node_box(struct wlr_scene_node *n, struct wlr_box *box) {
    if (!n->enabled) return;
    switch (n->type) {
    case WLR_SCENE_NODE_BUFFER: {
        struct wlr_scene_buffer *sb = wlr_scene_buffer_from_node(n);
        if (!sb->buffer) return;
        int x = 0, y = 0;
        if (!wlr_scene_node_coords(n, &x, &y)) return;
        int w = sb->dst_width > 0 ? sb->dst_width : sb->buffer->width;
        int h = sb->dst_height > 0 ? sb->dst_height : sb->buffer->height;
        if (w > 0 && h > 0) box_union(box, x, y, w, h);
        break;
    }
    case WLR_SCENE_NODE_TREE: {
        struct wlr_scene_tree *t = wlr_scene_tree_from_node(n);
        struct wlr_scene_node *c;
        wl_list_for_each(c, &t->children, link)
            collect_node_box(c, box);
        break;
    }
    default:
        break; /* rect 等非表面节点不参与模糊盒 */
    }
}

/* ---- 把 blur_tree 内容 (面板/通知表面) 画进第二遍 pass ---- */
static void render_node(struct wlr_renderer *renderer,
                        struct wlr_render_pass *pass,
                        struct wlr_scene_node *n) {
    if (!n->enabled) return;
    int x = 0, y = 0;
    if (!wlr_scene_node_coords(n, &x, &y)) return;
    switch (n->type) {
    case WLR_SCENE_NODE_BUFFER: {
        struct wlr_scene_buffer *sb = wlr_scene_buffer_from_node(n);
        if (!sb->buffer) return;
        struct wlr_texture *tex = wlr_texture_from_wlr_buffer(renderer, sb->buffer);
        if (!tex) return;
        struct wlr_box pos = { x, y, sb->buffer->width, sb->buffer->height };
        wlr_render_pass_add_texture(pass, tex, &pos, NULL,
                                    WL_OUTPUT_TRANSFORM_NORMAL, 1.0f);
        wlr_texture_destroy(tex);
        break;
    }
    case WLR_SCENE_NODE_TREE: {
        struct wlr_scene_tree *t = wlr_scene_tree_from_node(n);
        struct wlr_scene_node *c;
        wl_list_for_each(c, &t->children, link)
            render_node(renderer, pass, c);
        break;
    }
    default:
        break;
    }
}

bool openos_blur_render(struct wlr_renderer *renderer,
                        struct wlr_allocator *allocator,
                        struct wlr_output *output,
                        struct wlr_scene_output *scene_output,
                        struct wlr_scene *scene,
                        struct wlr_scene_tree *blur_tree,
                        const float bg_color[4]) {
    (void)scene;

    /* 1. 计算模糊盒, 裁剪到输出范围 */
    struct wlr_box box = {0};
    collect_node_box(&blur_tree->node, &box);
    if (box.width <= 0 || box.height <= 0) return false;
    if (box.x < 0) { box.width += box.x; box.x = 0; }
    if (box.y < 0) { box.height += box.y; box.y = 0; }
    if (box.x + box.width > output->width)
        box.width = output->width - box.x;
    if (box.y + box.height > output->height)
        box.height = output->height - box.y;
    if (box.width <= 0 || box.height <= 0) return false;

    /* 2. 第一遍: 禁用 blur_tree, 渲染清晰场景到独立 buffer (不提交) */
    wlr_scene_node_set_enabled(&blur_tree->node, false);
    struct wlr_output_state state;
    wlr_output_state_init(&state, output);
    bool ok = wlr_scene_output_build_state(scene_output, &state, NULL);
    wlr_scene_node_set_enabled(&blur_tree->node, true);
    if (!ok) {
        wlr_log(WLR_ERROR, "blur: wlr_scene_output_build_state 失败, 回退普通渲染");
        wlr_output_state_finish(&state);
        return false;
    }
    struct wlr_buffer *buf = state.buffer;
    if (!buf) {
        wlr_output_state_finish(&state);
        return false;
    }

    /* 3. 读回像素 -> CPU 模糊 -> 生成纹理 */
    void *data = NULL;
    uint32_t fmt = 0;
    size_t stride = 0;
    if (!wlr_buffer_begin_data_ptr_access(buf, &data, &fmt, &stride)) {
        wlr_log(WLR_ERROR, "blur: 无法访问 buffer 像素, 回退普通渲染");
        wlr_output_state_finish(&state);
        return false;
    }
    struct wlr_texture *scene_tex = wlr_texture_from_wlr_buffer(renderer, buf);
    blur_region(data, stride, &box);
    struct wlr_texture *blur_tex = wlr_texture_from_pixels(
        renderer, fmt, (uint32_t)stride, (uint32_t)box.width,
        (uint32_t)box.height,
        (uint8_t *)data + (size_t)box.y * stride + (size_t)box.x * 4);
    wlr_buffer_end_data_ptr_access(buf);
    wlr_output_state_finish(&state);

    if (!scene_tex || !blur_tex) {
        wlr_log(WLR_ERROR, "blur: 纹理创建失败, 回退普通渲染");
        if (scene_tex) wlr_texture_destroy(scene_tex);
        if (blur_tex) wlr_texture_destroy(blur_tex);
        return false;
    }

    /* 4. 第二遍: 合成并一次性提交 */
    struct wlr_render_pass *pass =
        wlr_output_begin_render_pass(output, allocator, bg_color);
    if (!pass) {
        wlr_texture_destroy(scene_tex);
        wlr_texture_destroy(blur_tex);
        return false;
    }
    struct wlr_box full = { 0, 0, output->width, output->height };
    wlr_render_pass_add_texture(pass, scene_tex, &full, NULL,
                                WL_OUTPUT_TRANSFORM_NORMAL, 1.0f);
    wlr_render_pass_add_texture(pass, blur_tex, &box, NULL,
                                WL_OUTPUT_TRANSFORM_NORMAL, 1.0f);
    render_node(renderer, pass, &blur_tree->node);
    wlr_render_pass_submit(pass);

    bool committed = wlr_output_commit(output);
    wlr_texture_destroy(scene_tex);
    wlr_texture_destroy(blur_tex);
    if (!committed)
        wlr_log(WLR_ERROR, "blur: wlr_output_commit 失败");
    return committed;
}
