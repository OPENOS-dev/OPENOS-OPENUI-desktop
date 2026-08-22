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

#pragma once
/* OAK 主循环集成 — Qt 事件循环 + wlroots 主循环 (C++)
 *
 * 集成方式 (避免线程锁死):
 *   单线程轮询:
 *     while (running) {
 *       wl_display_flush_clients(display);
 *       int n = wl_display_dispatch_pending(display);  // 处理 wlroots 事件
 *       QCoreApplication::processEvents();             // 处理 Qt 事件
 *       if (wl_display_dispatch_pending(display) == 0 && no qt events)
 *          wl_display_dispatch(display);               // 阻塞等待
 *     }
 *   或线程分离: wlroots 主循环在专门线程, Qt 事件循环在 GUI 线程, 用队列同步。
 *   此处给出单线程 + 可选 QThread 双模式框架。
 */

#include <functional>

class QCoreApplication;

/* 单线程事件循环 (推荐简单场景) */
void oak_run_single_loop(struct wl_display* display,
                         std::function<void()> perFrame,
                         std::function<bool()> shouldQuit);

/* 线程分离: wlroots 线程 + Qt 事件循环 (复杂场景) */
void oak_run_threaded(struct wl_display* display,
                      std::function<void()> perFrame,
                      QCoreApplication& app);
