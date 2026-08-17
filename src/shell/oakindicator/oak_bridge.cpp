/* OAK 状态指示器 — Qt(6)+wlroots 胶水桥实现
 * 见 oak_bridge.h 头部说明。
 */

#include "oak_bridge.h"

#include <QQuickWindow>
#include <QQuickRenderControl>
#include <QQuickItem>
#include <QSGTexture>
#include <QOpenGLContext>
#include <QOpenGLFramebufferObject>
#include <QThread>
#include <QDebug>

// wlroots
#include <wlr/render/drm_format_set.h>
#include <wlr/render/wlr_texture.h>
#include <wlr/render/wlr_renderer.h>
#include <wlr/render/wlr_render_pass.h>
#include <wlr/types/wlr_output.h>
#include <wlr/util/box.h>

// POSIX socket
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <cstdio>

/* =====================================================================
 * OakSocketClient — 轮询 openos-securityd
 * ===================================================================== */
OakSocketClient::OakSocketClient(const QString& sockPath, QObject* parent)
    : QObject(parent), m_sockPath(sockPath) {}

OakSocketClient::~OakSocketClient()
{
    if (m_notifier) delete m_notifier;
    if (m_fd >= 0) close(m_fd);
}

bool OakSocketClient::connectSocket()
{
    struct sockaddr_un addr;
    int fd;

    fd = ::socket(AF_UNIX, SOCK_STREAM, 0);
    if (fd < 0) return false;
    std::memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, m_sockPath.toUtf8().constData(),
                 sizeof addr.sun_path - 1);
    if (::connect(fd, (struct sockaddr*)&addr, sizeof addr) < 0) {
        ::close(fd);
        return false;
    }
    m_fd = fd;
    return true;
}

void OakSocketClient::start()
{
    if (connectSocket()) {
        m_notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
        connect(m_notifier, &QSocketNotifier::activated,
                this, &OakSocketClient::onReadable);
        emit stateChanged(QColor(0x4A, 0xA2, 0x6F), QStringLiteral("OAK: connected"));
    } else {
        /* 未连上: 状态置为待定/警告, 保持未连接 (由外部 timer 重试) */
        emit stateChanged(QColor(0xFF, 0x98, 0x00), QStringLiteral("OAK: offline"));
    }
}

void OakSocketClient::onReadable()
{
    char buf[256];
    ssize_t n = ::read(m_fd, buf, sizeof buf - 1);
    if (n <= 0) {
        /* 连接断开 */
        if (m_notifier) { delete m_notifier; m_notifier = nullptr; }
        if (m_fd >= 0) { ::close(m_fd); m_fd = -1; }
        emit stateChanged(QColor(0xF4, 0x43, 0x36), QStringLiteral("OAK: disconnected"));
        return;
    }
    buf[n] = '\0';
    QString msg = QString::fromUtf8(buf).trimmed();

    /* openos-securityd 响应: "OK <h2>" / "DENY" -> 映射到 NUI2 状态色 */
    QColor c;
    QString text;
    if (msg.startsWith(QStringLiteral("OK"))) {
        c = QColor(0x4A, 0xA2, 0x6F);   /* NUI_STATUS_SUCCESS */
        text = QStringLiteral("OAK: OK");
    } else if (msg.startsWith(QStringLiteral("DENY"))) {
        c = QColor(0xF4, 0x43, 0x36);   /* NUI_STATUS_ERROR */
        text = QStringLiteral("OAK: DENY");
    } else {
        c = QColor(0x21, 0x96, 0xF3);   /* NUI_STATUS_INFO */
        text = msg;
    }
    emit stateChanged(c, text);
}

/* =====================================================================
 * OakQmlRenderer — QML 离屏渲染到 wlroots 纹理
 * ===================================================================== */
bool OakQmlRenderer::init(QQuickRenderControl* rctl, QQuickWindow* win,
                          QQuickItem* root)
{
    m_rctl = rctl;
    m_win = win;
    m_root = root;
    if (m_root) {
        /* 找到 OAKState 组件 (由 QML 端提供 objectName="oakState") */
        m_stateItem = m_root->findChild<QQuickItem*>(QStringLiteral("oakState"));
    }
    return m_root != nullptr;
}

void OakQmlRenderer::setIndicatorColor(QColor c)
{
    if (c == m_color) return;
    m_color = c;
    m_dirty = true;
    /* 同步到 QML 组件的 color 属性 */
    if (m_stateItem) {
        m_stateItem->setProperty("color", c);
        m_win->requestUpdate();
    }
}

/* 在 wlr_output.frame 中调用:
 *   1. 渲染 QML 到离屏 FBO
 *   2. 读回 RGBA
 *   3. 用 wlr_texture_from_pixels 建 wlr 纹理 (需在渲染线程/GL 上下文) */
struct wlr_texture* OakQmlRenderer::renderToTexture(struct wlr_renderer* renderer)
{
    if (!m_rctl || !m_win || !m_root)
        return nullptr;

    const QSize sz = m_win->size();
    if (sz.width() <= 0 || sz.height() <= 0)
        return nullptr;

    /* QQuickRenderControl 渲染一帧到离屏 */
    m_rctl->polishItems();
    m_rctl->sync();
    if (m_dirty || true) {
        m_rctl->render();
    }
    m_rctl->releaseResources();
    m_dirty = false;

    /* 读回 FBO 像素 (需 GL 上下文活跃) */
    /* 简化: 用 QQuickWindow 的 grabWindow() 拿 QImage (跨平台, 但需在主线程) */
    QImage img = m_win->grabWindow();

    /* 上传 wlroots 纹理 */
    struct wlr_texture* tex = wlr_texture_from_pixels(
        renderer, DRM_FORMAT_ABGR8888,
        img.width(), img.height(), img.bytesPerLine(),
        img.constBits());
    return tex;
}
