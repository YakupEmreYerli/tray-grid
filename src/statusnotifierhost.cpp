// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "statusnotifierhost.h"
#include "statusnotifieritem.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusServiceWatcher>
#include <QDBusVariant>
#include <QLoggingCategory>

Q_LOGGING_CATEGORY(TRAYGRID_HOST, "traygrid.host", QtWarningMsg)

using namespace Qt::StringLiterals;

static const QString s_watcherService = u"org.kde.StatusNotifierWatcher"_s;
static const QString s_watcherPath = u"/StatusNotifierWatcher"_s;
static const QString s_watcherInterface = u"org.kde.StatusNotifierWatcher"_s;

StatusNotifierHost *StatusNotifierHost::self()
{
    // Deliberately leaked: items may still be referenced by QML during
    // engine teardown, and the process is exiting anyway.
    static StatusNotifierHost *instance = new StatusNotifierHost();
    return instance;
}

StatusNotifierHost::StatusNotifierHost()
    : QObject(nullptr)
{
    QDBusConnection bus = QDBusConnection::sessionBus();
    if (!bus.isConnected()) {
        qCWarning(TRAYGRID_HOST) << "No session bus";
        return;
    }

    // Plasma's own tray already owns org.kde.StatusNotifierHost-<pid> inside
    // plasmashell; use a distinct, still spec-conformant name.
    m_hostServiceName = u"org.kde.StatusNotifierHost-%1-traygrid"_s.arg(QCoreApplication::applicationPid());
    bus.registerService(m_hostServiceName);

    auto *serviceWatcher = new QDBusServiceWatcher(s_watcherService, bus, QDBusServiceWatcher::WatchForOwnerChange, this);
    connect(serviceWatcher, &QDBusServiceWatcher::serviceOwnerChanged, this, [this](const QString &, const QString &oldOwner, const QString &newOwner) {
        if (!oldOwner.isEmpty()) {
            disconnectFromWatcher();
        }
        if (!newOwner.isEmpty()) {
            connectToWatcher();
        }
    });

    bus.connect(s_watcherService, s_watcherPath, s_watcherInterface, u"StatusNotifierItemRegistered"_s, this, SLOT(onItemRegistered(QString)));
    bus.connect(s_watcherService, s_watcherPath, s_watcherInterface, u"StatusNotifierItemUnregistered"_s, this, SLOT(onItemUnregistered(QString)));

    if (bus.interface()->isServiceRegistered(s_watcherService)) {
        connectToWatcher();
    }
}

QList<StatusNotifierItem *> StatusNotifierHost::items() const
{
    QList<StatusNotifierItem *> result;
    result.reserve(m_order.size());
    for (const QString &registration : m_order) {
        result.append(m_items.value(registration));
    }
    return result;
}

void StatusNotifierHost::connectToWatcher()
{
    QDBusConnection bus = QDBusConnection::sessionBus();

    QDBusMessage registerHost = QDBusMessage::createMethodCall(s_watcherService, s_watcherPath, s_watcherInterface, u"RegisterStatusNotifierHost"_s);
    registerHost << m_hostServiceName;
    bus.call(registerHost, QDBus::NoBlock);

    QDBusMessage getItems = QDBusMessage::createMethodCall(s_watcherService, s_watcherPath, u"org.freedesktop.DBus.Properties"_s, u"Get"_s);
    getItems << s_watcherInterface << u"RegisteredStatusNotifierItems"_s;
    auto *watcher = new QDBusPendingCallWatcher(bus.asyncCall(getItems, 5000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        QDBusPendingReply<QDBusVariant> reply = *w;
        if (reply.isError()) {
            qCWarning(TRAYGRID_HOST) << "Cannot read RegisteredStatusNotifierItems:" << reply.error().message();
            return;
        }
        m_watcherAvailable = true;
        Q_EMIT watcherAvailableChanged();
        const QStringList registrations = reply.value().variant().toStringList();
        for (const QString &registration : registrations) {
            addItem(registration);
        }
    });
}

void StatusNotifierHost::disconnectFromWatcher()
{
    const QStringList registrations = m_order;
    for (const QString &registration : registrations) {
        removeItem(registration);
    }
    if (m_watcherAvailable) {
        m_watcherAvailable = false;
        Q_EMIT watcherAvailableChanged();
    }
}

void StatusNotifierHost::onItemRegistered(const QString &registration)
{
    addItem(registration);
}

void StatusNotifierHost::onItemUnregistered(const QString &registration)
{
    removeItem(registration);
}

void StatusNotifierHost::addItem(const QString &registration)
{
    if (registration.isEmpty() || m_items.contains(registration)) {
        return;
    }
    auto *item = new StatusNotifierItem(registration, this);
    if (!item->isValid()) {
        delete item;
        return;
    }
    m_items.insert(registration, item);
    m_order.append(registration);
    connect(item, &StatusNotifierItem::changed, this, [this, item]() {
        Q_EMIT itemChanged(item);
    });
    qCDebug(TRAYGRID_HOST) << "Item added" << registration;
    Q_EMIT itemAdded(item);
}

void StatusNotifierHost::removeItem(const QString &registration)
{
    StatusNotifierItem *item = m_items.take(registration);
    if (!item) {
        return;
    }
    m_order.removeAll(registration);
    qCDebug(TRAYGRID_HOST) << "Item removed" << registration;
    Q_EMIT itemRemoved(registration);
    item->disconnect(this);
    item->deleteLater();
}
