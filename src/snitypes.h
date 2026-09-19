// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <QByteArray>
#include <QDBusArgument>
#include <QList>
#include <QMetaType>
#include <QString>

// One entry of an IconPixmap / AttentionIconPixmap / OverlayIconPixmap
// property: D-Bus signature (iiay), ARGB32 in network byte order.
struct SniImageData {
    int width = 0;
    int height = 0;
    QByteArray data;
};
using SniImageVector = QList<SniImageData>;

// ToolTip property: D-Bus signature (sa(iiay)ss).
struct SniToolTip {
    QString iconName;
    SniImageVector image;
    QString title;
    QString subTitle;
};

Q_DECLARE_METATYPE(SniImageData)
Q_DECLARE_METATYPE(SniImageVector)
Q_DECLARE_METATYPE(SniToolTip)

inline QDBusArgument &operator<<(QDBusArgument &arg, const SniImageData &image)
{
    arg.beginStructure();
    arg << image.width << image.height << image.data;
    arg.endStructure();
    return arg;
}

inline const QDBusArgument &operator>>(const QDBusArgument &arg, SniImageData &image)
{
    arg.beginStructure();
    arg >> image.width >> image.height >> image.data;
    arg.endStructure();
    return arg;
}

inline QDBusArgument &operator<<(QDBusArgument &arg, const SniToolTip &tip)
{
    arg.beginStructure();
    arg << tip.iconName << tip.image << tip.title << tip.subTitle;
    arg.endStructure();
    return arg;
}

inline const QDBusArgument &operator>>(const QDBusArgument &arg, SniToolTip &tip)
{
    arg.beginStructure();
    arg >> tip.iconName >> tip.image >> tip.title >> tip.subTitle;
    arg.endStructure();
    return arg;
}

namespace SniTypes
{
void registerMetaTypes();
}
