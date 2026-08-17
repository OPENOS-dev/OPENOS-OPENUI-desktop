#pragma once
/* OAK 状态指示器 — Qt(6)+wlroots 胶水桥 (C++)
 *
 * 职责:
 *   1. 在 wlr_output.frame 中调用 QSGRenderer 渲染 QML 场景 (OAKState 组件)
 *   2. 把 QML OAKState 的颜色属性与 openos-securityd 的 Unix socket 消息绑定
 *   3. 集成 Qt 事件循环与 wlroots 主循环 (QCoreApplication::processEvents 轮询)
 *   4. 兼容 C++17 (CMAKE_CXX_STANDARD 17)
 *
 * 渲染策略 (避免线程锁死):
 *   - 用 QQuickRenderControl 离屏渲染 QML 到内存 RGBA (Qt Quick 纹理)
 *   - 在 wlr_output.frame 中把 RGBA 传成 wlroots 纹理 (wlr_texture_from_pixels)
 *   - 单线程主循环: wl_display_dispatch + QCoreApplication::processEvents
 *
 * 需: Qt6 (Gui/Quick/Qml), wlroots, wayland-server。
 */

#include <QColor>
#include <QQuickItem>
#include <QQuickRenderControl>
#include <QSize>
#include <QString>
#include <QObject>
#include <QSocketNotifier>
#include <cstdint>

// 前向声明 (wlroots 类型, 只在 .cpp 包含完整头)
struct wlr_output;
struct wlr_renderer;
struct wlr_texture;
struct wl_display;

/* openos-securityd socket 客户端: 轮询 OAK 状态 */
class OakSocketClient : public QObject {
    Q_OBJECT
public:
    explicit OakSocketClient(const QString& sockPath = QStringLiteral("/run/openos/oak.sock"),
                             QObject* parent = nullptr);
    ~OakSocketClient() override;

    /* 启动: 尝试连接 + QSocketNotifier 监听 (未连上则周期性重试) */
    void start();

signals:
    /* 状态变化: color=OAK 状态色 (NUI2: success/error/warning) */
    void stateChanged(QColor color, QString message);

private slots:
    void onReadable();

private:
    bool connectSocket();
    QString m_sockPath;
    int m_fd = -1;
    QSocketNotifier* m_notifier = nullptr;
};

/* QML 离屏渲染器: 渲染 OAKState.qml 到 RGBA 缓冲 */
class OakQmlRenderer {
public:
    bool init(QQuickRenderControl* rctl, QQuickWindow* win, QQuickItem* root);
    /* 在 wlr_output.frame 中调用: 渲染 QML, 上传 wlroots 纹理, 绘制到输出
     * 返回 wlr_texture* (由调用方 draw 后 destroy) */
    struct wlr_texture* renderToTexture(struct wlr_renderer* renderer);
    void setIndicatorColor(QColor c);
    QColor indicatorColor() const { return m_color; }

private:
    QQuickRenderControl* m_rctl = nullptr;
    QQuickWindow* m_win = nullptr;
    QQuickItem* m_root = nullptr;
    QQuickItem* m_stateItem = nullptr;   /* OAKState 组件 (findChild) */
    QColor m_color;
    uint8_t* m_rgba = nullptr;           /* 离屏 RGBA 缓冲 */
    int m_w = 0, m_h = 0;
    bool m_dirty = true;
};
