// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QDBusAbstractAdaptor>
#include <QDBusContext>
#include <QDBusServiceWatcher>
#include <QObject>
#include <QStringList>

// Minimal org.kde.StatusNotifierWatcher for a private test bus
// (dbus-run-session), so tests never touch the user's real tray.
class MockWatcher : public QObject, protected QDBusContext
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.kde.StatusNotifierWatcher")
    Q_PROPERTY(QStringList RegisteredStatusNotifierItems READ registeredItems)
    Q_PROPERTY(bool IsStatusNotifierHostRegistered READ hostRegistered)
    Q_PROPERTY(int ProtocolVersion READ protocolVersion)

public:
    explicit MockWatcher(QObject *parent = nullptr);
    bool start();

    QStringList registeredItems() const { return m_items; }
    bool hostRegistered() const { return true; }
    int protocolVersion() const { return 0; }

public Q_SLOTS:
    void RegisterStatusNotifierItem(const QString &service);
    void RegisterStatusNotifierHost(const QString &service);

Q_SIGNALS:
    void StatusNotifierItemRegistered(const QString &item);
    void StatusNotifierItemUnregistered(const QString &item);
    void StatusNotifierHostRegistered();
    void StatusNotifierHostUnregistered();

private:
    QStringList m_items;
    QDBusServiceWatcher m_serviceWatcher;
};
