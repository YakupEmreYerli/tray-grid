// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "sniimage.h"

#include <QDBusMetaType>
#include <QPixmap>
#include <QtEndian>

void SniTypes::registerMetaTypes()
{
    static bool done = false;
    if (done) {
        return;
    }
    done = true;
    qDBusRegisterMetaType<SniImageData>();
    qDBusRegisterMetaType<SniImageVector>();
    qDBusRegisterMetaType<SniToolTip>();
}

QImage SniImage::toImage(const SniImageData &data)
{
    if (data.width <= 0 || data.height <= 0) {
        return {};
    }
    const qsizetype pixels = qsizetype(data.width) * data.height;
    if (data.data.size() < pixels * 4) {
        return {};
    }

    QImage image(data.width, data.height, QImage::Format_ARGB32);
    if (image.isNull()) {
        return {};
    }
    // The spec mandates network byte order (big endian) ARGB32; QImage's
    // Format_ARGB32 is a host-endian 0xAARRGGBB uint per pixel.
    const auto *src = reinterpret_cast<const uchar *>(data.data.constData());
    for (int y = 0; y < data.height; ++y) {
        auto *line = reinterpret_cast<quint32 *>(image.scanLine(y));
        for (int x = 0; x < data.width; ++x) {
            line[x] = qFromBigEndian<quint32>(src);
            src += 4;
        }
    }
    return image;
}

QIcon SniImage::toIcon(const SniImageVector &vector)
{
    QIcon icon;
    for (const SniImageData &entry : vector) {
        const QImage image = toImage(entry);
        if (!image.isNull()) {
            icon.addPixmap(QPixmap::fromImage(image));
        }
    }
    return icon;
}
