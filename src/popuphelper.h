// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QPoint>
#include <QRect>
#include <QString>

#include <functional>

class QMenu;
class QObject;
class QQuickItem;
class QWindow;

namespace PopupHelper
{
// On Wayland, asks the compositor for an xdg-activation token (so the tray
// application may raise its window) and then invokes \a callback with it.
// On X11, or when no token can be obtained, \a callback gets an empty string.
void withActivationToken(QWindow *window, QObject *context, const std::function<void(const QString &)> &callback);

// Global (screen) position of \a localPos inside \a item, suitable for the
// x/y arguments of StatusNotifierItem.Activate & co.
QPoint globalPosition(QQuickItem *item, const QPointF &localPos);

// Shows \a menu as a popup attached to \a item at \a localPos (item
// coordinates). Uses xdg_popup anchoring on Wayland.
void showMenu(QMenu *menu, QQuickItem *item, const QPointF &localPos);
}
