#pragma once
/* OPENUI 共享组件 — 供所有独立 app 复用
 * 仅提供 GUI 框架 (设计令牌 + 通用基类), 不含具体 app 功能。
 */

#include <QObject>
#include <QQuickView>

/* 加载 OpenUI 令牌到 QML 上下文 (所有 app 的 QML 都能用 OpenUI.* 属性) */
class OpenUiComponent {
public:
    /* 在 engine 上注册 OpenUI 令牌组件 */
    static bool registerTokens(QQmlEngine* engine, QObject* parent);
};

/* 独立 app 窗口基类: 加载指定 QML 文件为独立 Wayland 窗口 */
class OpenUiAppWindow : public QQuickView {
    Q_OBJECT
public:
    OpenUiAppWindow(const QString& qmlSource, QWindow* parent = nullptr);
};
