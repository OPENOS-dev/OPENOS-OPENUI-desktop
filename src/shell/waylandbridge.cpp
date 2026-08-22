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

/* OPENOS 桌面外壳 — Wayland 协议桥实现
 *
 * 数据协议 (任务栏/工作区) 用原生 libwayland-client:
 *   - zwlr_foreign_toplevel_manager_v1 : 窗口列表 (bind 时合成器推送全部已开窗口)
 *   - openos_workspace_manager_v1       : 自研工作区协议
 * 面板 layer 层用 LayerShellQt (KDE) — 见 setWindowLayerSurface()。
 *
 * 事件循环: QSocketNotifier 监听 wl_display fd, 接入 Qt 事件循环。
 * 注意: 需在 OPENOS/Linux 上编译 (依赖 wayland-client + 生成协议头)。
 */

#include "waylandbridge.h"
#include "shellbackend.h"

#include <QDebug>
#include <QSocketNotifier>
#include <QWindow>
#include <QtGlobal>
#include <wayland-client.h>
#include "wlr-foreign-toplevel-management-unstable-v1-client-protocol.h"
#include "openos-workspace-unstable-v1-client-protocol.h"
#ifdef OPENOS_USE_LAYERSHELLQT
#include <LayerShellQt/Shell>
#include <LayerShellQt/LayerShellQt>
#endif

/* ---- 协议对象状态 ---- */
struct WaylandBridge::Impl {
    wl_display* display = nullptr;
    wl_registry* registry = nullptr;
    zwlr_foreign_toplevel_manager_v1* ftMgr = nullptr;
    openos_workspace_manager_v1* wsMgr = nullptr;
    wl_seat* seat = nullptr;
    QSocketNotifier* notifier = nullptr;

    /* 模型索引 = QVector 顺序 */
    QVector<zwlr_foreign_toplevel_handle_v1*> wins;
    QVector<openos_workspace_handle_v1*> wss;
};

/* ================= 回调 =================
 * 所有回调的 data = WaylandBridge* (add_listener 时传入)
 */
static WaylandBridge* bridge_from_data(void* data) {
    return static_cast<WaylandBridge*>(data);
}
static int index_of(const QVector<zwlr_foreign_toplevel_handle_v1*>& v,
                    zwlr_foreign_toplevel_handle_v1* h) {
    return v.indexOf(h);
}
static int index_of(const QVector<openos_workspace_handle_v1*>& v,
                    openos_workspace_handle_v1* h) {
    return v.indexOf(h);
}

/* ---- foreign-toplevel handle ---- */
static void ft_title(void* data, zwlr_foreign_toplevel_handle_v1* h, const char* t) {
    auto* b = bridge_from_data(data);
    int i = index_of(b->impl()->wins, h);
    if (i >= 0) emit b->windowUpdated(i, QString::fromUtf8(t), QString(), false);
}
static void ft_app_id(void* data, zwlr_foreign_toplevel_handle_v1* h, const char* a) {
    auto* b = bridge_from_data(data);
    int i = index_of(b->impl()->wins, h);
    if (i >= 0) emit b->windowUpdated(i, QString(), QString::fromUtf8(a), false);
}
static void ft_state(void* data, zwlr_foreign_toplevel_handle_v1* h,
                     struct wl_array* state) {
    auto* b = bridge_from_data(data);
    int i = index_of(b->impl()->wins, h);
    if (i < 0) return;
    bool active = false;
    uint32_t* s;
    wl_array_for_each(s, state)
        if (*s == ZWLR_FOREIGN_TOPLEVEL_HANDLE_V1_STATE_ACTIVATED) active = true;
    emit b->windowUpdated(i, QString(), QString(), active);
}
static void ft_done(void* data, zwlr_foreign_toplevel_handle_v1* h) {
    Q_UNUSED(data); Q_UNUSED(h);
}
static void ft_closed(void* data, zwlr_foreign_toplevel_handle_v1* h) {
    auto* b = bridge_from_data(data);
    auto* m = b->impl();
    int i = index_of(m->wins, h);
    if (i < 0) return;
    zwlr_foreign_toplevel_handle_v1_destroy(h);
    m->wins.removeAt(i);
    emit b->windowRemoved(i);
}
static void ft_parent(void* data, zwlr_foreign_toplevel_handle_v1* h,
                      zwlr_foreign_toplevel_handle_v1* p) {
    Q_UNUSED(data); Q_UNUSED(h); Q_UNUSED(p);
}
static const struct zwlr_foreign_toplevel_handle_v1_listener ft_handle_listener = {
    .title = ft_title,
    .app_id = ft_app_id,
    .output_enter = nullptr,
    .output_leave = nullptr,
    .state = ft_state,
    .done = ft_done,
    .closed = ft_closed,
    .parent = ft_parent,
};

static void ft_toplevel(void* data, zwlr_foreign_toplevel_manager_v1* m,
                        zwlr_foreign_toplevel_handle_v1* h) {
    Q_UNUSED(m);
    auto* b = bridge_from_data(data);
    b->impl()->wins.append(h);
    zwlr_foreign_toplevel_handle_v1_add_listener(h, &ft_handle_listener, data);
    emit b->windowAdded(QString(), QString(), false);
}
static void ft_finished(void* data, zwlr_foreign_toplevel_manager_v1* m) {
    Q_UNUSED(data); Q_UNUSED(m);
}
static const struct zwlr_foreign_toplevel_manager_v1_listener ft_mgr_listener = {
    .toplevel = ft_toplevel,
    .finished = ft_finished,
};

/* ---- 自研 workspace ---- */
static void ws_name(void* data, openos_workspace_handle_v1* h, const char* n) {
    auto* b = bridge_from_data(data);
    int i = index_of(b->impl()->wss, h);
    if (i >= 0) emit b->workspaceUpdated(i, QString::fromUtf8(n));
}
static void ws_state(void* data, openos_workspace_handle_v1* h,
                     struct wl_array* state) {
    auto* b = bridge_from_data(data);
    int i = index_of(b->impl()->wss, h);
    if (i < 0) return;
    bool active = false;
    uint32_t* s;
    wl_array_for_each(s, state)
        if (*s == OPENOS_WORKSPACE_HANDLE_V1_STATE_ACTIVE) active = true;
    emit b->workspaceActivated(i, active);
}
static void ws_done(void* data, openos_workspace_handle_v1* h) {
    Q_UNUSED(data); Q_UNUSED(h);
}
static const struct openos_workspace_handle_v1_listener ws_handle_listener = {
    .name = ws_name,
    .state = ws_state,
    .done = ws_done,
};

static void ws_workspace(void* data, openos_workspace_manager_v1* m,
                         openos_workspace_handle_v1* h) {
    Q_UNUSED(m);
    auto* b = bridge_from_data(data);
    b->impl()->wss.append(h);
    openos_workspace_handle_v1_add_listener(h, &ws_handle_listener, data);
    emit b->workspaceAdded(QString());
}
static void ws_finished(void* data, openos_workspace_manager_v1* m) {
    Q_UNUSED(data); Q_UNUSED(m);
}
static const struct openos_workspace_manager_v1_listener ws_mgr_listener = {
    .workspace = ws_workspace,
    .finished = ws_finished,
};

/* ---- registry ---- */
static void registry_global(void* data, wl_registry* reg, uint32_t name,
                            const char* iface, uint32_t ver) {
    auto* b = bridge_from_data(data);
    auto* m = b->impl();
    if (strcmp(iface, zwlr_foreign_toplevel_manager_v1_interface.name) == 0) {
        m->ftMgr = static_cast<zwlr_foreign_toplevel_manager_v1*>(
            wl_registry_bind(reg, name, &zwlr_foreign_toplevel_manager_v1_interface,
                             qMin(3u, ver)));
        zwlr_foreign_toplevel_manager_v1_add_listener(m->ftMgr, &ft_mgr_listener, data);
    } else if (strcmp(iface, openos_workspace_manager_v1_interface.name) == 0) {
        m->wsMgr = static_cast<openos_workspace_manager_v1*>(
            wl_registry_bind(reg, name, &openos_workspace_manager_v1_interface,
                             qMin(1u, ver)));
        openos_workspace_manager_v1_add_listener(m->wsMgr, &ws_mgr_listener, data);
    } else if (strcmp(iface, wl_seat_interface.name) == 0 && !m->seat) {
        m->seat = static_cast<wl_seat*>(
            wl_registry_bind(reg, name, &wl_seat_interface, qMin(1u, ver)));
    }
}
static void registry_remove(void* data, wl_registry* reg, uint32_t name) {
    Q_UNUSED(data); Q_UNUSED(reg); Q_UNUSED(name);
}
static const struct wl_registry_listener registry_listener = {
    .global = registry_global,
    .global_remove = registry_remove,
};

/* ================= 实现 ================= */

WaylandBridge::WaylandBridge(ShellBackend* backend, QObject* parent)
    : QObject(parent), m_backend(backend), m_impl(new Impl) {}

WaylandBridge::~WaylandBridge() {
    if (m_impl) {
        if (m_impl->notifier) delete m_impl->notifier;
        if (m_impl->display) wl_display_disconnect(m_impl->display);
        delete m_impl;
    }
}

/* 处理 wayland fd 可读: 读取事件并分发 */
static void on_display_readable(int fd, WaylandBridge* b) {
    Q_UNUSED(fd);
    auto* m = b->impl();
    if (!m->display) return;
    while (wl_display_prepare_read(m->display) != 0)
        wl_display_dispatch_pending(m->display);
    wl_display_flush(m->display);
    if (wl_display_read_events(m->display) == -1) {
        qWarning() << "WaylandBridge: 合成器连接断开";
        return;
    }
    wl_display_dispatch_pending(m->display);
}

bool WaylandBridge::init() {
    m_impl->display = wl_display_connect(nullptr);
    if (!m_impl->display) {
        qWarning() << "WaylandBridge: 无法连接 WAYLAND_DISPLAY, 外壳以独立窗口运行";
        return false;
    }

    m_impl->registry = wl_display_get_registry(m_impl->display);
    wl_registry_add_listener(m_impl->registry, &registry_listener, this);
    wl_display_roundtrip(m_impl->display);   // 触发 global + 初始 toplevel/workspace 事件
    wl_display_roundtrip(m_impl->display);

    /* 协议事件 -> 数据模型 */
    connect(this, &WaylandBridge::windowAdded, m_backend, &ShellBackend::addWindow);
    connect(this, &WaylandBridge::windowUpdated, m_backend,
            &ShellBackend::updateWindow);
    connect(this, &WaylandBridge::windowRemoved, m_backend,
            &ShellBackend::removeWindow);
    connect(this, &WaylandBridge::workspaceAdded, m_backend,
            &ShellBackend::addWorkspace);
    connect(this, &WaylandBridge::workspaceUpdated, m_backend,
            &ShellBackend::setWorkspaceName);
    connect(this, &WaylandBridge::workspaceActivated, m_backend,
            &ShellBackend::setWorkspaceActive);

    m_impl->notifier = new QSocketNotifier(wl_display_get_fd(m_impl->display),
                                           QSocketNotifier::Read, this);
    connect(m_impl->notifier, &QSocketNotifier::activated, this,
            [this](int fd) { on_display_readable(fd, this); });

    m_initialized = true;
    qInfo() << "WaylandBridge: 已连接合成器"
            << " windows=" << m_impl->wins.size()
            << " workspaces=" << m_impl->wss.size();
    return true;
}

void WaylandBridge::setWindowLayerSurface(QWindow* window, const QString& ns,
                                          const QString& layer, bool exclusive) {
#ifdef OPENOS_USE_LAYERSHELLQT
    using namespace LayerShellQt;
    if (!window) return;
    Shell::useLayerShell();
    LayerSurface* ls = Shell::getLayerSurface(window);
    if (!ls) {
        qWarning() << "WaylandBridge: LayerShellQt 不可用, 保持普通窗口";
        return;
    }
    ls->setScope(ns);
    ls->setLayer(layer == QLatin1String("overlay") ? Shell::LayerOverlay
                                                   : Shell::LayerTop);
    uint32_t anchors = LayerSurface::AnchorTop | LayerSurface::AnchorLeft;
    if (ns == QLatin1String("openos-panel"))
        anchors |= LayerSurface::AnchorRight;
    ls->setAnchors(anchors);
    if (exclusive) ls->setExclusiveZone(32);
    ls->setSize(QSize(0, 32));
    qInfo() << "WaylandBridge: layer-surface 面板已注册:" << ns;
#else
    Q_UNUSED(window); Q_UNUSED(ns); Q_UNUSED(layer); Q_UNUSED(exclusive);
    qWarning() << "WaylandBridge: 未启用 LayerShellQt (OPENOS_USE_LAYERSHELLQT), 面板保持普通窗口";
#endif
}

void WaylandBridge::requestActivateWindow(int index) {
    auto* m = m_impl;
    if (index < 0 || index >= m->wins.size()) return;
    if (m->seat && m->wins[index])
        zwlr_foreign_toplevel_handle_v1_activate(m->wins[index], m->seat);
}
void WaylandBridge::requestCloseWindow(int index) {
    auto* m = m_impl;
    if (index < 0 || index >= m->wins.size()) return;
    if (m->wins[index])
        zwlr_foreign_toplevel_handle_v1_close(m->wins[index]);
}
void WaylandBridge::requestActivateWorkspace(int index) {
    auto* m = m_impl;
    if (index < 0 || index >= m->wss.size()) return;
    if (m->wss[index])
        openos_workspace_handle_v1_activate(m->wss[index]);
}
