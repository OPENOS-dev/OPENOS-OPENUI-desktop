/* OAK 状态指示器 — 合成器集成 main.cpp 框架 (C++17)
 *
 * 在 wlr_output.frame 中调用 QSGRenderer 渲染 QML, 并绑定 openos-securityd
 * socket 消息到 QML OAKState 颜色属性。
 *
 * 主循环: oak_run_single_loop (单线程) 或 oak_run_threaded (线程分离)。
 * 说明: 本框架演示 Qt/wlroots 集成点; 真实合成器需接入现有 server 的
 *       wlr_output.frame 信号 (在 perFrame 回调中调用 renderToTexture + draw)。
 *
 * 构建: cmake -DCMAKE_CXX_STANDARD=17 ...  (见 CMakeLists.txt)
 */

#include "oak_bridge.h"
#include "oak_mainloop.h"

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QQuickItem>
#include <QCoreApplication>
#include <QTimer>
#include <QUrl>
#include <atomic>

#include <wayland-server-core.h>
#include <wlr/backend.h>

int main(int argc, char** argv)
{
    QGuiApplication app(argc, argv);
    app.setApplicationName("openos-oak-indicator");
    app.setQuitOnLastWindowClosed(false);

    /* ---- 1. QML 场景 (OAKState 指示器) ---- */
    QQmlEngine qmlEngine;
    QQmlComponent component(&qmlEngine,
                            QUrl(QStringLiteral("qrc:/oak/OAKState.qml")));
    QQuickItem* root = qobject_cast<QQuickItem*>(component.create());
    if (!root) {
        qCritical("无法加载 OAKState.qml: %s",
                  qPrintable(component.errorString()));
        return 1;
    }

    /* 离屏窗口 + 渲染控制 */
    QQuickWindow win;
    win.resize(96, 24);
    QQuickRenderControl rctl;
    rctl.initialize(&win);
    root->setParentItem(win.contentItem());

    OakQmlRenderer renderer;
    renderer.init(&rctl, &win, root);

    /* ---- 2. OAK socket 状态绑定 ---- */
    OakSocketClient oakSock;
    QObject::connect(&oakSock, &OakSocketClient::stateChanged,
                     &renderer, [&](QColor c, QString msg) {
        renderer.setIndicatorColor(c);
    });
    oakSock.start();

    /* ---- 3. wlroots server (简化为无头? 真实合成器用已有 server) ---- */
    struct wl_display* display = wl_display_create();
    /* 说明: 完整合成器在此创建 backend/renderer, 并把 renderer 传给
     *       renderToTexture; 并连接 wlr_output.frame 信号 -> perFrame。 */

    /* ---- 4. 主循环集成 (单线程轮询) ---- */
    std::atomic<bool> quit{ false };
    oak_run_single_loop(display, [&]() {
        /* perFrame: 由 wlr_output.frame 驱动; 此处示例:
         *   wlr_texture* t = renderer.renderToTexture(renderer_instance);
         *   (draw 到输出, 然后 wlr_texture_destroy(t))
         * 真实集成见 oak_bridge.h 渲染策略注释。 */
    }, [&]() { return quit.load(); });

    if (display)
        wl_display_destroy(display);
    return 0;
}
