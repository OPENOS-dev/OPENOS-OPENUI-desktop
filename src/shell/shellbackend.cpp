#include "shellbackend.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QRegularExpression>

ShellBackend::ShellBackend(QObject* parent)
    : QObject(parent),
      m_windows(new ToplevelModel(this)),
      m_workspaces(new WorkspaceModel(this)),
      m_notifications(new NotificationModel(this)),
      m_apps(new AppsModel(this)) {}

void ShellBackend::addWindow(const QString& title, const QString& appId, bool active) {
    m_windows->add({title, appId, active});
}
void ShellBackend::updateWindow(int index, const QString& title,
                                const QString& appId, bool active) {
    m_windows->update(index, title, appId, active);
}
void ShellBackend::removeWindow(int index) { m_windows->removeAt(index); }
void ShellBackend::setWindowActive(int index, bool active) { m_windows->setActive(index, active); }
void ShellBackend::addWorkspace(const QString& name) { m_workspaces->add({name, false}); }
void ShellBackend::setWorkspaceName(int index, const QString& name) { m_workspaces->setName(index, name); }
void ShellBackend::setWorkspaceActive(int index, bool active) { m_workspaces->setActive(index, active); }
void ShellBackend::setDoNotDisturb(bool dnd) {
    if (m_doNotDisturb != dnd) {
        m_doNotDisturb = dnd;
        emit doNotDisturbChanged();
    }
}

void ShellBackend::showNotification(const QString& title, const QString& body,
                                    const QString& appId, const QVariantList& actions) {
    if (m_doNotDisturb) return;  /* 勿扰模式: 静默丢弃 */

    QVector<NotificationAction> acts;
    for (const QVariant& a : actions) {
        QVariantMap m = a.toMap();
        acts.append({m.value("id").toString(), m.value("label").toString()});
    }
    m_notifications->add({title, body, appId,
                          QDateTime::currentSecsSinceEpoch(), acts});
}
void ShellBackend::dismissNotification(int index) { m_notifications->removeAt(index); }
void ShellBackend::clearAllNotifications() { m_notifications->clear(); }

/* 启动应用: fork/exec, 不阻塞 shell */
void ShellBackend::launchApp(const QString& command) {
    if (command.isEmpty()) return;
    QProcess* p = new QProcess(this);
    connect(p, &QProcess::finished, p, &QProcess::deleteLater);
    p->start("/bin/sh", QStringList() << "-c" << command);
}

/* 扫描 .desktop 目录, 解析 Name/Exec/Icon, 加入 apps 模型
 * source: "system" = pacman/系统安装; "vmapp" = 隔离环境
 * 跳过 NoDisplay/Hidden 条目, 同名同 exec 去重 */
void ShellBackend::scanDesktopDir(const QString& dir, const QString& vmapp,
                                  const QString& source) {
    QDir d(dir);
    if (!d.exists()) return;
    const QFileInfoList entries = d.entryInfoList(QStringList() << "*.desktop",
                                                   QDir::Files);
    for (const QFileInfo& fi : entries) {
        QFile f(fi.absoluteFilePath());
        if (!f.open(QIODevice::ReadOnly)) continue;
        QString name, exec, icon;
        bool hide = false;
        bool inDesktop = false;
        while (!f.atEnd()) {
            const QString line = QString::fromUtf8(f.readLine()).trimmed();
            if (line == QLatin1String("[Desktop Entry]")) inDesktop = true;
            else if (line.startsWith(QLatin1Char('[')) && !line.startsWith(QLatin1String("[Desktop Entry]")))
                inDesktop = false;   /* 跳过 Action=/其他段 */
            if (!inDesktop) continue;
            if (line.startsWith(QLatin1String("Name="))) name = line.mid(5);
            else if (line.startsWith(QLatin1String("Exec="))) exec = line.mid(5);
            else if (line.startsWith(QLatin1String("Icon="))) icon = line.mid(5);
            else if (line.startsWith(QLatin1String("NoDisplay=")) && line.mid(10).startsWith(QLatin1Char('t'))) hide = true;
            else if (line.startsWith(QLatin1String("Hidden=")) && line.mid(7).startsWith(QLatin1Char('t'))) hide = true;
        }
        if (hide) continue;
        if (name.isEmpty() || exec.isEmpty()) continue;
        /* 去重: 同源同名同 exec 跳过 */
        for (const AppInfo& a : m_apps->items()) {
            if (a.source == source && a.name == name && a.exec == exec) { hide = true; break; }
        }
        if (hide) continue;
        /* 字母分组 (Win10 "所有应用" 首字母分界) */
        QChar c = name.at(0).toUpper();
        QString group = c.isLetter() ? QString(c) : QStringLiteral("#");
        m_apps->add({name, exec, vmapp, source, icon, group});
    }
}

/* 应用抽屉: 扫描 vmapp 各隔离环境 + 宿主系统 .desktop (pacman 安装) */
void ShellBackend::refreshApps() {
    m_apps->clear();

    /* 宿主系统 .desktop — pacman 安装的软件写入此处 */
    scanDesktopDir(QStringLiteral("/usr/share/applications"),
                   QString(), QStringLiteral("system"));
    scanDesktopDir(QStringLiteral("/usr/local/share/applications"),
                   QString(), QStringLiteral("system"));

    /* vmapp 隔离环境: 经 libvmapp 枚举 /vmapp 各子目录。
     * 生产: 调 libvmapp 列出; 原型枚举常见目录 */
    const QStringList vmapps = {
        QStringLiteral("opt"), QStringLiteral("firefox"), QStringLiteral("code")
    };
    for (const QString& app : vmapps) {
        const QString p = QStringLiteral("/vmapp/%1/usr/share/applications")
                              .arg(app);
        scanDesktopDir(p, app, QStringLiteral("vmapp"));
    }
}

/* 在指定 vmapp 环境启动应用 */
void ShellBackend::launchInVmapp(const QString& vmapp, const QString& command) {
    if (vmapp.isEmpty()) { launchApp(command); return; }
    /* 生产: 调 libvmapp 进入隔离视图后 exec; 原型 shell 包装 */
    const QString full = QStringLiteral("cd /vmapp/%1 && %2").arg(vmapp, command);
    launchApp(full);
}

/* 以下动作最终应转发给 Wayland 协议 (foreign-toplevel / workspace)。
 * 骨架阶段由 ShellBackend 内部处理; 完整版由 WaylandBridge 接管。
 */
void ShellBackend::activateWindow(int index) {
    // TODO(wayland): zwlr_foreign_toplevel_manager_v1 activate
    Q_UNUSED(index);
}
void ShellBackend::closeWindow(int index) {
    // TODO(wayland): zwlr_foreign_toplevel_manager_v1 close
    Q_UNUSED(index);
}
void ShellBackend::activateWorkspace(int index) {
    // TODO(wayland): openos_workspace_handle_v1 activate
    Q_UNUSED(index);
}
