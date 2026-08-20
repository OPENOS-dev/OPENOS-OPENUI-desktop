#ifndef OPENOS_GESTURES_H
#define OPENOS_GESTURES_H

/* OPENOS 触摸板手势支持
 * 三指左右滑动: 切换工作区
 * 三指上下滑动: 打开/关闭概览
 * 双指缩放: 暂留
 */

#include <wayland-server-core.h>
#include <wlr/types/wlr_input_device.h>
#include "compositor.h"

/* 初始化手势监听 */
void gestures_init(struct openos_server *server);

/* 手势事件处理 */
void handle_gesture_swipe_begin(struct wl_listener *listener, void *data);
void handle_gesture_swipe_update(struct wl_listener *listener, void *data);
void handle_gesture_swipe_end(struct wl_listener *listener, void *data);

#endif /* OPENOS_GESTURES_H */