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
/* OPENOS 桌面外壳 — Wayland 协议桥
 *
 * 职责: 把 Qt 外壳连接到 wlroots 合成器, 驱动 ShellBackend 数据模型:
 *   - wlr-layer-shell (LayerShellQt): 面板/通知 作为 layer surface
 *   - wlr-foreign-toplevel-management (原生 libwayland-client): 任务栏窗口列表
 *   - openos-workspace (自研协议): 工作区切换器
 *
 * 事件循环集成: 用 QSocketNotifier 监听 wl_display fd, 接入 Qt 事件循环
 * (无需额外线程/阻塞 dispatch)。
 */
#include <QObject>
#include <QVector>

class ShellBackend;
class QWindow;

class WaylandBridge : public QObject {
    Q_OBJECT
public:
    explicit WaylandBridge(ShellBackend* backend, QObject* parent = nullptr);
    ~WaylandBridge() override;

    /* 连接合成器 (WAYLAND_DISPLAY), 绑定 globals, 开始接收事件 */
    bool init();

    /* 把 Qt 窗口注册为 layer-shell surface (面板/通知) */
    void setWindowLayerSurface(QWindow* window, const QString& ns,
                               const QString& layer, bool exclusive = true);

    /* 用户动作 (QML 调用 -> 协议请求) */
    Q_INVOKABLE void requestActivateWindow(int index);
    Q_INVOKABLE void requestCloseWindow(int index);
    Q_INVOKABLE void requestActivateWorkspace(int index);

    /* 供协议回调访问内部状态 (实现定义于 .cpp) */
    struct Impl;
    Impl* impl() const { return m_impl; }

signals:
    /* 数据变更 -> 由 ShellBackend 槽填充模型 */
    void windowAdded(const QString& title, const QString& appId, bool active);
    void windowUpdated(int index, const QString& title, const QString& appId,
                       bool active);
    void windowRemoved(int index);
    void workspaceAdded(const QString& name);
    void workspaceUpdated(int index, const QString& name);
    void workspaceActivated(int index, bool active);

private:
    ShellBackend* m_backend = nullptr;
    bool m_initialized = false;
    Impl* m_impl = nullptr;
};
