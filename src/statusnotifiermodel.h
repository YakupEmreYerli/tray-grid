// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QPointer>
#include <QStringList>

class QQuickItem;
class StatusNotifierItem;

// QML-facing list of StatusNotifierItems known to the process-wide host.
class StatusNotifierModel : public QAbstractListModel
{
    Q_OBJECT
    // Config ids (see StatusNotifierItem::configId) that are left out.
    Q_PROPERTY(QStringList excludedIds READ excludedIds WRITE setExcludedIds NOTIFY excludedIdsChanged)
    // Whether items with Status == Passive are listed.
    Q_PROPERTY(bool showPassive READ showPassive WRITE setShowPassive NOTIFY showPassiveChanged)
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    // Whether org.kde.StatusNotifierWatcher is reachable.
    Q_PROPERTY(bool watcherAvailable READ watcherAvailable NOTIFY watcherAvailableChanged)
    // True while one of the items' context menus is open.
    Q_PROPERTY(bool menuOpen READ menuOpen NOTIFY menuOpenChanged)

public:
    enum Roles {
        RegistrationRole = Qt::UserRole + 1,
        ItemIdRole,
        ConfigIdRole,
        TitleRole,
        ToolTipTitleRole,
        ToolTipSubTitleRole,
        StatusRole,
        CategoryRole,
        IconSourceRole,
        OverlayIconNameRole,
        ItemIsMenuRole,
        HasMenuRole,
        IconOriginRole,
        ExcludedRole,
    };
    Q_ENUM(Roles)

    explicit StatusNotifierModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    QStringList excludedIds() const { return m_excludedIds; }
    void setExcludedIds(const QStringList &ids);
    bool showPassive() const { return m_showPassive; }
    void setShowPassive(bool show);
    bool watcherAvailable() const;
    bool menuOpen() const { return m_menuOpen; }

    // Mouse actions. \a item is the delegate, \a x / \a y the position
    // inside it (used for Activate's coordinates and menu placement).
    Q_INVOKABLE void activate(int row, QQuickItem *item, qreal x, qreal y);
    Q_INVOKABLE void secondaryActivate(int row, QQuickItem *item, qreal x, qreal y);
    Q_INVOKABLE void showContextMenu(int row, QQuickItem *item, qreal x, qreal y);
    Q_INVOKABLE void scroll(int row, int delta, bool horizontal);

    // When set, excluded items are still listed (with ExcludedRole == true);
    // used by the configuration page.
    Q_PROPERTY(bool includeExcluded READ includeExcluded WRITE setIncludeExcluded NOTIFY includeExcludedChanged)
    bool includeExcluded() const { return m_includeExcluded; }
    void setIncludeExcluded(bool include);

Q_SIGNALS:
    void excludedIdsChanged();
    void showPassiveChanged();
    void countChanged();
    void watcherAvailableChanged();
    void menuOpenChanged();
    void includeExcludedChanged();
    // A context menu entry was chosen / an item was activated; the popup
    // usually wants to close.
    void menuActionTriggered();
    void itemActivated();

private:
    bool accepts(StatusNotifierItem *item) const;
    void connectItem(StatusNotifierItem *item);
    void rebuild();
    void onItemChanged(StatusNotifierItem *item);
    StatusNotifierItem *itemAt(int row) const;
    void setMenuOpen(bool open);

    QList<QPointer<StatusNotifierItem>> m_rows;
    QStringList m_excludedIds;
    bool m_showPassive = true;
    bool m_includeExcluded = false;
    bool m_menuOpen = false;
};
