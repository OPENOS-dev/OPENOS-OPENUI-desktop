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

#include "gestures.h"
#include "compositor.h"
#include <wlr/types/wlr_pointer_gestures_v1.h>
#include <wlr/util/log.h>

/* 初始化手势监听 */
void gestures_init(struct openos_server *server) {
    /* 手势监听器通过 on_new_input 在键盘/指针设备创建时注册。
     * 实际手势事件由 libinput 通过 wlr_pointer_gesture 协议上报,
     * 在 compositor.c 的 on_new_input 中, 指针设备类型注册后,
     * 通过 wlr_pointer 的 gesture 事件接收。
     *
     * wlroots 0.17 中手势事件通过 wlr_pointer 的 events 发出:
     *   - wlr_pointer.events.gesture_swipe_begin
     *   - wlr_pointer.events.gesture_swipe_update
     *   - wlr_pointer.events.gesture_swipe_end
     * 这些在 compositor.c 的 on_new_input 指针分支中连接。
     *
     * 本文件提供事件处理函数实现。
     */
    (void)server;
}

/* 三指滑动开始 */
void handle_gesture_swipe_begin(struct wl_listener *listener, void *data) {
    struct openos_server *server =
        wl_container_of(listener, server, gesture_swipe_begin);
    struct wlr_pointer_swipe_begin_event *event = data;

    server->gesture_swipe_fingers = event->fingers;
    server->gesture_swipe_dx = 0;
    server->gesture_swipe_dy = 0;

    wlr_log(WLR_DEBUG, "gesture: swipe begin (%d fingers)",
            event->fingers);
}

/* 三指滑动更新 */
void handle_gesture_swipe_update(struct wl_listener *listener, void *data) {
    struct openos_server *server =
        wl_container_of(listener, server, gesture_swipe_update);
    struct wlr_pointer_swipe_update_event *event = data;

    server->gesture_swipe_dx += event->dx;
    server->gesture_swipe_dy += event->dy;
}

/* 三指滑动结束 */
void handle_gesture_swipe_end(struct wl_listener *listener, void *data) {
    (void)data;
    struct openos_server *server =
        wl_container_of(listener, server, gesture_swipe_end);

    /* 三指左右滑动: 切换工作区 */
    if (server->gesture_swipe_fingers == 3) {
        double threshold = 100.0; /* px 阈值 */
        if (server->gesture_swipe_dx > threshold) {
            /* 向右滑 -> 上一工作区 */
            int prev = (server->current_workspace - 1 + WORKSPACE_COUNT)
                       % WORKSPACE_COUNT;
            switch_workspace(server, prev);
        } else if (server->gesture_swipe_dx < -threshold) {
            /* 向左滑 -> 下一工作区 */
            int next = (server->current_workspace + 1) % WORKSPACE_COUNT;
            switch_workspace(server, next);
        }
        /* 三指上下滑动: 暂留 (可用于打开概览视图) */
    }

    server->gesture_swipe_fingers = 0;
    server->gesture_swipe_dx = 0;
    server->gesture_swipe_dy = 0;
}