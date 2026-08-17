/* OPENUI 共享组件实现 (见 openui_component.h)
 * 独立 app 复用: 注册令牌 + 加载 QML 窗口。
 */

#include "openui_component.h"

#include <QQmlComponent>
#include <QQmlContext>
#include <QQuickItem>
#include <QUrl>

bool OpenUiComponent::registerTokens(QQmlEngine* engine, QObject* parent)
{
    QQmlComponent comp(engine, QUrl(QStringLiteral("qrc:/ui/OpenUI.qml")));
    if (comp.isError()) {
        qWarning("OpenUI 令牌加载失败: %s",
                 qPrintable(comp.errorString()));
        return false;
    }
    QObject* token = comp.create();
    if (!token) return false;
    engine->rootContext()->setContextProperty(QStringLiteral("OpenUI"), token);
    token->setParent(parent ? parent : engine);
    return true;
}

OpenUiAppWindow::OpenUiAppWindow(const QString& qmlSource, QWindow* parent)
    : QQuickView(parent)
{
    setResizeMode(QQuickView::SizeRootObjectToView);
    setSource(QUrl(qmlSource));
    setColor(Qt::transparent);
}
