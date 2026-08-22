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