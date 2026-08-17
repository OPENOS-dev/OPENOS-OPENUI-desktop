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
struct AppInfo { QString name; QString exec; QString vmapp; };   /* 应用抽屉条目 */
struct WorkspaceInfo { QString name; bool active; };
struct NotificationInfo { QString title; QString body; };

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
public:
    enum Role { TitleRole = Qt::UserRole + 1, BodyRole };
    int rowCount(const QModelIndex&) const override { return m_items.size(); }
    QVariant data(const QModelIndex& idx, int role) const override {
        if (idx.row() < 0 || idx.row() >= m_items.size()) return {};
        const NotificationInfo& n = m_items[idx.row()];
        return role == TitleRole ? n.title : role == BodyRole ? n.body : QVariant();
    }
    QHash<int, QByteArray> roleNames() const override {
        return { {TitleRole, "title"}, {BodyRole, "body"} };
    }
    void add(const NotificationInfo& n) {
        if (m_items.size() >= 4) removeAt(0);   /* 最多 4 条 */
        beginInsertRows({}, m_items.size(), m_items.size());
        m_items.append(n); endInsertRows();
    }
    void removeAt(int i) {
        if (i < 0 || i >= m_items.size()) return;
        beginRemoveRows({}, i, i); m_items.removeAt(i); endRemoveRows();
    }
    int count() const { return m_items.size(); }
private:
    QVector<NotificationInfo> m_items;
};

/* 应用抽屉模型 (来自 vmapp 各虚拟化环境的 .desktop 文件) */
class AppsModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role { NameRole = Qt::UserRole + 1, ExecRole, VmappRole };
    int rowCount(const QModelIndex&) const override { return m_items.size(); }
    QVariant data(const QModelIndex& idx, int role) const override {
        if (idx.row() < 0 || idx.row() >= m_items.size()) return {};
        const AppInfo& a = m_items[idx.row()];
        switch (role) {
        case NameRole:  return a.name;
        case ExecRole:  return a.exec;
        case VmappRole: return a.vmapp;
        }
        return {};
    }
    QHash<int, QByteArray> roleNames() const override {
        return { {NameRole, "name"}, {ExecRole, "exec"}, {VmappRole, "vmapp"} };
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
public:
    explicit ShellBackend(QObject* parent = nullptr);
    ToplevelModel* windows() const { return m_windows; }
    WorkspaceModel* workspaces() const { return m_workspaces; }
    NotificationModel* notifications() const { return m_notifications; }
    AppsModel* apps() const { return m_apps; }
    int panelHeight() const { return 32; }   /* OUI_PANEL_HEIGHT */

    /* 供 WaylandBridge 调用: 数据更新入口 */
    Q_INVOKABLE void addWindow(const QString& title, const QString& appId, bool active);
    Q_INVOKABLE void updateWindow(int index, const QString& title,
                                  const QString& appId, bool active);
    Q_INVOKABLE void removeWindow(int index);
    Q_INVOKABLE void addWorkspace(const QString& name);
    Q_INVOKABLE void setWorkspaceName(int index, const QString& name);
    Q_INVOKABLE void setWorkspaceActive(int index, bool active);
    Q_INVOKABLE void showNotification(const QString& title, const QString& body);
    Q_INVOKABLE void dismissNotification(int index);

    /* 应用抽屉: 重新扫描 vmapp 各环境 + 系统 .desktop, 填充 apps 模型 */
    Q_INVOKABLE void refreshApps();
    /* 在指定 vmapp 环境中启动应用 */
    Q_INVOKABLE void launchInVmapp(const QString& vmapp, const QString& command);

    /* 用户操作 -> WaylandBridge 转发 */
    Q_INVOKABLE void launchApp(const QString& command);
    Q_INVOKABLE void activateWindow(int index);
    Q_INVOKABLE void closeWindow(int index);
    Q_INVOKABLE void activateWorkspace(int index);

private:
    void scanDesktopDir(const QString& dir, const QString& vmapp);

    ToplevelModel* m_windows;
    WorkspaceModel* m_workspaces;
    NotificationModel* m_notifications;
    AppsModel* m_apps;
};
