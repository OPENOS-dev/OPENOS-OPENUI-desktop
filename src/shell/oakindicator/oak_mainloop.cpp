/* OAK 主循环集成实现 (见 oak_mainloop.h) */

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

#include "oak_mainloop.h"

#include <QCoreApplication>
#include <QThread>
#include <QTimer>
#include <wayland-server-core.h>
#include <atomic>

/* 单线程轮询: 同一线程交替处理 Qt 与 wlroots 事件 */
void oak_run_single_loop(struct wl_display* display,
                         std::function<void()> perFrame,
                         std::function<bool()> shouldQuit)
{
    while (!shouldQuit()) {
        /* 处理 wlroots 事件 */
        if (display)
            wl_display_flush_clients(display);

        /* 处理 Qt 事件 (含 QML 渲染请求、socket notifier、定时器) */
        QCoreApplication::processEvents(QEventLoop::AllEvents);

        /* 每帧回调 (wlr_output.frame 驱动的纹理上传/绘制逻辑由外部提供) */
        if (perFrame)
            perFrame();

        /* 若无待处理事件, 阻塞等待 wlroots (避免忙轮询) */
        if (display) {
            while (!shouldQuit() &&
                   !QCoreApplication::hasPendingEvents() &&
                   wl_display_dispatch_pending(display) == 0) {
                if (wl_display_dispatch(display) < 0)
                    break;
            }
        } else {
            QThread::msleep(5);
        }
    }
}

/* 线程分离: wlroots 事件循环在后台线程, Qt 事件循环在 GUI 线程。
 * perFrame 由 wlroots 线程回调; 通过信号/队列同步到 GUI 线程渲染 QML。 */
void oak_run_threaded(struct wl_display* display,
                      std::function<void()> perFrame,
                      QCoreApplication& app)
{
    std::atomic<bool> stop{ false };

    QThread wlThread;
    QObject ctx;

    /* 后台线程: 处理 wlroots 事件 + 帧回调 */
    QObject::connect(&wlThread, &QThread::started, &ctx, [&]() {
        while (!stop.load()) {
            if (display) {
                wl_display_flush_clients(display);
                int rc = wl_display_dispatch(display);   /* 阻塞等待事件 */
                if (rc < 0)
                    break;
            }
            /* perFrame 可触发 Qt 队列 (invokeMethod) 到 GUI 线程渲染 */
            if (perFrame)
                perFrame();
        }
        QCoreApplication::postEvent(&app, new QEvent(QEvent::Quit));
    });

    wlThread.start();
    app.exec();   /* GUI 线程跑 Qt 事件循环 */

    stop.store(true);
    wlThread.quit();
    wlThread.wait();
}
