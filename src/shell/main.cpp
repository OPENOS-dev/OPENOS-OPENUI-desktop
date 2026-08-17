/* OPENOS 桌面外壳 — Qt Quick 客户端入口
 * 架构: wlroots 合成器(openos-compositor) + Qt Quick 外壳(openos-shell)
 *   - 外壳以 layer-shell 方式显示面板/通知 (LayerShellQt 路线, 见 waylandbridge)
 *   - 任务栏经 wlr-foreign-toplevel-management 协议; 工作区经自研协议
 * 依赖: Qt 6 (Core/Gui/Quick/Qml) + qtwayland, 在 OPENOS/Linux 上运行。
 */
#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickStyle>
#include <QUrl>
#include <QWindow>
#include <QtGlobal>
#include "shellbackend.h"
#include "waylandbridge.h"

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    app.setApplicationName("OPENOS Shell");
    app.setOrganizationName("OPENOS");
    app.setQuitOnLastWindowClosed(false);   // 面板关闭不应退出外壳

    ShellBackend backend;
    WaylandBridge bridge(&backend);
    bridge.init();   // 绑定合成器协议 (骨架: 待 Linux 接入)

    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty("shell", &backend);
    engine.rootContext()->setContextProperty("wayland", &bridge);

    // 注入 OPENUI 设计令牌 (qml/OpenUI.qml)
    QQmlComponent openuiComp(&engine, QUrl(QStringLiteral("qrc:/qml/OpenUI.qml")));
    QObject* openUI = openuiComp.create();
    if (openUI)
        engine.rootContext()->setContextProperty("OpenUI", openUI);
    else
        qFatal("无法加载 OpenUI 令牌: %s", qPrintable(openuiComp.errorString()));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/qml/main.qml")));

    // 加载完成后, 把主窗口注册为 layer-shell 面板 (LayerShellQt 路线)
    if (!engine.rootObjects().isEmpty()) {
        if (auto* win = qobject_cast<QWindow*>(engine.rootObjects().first()))
            bridge.setWindowLayerSurface(win, QStringLiteral("openos-panel"),
                                         QStringLiteral("top"), true);
    }

    return app.exec();
}
