// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "popuphelper.h"

#include <KWaylandExtras>
#include <KWindowSystem>

#include <QFuture>
#include <QGuiApplication>
#include <QMenu>
#include <QPointer>
#include <QQuickItem>
#include <QQuickWindow>
#include <QScreen>

void PopupHelper::withActivationToken(QWindow *window, QObject *context, const std::function<void(const QString &)> &callback)
{
    if (!KWindowSystem::isPlatformWayland()) {
        callback(QString());
        return;
    }
    QPointer<QObject> guard(context);
    KWaylandExtras::xdgActivationToken(window, QString()).then(context, [guard, callback](const QString &token) {
        if (guard) {
            callback(token);
        }
    });
}

QPoint PopupHelper::globalPosition(QQuickItem *item, const QPointF &localPos)
{
    if (!item || !item->window()) {
        return {};
    }
    const QPointF scenePos = item->mapToScene(localPos);
    return item->window()->mapToGlobal(scenePos.toPoint());
}

void PopupHelper::showMenu(QMenu *menu, QQuickItem *item, const QPointF &localPos)
{
    if (!menu) {
        return;
    }
    QQuickWindow *parentWindow = item ? item->window() : nullptr;
    if (!parentWindow) {
        menu->popup(QCursor::pos());
        return;
    }

    menu->adjustSize();
    menu->winId(); // create the native window so we can configure it
    QWindow *menuWindow = menu->windowHandle();
    menuWindow->setTransientParent(parentWindow);

    const QPoint scenePos = item->mapToScene(localPos).toPoint();

    if (KWindowSystem::isPlatformWayland()) {
        // Anchor a 1x1 rect at the click position; the compositor flips or
        // slides the popup when it would leave the screen.
        const QRect anchorRect(scenePos, QSize(1, 1));
        const Qt::Edges anchor = Qt::TopEdge | (qGuiApp->isLeftToRight() ? Qt::LeftEdge : Qt::RightEdge);
        const Qt::Edges gravity = Qt::BottomEdge | (qGuiApp->isLeftToRight() ? Qt::RightEdge : Qt::LeftEdge);
        menuWindow->setProperty("_q_waylandPopupAnchorRect", anchorRect);
        menuWindow->setProperty("_q_waylandPopupAnchor", QVariant::fromValue(anchor));
        menuWindow->setProperty("_q_waylandPopupGravity", QVariant::fromValue(gravity));
        menu->popup(parentWindow->screen() ? parentWindow->screen()->geometry().topLeft() : QPoint());
        return;
    }

    menu->popup(parentWindow->mapToGlobal(scenePos));
}
