#ifndef OPENOS_BLUR_H
#define OPENOS_BLUR_H

#include <wlr/render/allocator.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/types/wlr_scene.h>

/* OPENOS 动态模糊 (NUI2 核心视觉) — 两阶段渲染管线
 *
 * 目标效果: 面板/菜单/通知 等浮动层下方的桌面内容被高斯模糊 (毛玻璃),
 * 浮动层自身仍清晰绘制在其上。
 *
 * 实现原理 (基于 wlroots 0.17 公开 API):
 *   1) 临时禁用 blur_tree, 用 wlr_scene_output_build_state() 渲染"清晰场景"
 *      到独立 buffer (不提交, 无闪烁)
 *   2) wlr_buffer_begin_data_ptr_access() 读回像素, 对 blur 包围盒做 CPU 高斯模糊
 *   3) wlr_texture_from_pixels() 生成模糊纹理
 *   4) 第二遍渲染: 清晰场景纹理铺满全屏 -> 模糊纹理盖住 blur 盒 ->
 *      再把 blur_tree 下各层表面(面板/通知)的纹理画回原位, 一次性提交
 *
 * 说明:
 *   - 模糊是 best-effort: 任何一步失败都返回 false, 调用方走普通
 *     wlr_scene_output_commit() 快速路径, 桌面功能不受影响。
 *   - 依赖 wlroots 0.17 (wlr_scene_output_build_state)。macOS 无 wlroots,
 *     需在 OPENOS(Linux) 上构建验证。
 */

#define OPENOS_BLUR_RADIUS 8  /* 高斯模糊半径 (px) */

/* 执行带动态模糊的渲染并提交输出。
 * blur_tree 下无可见内容时返回 false (由调用方走常规提交)。
 * 返回 true 表示本函数已提交输出 (场景含模糊效果)。
 */
bool openos_blur_render(struct wlr_renderer *renderer,
                        struct wlr_allocator *allocator,
                        struct wlr_output *output,
                        struct wlr_scene_output *scene_output,
                        struct wlr_scene *scene,
                        struct wlr_scene_tree *blur_tree,
                        const float bg_color[4]);

#endif /* OPENOS_BLUR_H */
