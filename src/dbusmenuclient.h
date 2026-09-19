// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QObject>
#include <QPointer>
#include <QPointF>
#include <QString>
#include <QVariantMap>

class QDBusArgument;
class QMenu;
class QQuickItem;

// Minimal com.canonical.dbusmenu client. Fetches the layout with GetLayout,
// turns it into a native QMenu (the same widget PlasmaExtras.Menu and the
// built-in system tray use, so it follows the Plasma style and colour scheme)
// and sends Event(id, "clicked") back when an entry is triggered.
class DBusMenuClient : public QObject
{
    Q_OBJECT

public:
    DBusMenuClient(const QString &service, const QString &path, QObject *parent = nullptr);
    ~DBusMenuClient() override;

    QString service() const;
    QString path() const;

    // Asynchronously refreshes the layout and pops the menu up at
    // \a localPos inside \a item. Emits menuShown() or failed().
    void popup(QQuickItem *item, const QPointF &localPos);

    // Closes the menu if it is open.
    void close();

    bool isOpen() const;

    // A tree node from GetLayout, exposed for testing (tools/sni-dump).
    struct Node {
        int id = 0;
        QVariantMap properties;
        QList<Node> children;
    };
    static Node parseLayout(const QDBusArgument &arg);

    // Fetches the layout synchronously (for command line tools only).
    bool fetchLayoutBlocking(Node *root, QString *error = nullptr);

    // Converts a dbusmenu label ("_File", "Save __as") into a QAction text.
    static QString convertMnemonic(const QString &label);

    // Sends Event(id, "clicked") (public for the test tool).
    void sendClicked(int id);

Q_SIGNALS:
    void menuShown();
    void menuHidden();
    void actionTriggered(int id);
    void failed(const QString &reason);
    // Emitted (Wayland) right before a "clicked" event is sent.
    void activationTokenReady(const QString &token);

private:
    void fillMenu(QMenu *menu, const Node &node);
    void refreshSubmenu(QMenu *submenu, int id);
    void sendEvent(int id, const QString &eventId);
    void triggered(int id);

    QString m_service;
    QString m_path;
    QPointer<QMenu> m_menu;
    QPointer<QQuickItem> m_anchorItem;
    int m_requestSerial = 0;
};
