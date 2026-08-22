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
/* OPENOS 桌面外壳 — QML 后端 (ShellBackend)
 * 提供 QML 所需的数据模型与操作:
 *   windows(任务栏窗口列表) / workspaces(工作区) / notifications(通知)
 *   launchApp / activateWorkspace / showNotification ...
 * 数据由 WaylandBridge (foreign-toplevel / workspace 协议) 驱动填充。
 */
#include <QAbstractListModel>
#include <QObject>
#include <QStringList>
#include <QVector>

struct ToplevelInfo { QString title; QString appId; bool active; };
struct AppInfo { QString name; QString exec; QString vmapp; QString source; QString icon; QString group; };
/* 应用抽屉条目:
 *   source = "system"  -> pacman / 系统安装 (宿主 /usr/share/applications)
 *   source = "vmapp"   -> 隔离环境 (/vmapp/<app>/usr/share/applications)
 */
struct WorkspaceInfo { QString name; bool active; };
struct NotificationAction { QString id; QString label; };
struct NotificationInfo {
    QString title;
    QString body;
    QString appId;
    qint64 timestamp;
    QVector<NotificationAction> actions;
};

class ToplevelModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role { TitleRole = Qt::UserRole + 1, AppIdRole, ActiveRole };
    int rowCount(const QModelIndex&) const override { return m_items.size(); }
    QVariant data(const QModelIndex& idx, int role) const override {
        if (idx.row() < 0 || idx.row() >= m_items.size()) return {};
        const ToplevelInfo& t = m_items[idx.row()];
        switch (role) {
        case TitleRole:  return t.title;
        case AppIdRole:  return t.appId;
        case ActiveRole: return t.active;
        }
        return {};
    }
    QHash<int, QByteArray> roleNames() const override {
        return { {TitleRole, "title"}, {AppIdRole, "appId"}, {ActiveRole, "active"} };
    }
    void add(const ToplevelInfo& t) {
        beginInsertRows({}, m_items.size(), m_items.size());
        m_items.append(t); endInsertRows();
    }
    void removeAt(int i) {
        if (i < 0 || i >= m_items.size()) return;
        beginRemoveRows({}, i, i); m_items.removeAt(i); endRemoveRows();
    }
    void setActive(int i, bool a) {
        if (i < 0 || i >= m_items.size()) return;
        m_items[i].active = a;
        emit dataChanged(index(i), index(i), {ActiveRole});
    }
    void update(int i, const QString& title, const QString& appId, bool active) {
        if (i < 0 || i >= m_items.size()) return;
        ToplevelInfo& t = m_items[i];
        if (!title.isNull()) t.title = title;
        if (!appId.isNull()) t.appId = appId;
        t.active = active;
        emit dataChanged(index(i), index(i), {TitleRole, AppIdRole, ActiveRole});
    }
    int count() const { return m_items.size(); }
private:
    QVector<ToplevelInfo> m_items;
};

class WorkspaceModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role { NameRole = Qt::UserRole + 1, ActiveRole };
    int rowCount(const QModelIndex&) const override { return m_items.size(); }
    QVariant data(const QModelIndex& idx, int role) const override {
        if (idx.row() < 0 || idx.row() >= m_items.size()) return {};
        const WorkspaceInfo& w = m_items[idx.row()];
        return role == NameRole ? w.name : role == ActiveRole ? w.active : QVariant();
    }
    QHash<int, QByteArray> roleNames() const override {
        return { {NameRole, "name"}, {ActiveRole, "active"} };
    }
    void add(const WorkspaceInfo& w) {
        beginInsertRows({}, m_items.size(), m_items.size());
        m_items.append(w); endInsertRows();
    }
    void setName(int i, const QString& name) {
        if (i < 0 || i >= m_items.size()) return;
        m_items[i].name = name;
        emit dataChanged(index(i), index(i), {NameRole});
    }
    void setActive(int i, bool a) {
        if (i < 0 || i >= m_items.size()) return;
        m_items[i].active = a;
        emit dataChanged(index(i), index(i), {ActiveRole});
    }
    int count() const { return m_items.size(); }
private:
    QVector<WorkspaceInfo> m_items;
};

class NotificationModel : public QAbstractListModel {
    Q_OBJECT
    Q_PROPERTY(int count READ count NOTIFY countChanged)
public:
    enum Role {
        TitleRole = Qt::UserRole + 1,
        BodyRole,
        AppIdRole,
        TimestampRole,
        ActionsRole,
        GroupRole
    };
    int rowCount(const QModelIndex&) const override { return m_items.size(); }
    QVariant data(const QModelIndex& idx, int role) const override {
        if (idx.row() < 0 || idx.row() >= m_items.size()) return {};
        const NotificationInfo& n = m_items[idx.row()];
        switch (role) {
        case TitleRole:     return n.title;
        case BodyRole:      return n.body;
        case AppIdRole:     return n.appId;
        case TimestampRole: return n.timestamp;
        case ActionsRole: {
            QStringList labels;
            for (const auto& a : n.actions)
                labels.append(a.label);
            return labels;
        }
        case GroupRole: {
            if (n.appId.isEmpty()) return QStringLiteral("system");
            return n.appId;
        }
        }
        return {};
    }
    QHash<int, QByteArray> roleNames() const override {
        return { {TitleRole, "title"}, {BodyRole, "body"},
                 {AppIdRole, "appId"}, {TimestampRole, "timestamp"},
                 {ActionsRole, "actions"}, {GroupRole, "group"} };
    }
    void add(const NotificationInfo& n) {
        if (m_items.size() >= 8) removeAt(0);   /* 最多 8 条 */
        beginInsertRows({}, m_items.size(), m_items.size());
        m_items.append(n);
        endInsertRows();
        emit countChanged();
    }
    void removeAt(int i) {
        if (i < 0 || i >= m_items.size()) return;
        beginRemoveRows({}, i, i);
        m_items.removeAt(i);
        endRemoveRows();
        emit countChanged();
    }
    void clear() {
        if (m_items.isEmpty()) return;
        beginResetModel();
        m_items.clear();
        endResetModel();
        emit countChanged();
    }
    int count() const { return m_items.size(); }
    QVector<NotificationInfo> history() const { return m_items; }
signals:
    void countChanged();
private:
    QVector<NotificationInfo> m_items;
};

/* 应用抽屉模型 (来自 vmapp 各虚拟化环境的 .desktop 文件) */
class AppsModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role { NameRole = Qt::UserRole + 1, ExecRole, VmappRole, SourceRole, IconRole, GroupRole };
    int rowCount(const QModelIndex&) const override { return m_items.size(); }
    QVariant data(const QModelIndex& idx, int role) const override {
        if (idx.row() < 0 || idx.row() >= m_items.size()) return {};
        const AppInfo& a = m_items[idx.row()];
        switch (role) {
        case NameRole:  return a.name;
        case ExecRole:  return a.exec;
        case VmappRole: return a.vmapp;
        case SourceRole: return a.source;
        case IconRole:  return a.icon;
        case GroupRole: return a.group;
        }
        return {};
    }
    QHash<int, QByteArray> roleNames() const override {
        return { {NameRole, "name"}, {ExecRole, "exec"}, {VmappRole, "vmapp"},
                 {SourceRole, "source"}, {IconRole, "icon"}, {GroupRole, "group"} };
    }
    void clear() {
        if (!m_items.isEmpty()) {
            beginResetModel(); m_items.clear(); endResetModel();
        }
    }
    void add(const AppInfo& a) {
        beginInsertRows({}, m_items.size(), m_items.size());
        m_items.append(a); endInsertRows();
    }
    int count() const { return m_items.size(); }
    const QVector<AppInfo>& items() const { return m_items; }
private:
    QVector<AppInfo> m_items;
};

class ShellBackend : public QObject {
    Q_OBJECT
    Q_PROPERTY(ToplevelModel* windows READ windows CONSTANT)
    Q_PROPERTY(WorkspaceModel* workspaces READ workspaces CONSTANT)
    Q_PROPERTY(NotificationModel* notifications READ notifications CONSTANT)
    Q_PROPERTY(AppsModel* apps READ apps CONSTANT)
    Q_PROPERTY(int panelHeight READ panelHeight CONSTANT)
    Q_PROPERTY(bool doNotDisturb READ doNotDisturb WRITE setDoNotDisturb NOTIFY doNotDisturbChanged)
public:
    explicit ShellBackend(QObject* parent = nullptr);
    ToplevelModel* windows() const { return m_windows; }
    WorkspaceModel* workspaces() const { return m_workspaces; }
    NotificationModel* notifications() const { return m_notifications; }
    AppsModel* apps() const { return m_apps; }
    int panelHeight() const { return 32; }   /* OUI_PANEL_HEIGHT */
    bool doNotDisturb() const { return m_doNotDisturb; }
    void setDoNotDisturb(bool dnd);

    /* 供 WaylandBridge 调用: 数据更新入口 */
    Q_INVOKABLE void addWindow(const QString& title, const QString& appId, bool active);
    Q_INVOKABLE void updateWindow(int index, const QString& title,
                                  const QString& appId, bool active);
    Q_INVOKABLE void removeWindow(int index);
    Q_INVOKABLE void addWorkspace(const QString& name);
    Q_INVOKABLE void setWorkspaceName(int index, const QString& name);
    Q_INVOKABLE void setWorkspaceActive(int index, bool active);
    Q_INVOKABLE void showNotification(const QString& title, const QString& body,
                                      const QString& appId = QString(),
                                      const QVariantList& actions = QVariantList());
    Q_INVOKABLE void dismissNotification(int index);
    Q_INVOKABLE void clearAllNotifications();

    /* 应用抽屉: 重新扫描 vmapp 各环境 + 系统 .desktop, 填充 apps 模型 */
    Q_INVOKABLE void refreshApps();
    /* 在指定 vmapp 环境中启动应用 */
    Q_INVOKABLE void launchInVmapp(const QString& vmapp, const QString& command);

    /* 用户操作 -> WaylandBridge 转发 */
    Q_INVOKABLE void launchApp(const QString& command);
    Q_INVOKABLE void activateWindow(int index);
    Q_INVOKABLE void closeWindow(int index);
    Q_INVOKABLE void activateWorkspace(int index);

signals:
    void doNotDisturbChanged();
private:
    void scanDesktopDir(const QString& dir, const QString& vmapp, const QString& source);

    ToplevelModel* m_windows;
    WorkspaceModel* m_workspaces;
    NotificationModel* m_notifications;
    AppsModel* m_apps;
    bool m_doNotDisturb = false;
};