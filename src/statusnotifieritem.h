// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QIcon>
#include <QObject>
#include <QPointer>
#include <QTimer>
#include <QVariant>

class DBusMenuClient;
class KIconLoader;
class QDBusPendingCallWatcher;
class QQuickItem;

// Client-side proxy for one org.kde.StatusNotifierItem. Reads all properties
// with Properties.GetAll, re-reads them (debounced) on the New* signals.
class StatusNotifierItem : public QObject
{
    Q_OBJECT

public:
    // \a registration is the string reported by the watcher: either
    // "<bus name>/<object path>" or just "<bus name>".
    explicit StatusNotifierItem(const QString &registration, QObject *parent = nullptr);
    ~StatusNotifierItem() override;

    QString registration() const { return m_registration; }
    QString service() const { return m_service; }
    QString path() const { return m_path; }

    bool isValid() const { return m_valid; }
    // True once the first GetAll reply was processed.
    bool isReady() const { return m_ready; }

    // Id as used by Plasma's system tray (Chromium/Electron ids are
    // disambiguated with the process name), so the same string can be used
    // in both applets' configuration.
    QString itemId() const { return m_itemId; }
    QString rawId() const { return m_rawId; }
    // Key used in tray-grid's own configuration. Same as itemId(), except for
    // libayatana-appindicator items whose Id is random per run
    // ("t_7wfEdOM1"); those are keyed by Title instead.
    QString configId() const;
    QString title() const { return m_title; }
    QString category() const { return m_category; }
    QString status() const { return m_status; }
    QString toolTipTitle() const { return m_toolTipTitle; }
    QString toolTipSubTitle() const { return m_toolTipSubTitle; }
    QString iconThemePath() const { return m_iconThemePath; }
    QString overlayIconName() const { return m_overlayIconName; }
    bool itemIsMenu() const { return m_itemIsMenu; }
    bool hasMenu() const { return !m_menuPath.isEmpty(); }
    QString menuPath() const { return m_menuPath; }

    // What QML should show: either a theme icon name (QString, recoloured by
    // Kirigami.Icon according to the colour scheme) or a QIcon. Takes the
    // NeedsAttention status into account.
    QVariant iconSource() const;
    // Human readable description of where the icon came from (debugging).
    QString iconOrigin() const { return m_iconOrigin; }

    void activate(QQuickItem *item, const QPointF &localPos);
    void secondaryActivate(QQuickItem *item, const QPointF &localPos);
    void scroll(int delta, Qt::Orientation orientation);
    // Opens the dbusmenu, or falls back to the item's ContextMenu method.
    void showContextMenu(QQuickItem *item, const QPointF &localPos);

    DBusMenuClient *menuClient();

Q_SIGNALS:
    void changed();
    void ready();
    void menuShown();
    void menuHidden();
    void menuActionTriggered();
    // Activate() succeeded (on failure the context menu is shown instead).
    void activated();

private Q_SLOTS:
    void scheduleRefresh();
    void onNewStatus(const QString &status);
    void onNewMenu();

private:
    void performRefresh();
    void refreshFinished(QDBusPendingCallWatcher *watcher);
    void resolveIcons(const QVariantMap &properties);
    void provideActivationToken(const QString &token);
    void callMethod(const QString &method, const QPoint &pos);
    QString resolveProcessName() const;
    KIconLoader *iconLoader() const;

    QString m_registration;
    QString m_service;
    QString m_path;
    QString m_interface;
    bool m_valid = false;
    bool m_ready = false;

    QString m_rawId;
    QString m_itemId;
    QString m_title;
    QString m_category;
    QString m_status;
    QString m_toolTipTitle;
    QString m_toolTipSubTitle;
    QString m_iconThemePath;
    QString m_overlayIconName;
    QString m_menuPath;
    bool m_itemIsMenu = false;

    QString m_iconName; // non-empty: theme icon
    QIcon m_icon; // otherwise
    QString m_attentionIconName;
    QIcon m_attentionIcon;
    QString m_iconOrigin;

    KIconLoader *m_customIconLoader = nullptr;
    DBusMenuClient *m_menuClient = nullptr;

    QTimer m_refreshTimer;
    bool m_refreshing = false;
    bool m_refreshAgain = false;
};
