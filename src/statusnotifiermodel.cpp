// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "statusnotifiermodel.h"
#include "statusnotifierhost.h"
#include "statusnotifieritem.h"

#include <QQuickItem>

using namespace Qt::StringLiterals;

StatusNotifierModel::StatusNotifierModel(QObject *parent)
    : QAbstractListModel(parent)
{
    StatusNotifierHost *host = StatusNotifierHost::self();

    connect(host, &StatusNotifierHost::itemAdded, this, [this](StatusNotifierItem *item) {
        connectItem(item);
        if (item->isReady()) {
            rebuild();
        }
    });
    connect(host, &StatusNotifierHost::itemRemoved, this, &StatusNotifierModel::rebuild);
    connect(host, &StatusNotifierHost::itemChanged, this, &StatusNotifierModel::onItemChanged);
    connect(host, &StatusNotifierHost::watcherAvailableChanged, this, &StatusNotifierModel::watcherAvailableChanged);

    for (StatusNotifierItem *item : host->items()) {
        connectItem(item);
    }
    rebuild();
}

void StatusNotifierModel::connectItem(StatusNotifierItem *item)
{
    connect(item, &StatusNotifierItem::ready, this, &StatusNotifierModel::rebuild);
    connect(item, &StatusNotifierItem::menuShown, this, [this]() {
        setMenuOpen(true);
    });
    connect(item, &StatusNotifierItem::menuHidden, this, [this]() {
        setMenuOpen(false);
    });
    connect(item, &StatusNotifierItem::menuActionTriggered, this, &StatusNotifierModel::menuActionTriggered);
    connect(item, &StatusNotifierItem::activated, this, &StatusNotifierModel::itemActivated);
}

bool StatusNotifierModel::watcherAvailable() const
{
    return StatusNotifierHost::self()->isWatcherAvailable();
}

bool StatusNotifierModel::accepts(StatusNotifierItem *item) const
{
    if (!item || !item->isReady()) {
        return false;
    }
    if (!m_showPassive && item->status() == u"Passive"_s) {
        return false;
    }
    if (!m_includeExcluded && m_excludedIds.contains(item->configId())) {
        return false;
    }
    return true;
}

void StatusNotifierModel::rebuild()
{
    QList<StatusNotifierItem *> wanted;
    for (StatusNotifierItem *item : StatusNotifierHost::self()->items()) {
        if (accepts(item)) {
            wanted.append(item);
        }
    }

    // Removals first (back to front), then insertions, so delegates of
    // unchanged items survive and the grid does not flicker.
    for (int row = m_rows.size() - 1; row >= 0; --row) {
        StatusNotifierItem *item = m_rows.at(row);
        if (!item || !wanted.contains(item)) {
            beginRemoveRows(QModelIndex(), row, row);
            m_rows.removeAt(row);
            endRemoveRows();
        }
    }
    for (int row = 0; row < wanted.size(); ++row) {
        StatusNotifierItem *item = wanted.at(row);
        if (row < m_rows.size() && m_rows.at(row) == item) {
            continue;
        }
        const qsizetype existing = m_rows.indexOf(item);
        if (existing >= 0) {
            // Order changed; unusual (items keep registration order).
            beginMoveRows(QModelIndex(), int(existing), int(existing), QModelIndex(), row > existing ? row + 1 : row);
            m_rows.move(existing, row);
            endMoveRows();
        } else {
            beginInsertRows(QModelIndex(), row, row);
            m_rows.insert(row, item);
            endInsertRows();
        }
    }
    Q_EMIT countChanged();
}

void StatusNotifierModel::onItemChanged(StatusNotifierItem *item)
{
    const qsizetype row = m_rows.indexOf(item);
    if ((row >= 0) != accepts(item)) {
        rebuild(); // status or id change affects filtering
        return;
    }
    if (row >= 0) {
        const QModelIndex idx = index(int(row));
        Q_EMIT dataChanged(idx, idx);
    }
}

void StatusNotifierModel::setExcludedIds(const QStringList &ids)
{
    if (m_excludedIds == ids) {
        return;
    }
    m_excludedIds = ids;
    Q_EMIT excludedIdsChanged();
    if (m_includeExcluded) {
        if (!m_rows.isEmpty()) {
            Q_EMIT dataChanged(index(0), index(int(m_rows.size()) - 1), {ExcludedRole});
        }
    } else {
        rebuild();
    }
}

void StatusNotifierModel::setShowPassive(bool show)
{
    if (m_showPassive == show) {
        return;
    }
    m_showPassive = show;
    Q_EMIT showPassiveChanged();
    rebuild();
}

void StatusNotifierModel::setIncludeExcluded(bool include)
{
    if (m_includeExcluded == include) {
        return;
    }
    m_includeExcluded = include;
    Q_EMIT includeExcludedChanged();
    rebuild();
}

void StatusNotifierModel::setMenuOpen(bool open)
{
    if (m_menuOpen == open) {
        return;
    }
    m_menuOpen = open;
    Q_EMIT menuOpenChanged();
}

int StatusNotifierModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_rows.size());
}

StatusNotifierItem *StatusNotifierModel::itemAt(int row) const
{
    if (row < 0 || row >= m_rows.size()) {
        return nullptr;
    }
    return m_rows.at(row);
}

QVariant StatusNotifierModel::data(const QModelIndex &index, int role) const
{
    StatusNotifierItem *item = itemAt(index.row());
    if (!item) {
        return {};
    }
    switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
        return item->title();
    case RegistrationRole:
        return item->registration();
    case ItemIdRole:
        return item->itemId();
    case ConfigIdRole:
        return item->configId();
    case ToolTipTitleRole:
        return item->toolTipTitle();
    case ToolTipSubTitleRole:
        return item->toolTipSubTitle();
    case StatusRole:
        return item->status();
    case CategoryRole:
        return item->category();
    case Qt::DecorationRole:
    case IconSourceRole:
        return item->iconSource();
    case OverlayIconNameRole:
        return item->overlayIconName();
    case ItemIsMenuRole:
        return item->itemIsMenu();
    case HasMenuRole:
        return item->hasMenu();
    case IconOriginRole:
        return item->iconOrigin();
    case ExcludedRole:
        return m_excludedIds.contains(item->configId());
    }
    return {};
}

QHash<int, QByteArray> StatusNotifierModel::roleNames() const
{
    return {
        {Qt::DisplayRole, "display"},
        {RegistrationRole, "registration"},
        {ItemIdRole, "itemId"},
        {ConfigIdRole, "configId"},
        {TitleRole, "title"},
        {ToolTipTitleRole, "toolTipTitle"},
        {ToolTipSubTitleRole, "toolTipSubTitle"},
        {StatusRole, "status"},
        {CategoryRole, "category"},
        {IconSourceRole, "iconSource"},
        {OverlayIconNameRole, "overlayIconName"},
        {ItemIsMenuRole, "itemIsMenu"},
        {HasMenuRole, "hasMenu"},
        {IconOriginRole, "iconOrigin"},
        {ExcludedRole, "excluded"},
    };
}

void StatusNotifierModel::activate(int row, QQuickItem *item, qreal x, qreal y)
{
    StatusNotifierItem *sni = itemAt(row);
    if (!sni) {
        return;
    }
    if (sni->itemIsMenu() && sni->hasMenu()) {
        sni->showContextMenu(item, QPointF(x, y));
        return;
    }
    sni->activate(item, QPointF(x, y));
}

void StatusNotifierModel::secondaryActivate(int row, QQuickItem *item, qreal x, qreal y)
{
    if (StatusNotifierItem *sni = itemAt(row)) {
        sni->secondaryActivate(item, QPointF(x, y));
    }
}

void StatusNotifierModel::showContextMenu(int row, QQuickItem *item, qreal x, qreal y)
{
    if (StatusNotifierItem *sni = itemAt(row)) {
        sni->showContextMenu(item, QPointF(x, y));
    }
}

void StatusNotifierModel::scroll(int row, int delta, bool horizontal)
{
    if (StatusNotifierItem *sni = itemAt(row)) {
        sni->scroll(delta, horizontal ? Qt::Horizontal : Qt::Vertical);
    }
}
