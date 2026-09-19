// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QHash>
#include <QObject>
#include <QStringList>

#include "statusnotifieritem.h"

// Process-wide StatusNotifierHost. Registers itself with
// org.kde.StatusNotifierWatcher and keeps one StatusNotifierItem per
// registered item, in registration order.
class StatusNotifierHost : public QObject
{
    Q_OBJECT

public:
    static StatusNotifierHost *self();

    // Registration strings, in the order the items appeared.
    QStringList registrations() const { return m_order; }
    StatusNotifierItem *item(const QString &registration) const { return m_items.value(registration); }
    QList<StatusNotifierItem *> items() const;

    bool isWatcherAvailable() const { return m_watcherAvailable; }
    QString hostServiceName() const { return m_hostServiceName; }

Q_SIGNALS:
    void itemAdded(StatusNotifierItem *item);
    void itemRemoved(const QString &registration);
    void itemChanged(StatusNotifierItem *item);
    void watcherAvailableChanged();

private Q_SLOTS:
    void onItemRegistered(const QString &registration);
    void onItemUnregistered(const QString &registration);

private:
    StatusNotifierHost();
    void connectToWatcher();
    void disconnectFromWatcher();
    void addItem(const QString &registration);
    void removeItem(const QString &registration);

    QString m_hostServiceName;
    bool m_watcherAvailable = false;
    QHash<QString, StatusNotifierItem *> m_items;
    QStringList m_order;
};
