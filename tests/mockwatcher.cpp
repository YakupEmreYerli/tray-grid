// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mockwatcher.h"

#include <QDBusConnection>
#include <QDBusMessage>

MockWatcher::MockWatcher(QObject *parent)
    : QObject(parent)
{
    m_serviceWatcher.setConnection(QDBusConnection::sessionBus());
    m_serviceWatcher.setWatchMode(QDBusServiceWatcher::WatchForUnregistration);
    connect(&m_serviceWatcher, &QDBusServiceWatcher::serviceUnregistered, this, [this](const QString &service) {
        const QStringList items = m_items;
        for (const QString &item : items) {
            if (item.startsWith(service + QLatin1Char('/'))) {
                m_items.removeAll(item);
                Q_EMIT StatusNotifierItemUnregistered(item);
            }
        }
        m_serviceWatcher.removeWatchedService(service);
    });
}

bool MockWatcher::start()
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.registerService(QStringLiteral("org.kde.StatusNotifierWatcher"))) {
        return false;
    }
    return bus.registerObject(QStringLiteral("/StatusNotifierWatcher"), this, QDBusConnection::ExportAllSlots | QDBusConnection::ExportAllSignals | QDBusConnection::ExportAllProperties);
}

void MockWatcher::RegisterStatusNotifierItem(const QString &serviceOrPath)
{
    QString service = serviceOrPath;
    QString path = QStringLiteral("/StatusNotifierItem");
    if (serviceOrPath.startsWith(QLatin1Char('/'))) {
        service = message().service();
        path = serviceOrPath;
    }
    const QString item = service + path;
    if (m_items.contains(item)) {
        return;
    }
    m_items.append(item);
    m_serviceWatcher.addWatchedService(service);
    Q_EMIT StatusNotifierItemRegistered(item);
}

void MockWatcher::RegisterStatusNotifierHost(const QString &)
{
    Q_EMIT StatusNotifierHostRegistered();
}
