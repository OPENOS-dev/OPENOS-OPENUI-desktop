/* OPENUI 共享组件实现 (见 openui_component.h)
 * 独立 app 复用: 注册令牌 + 加载 QML 窗口。
 */

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
