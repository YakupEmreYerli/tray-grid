// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "snitypes.h"

#include <QIcon>
#include <QImage>

namespace SniImage
{
// Converts one (iiay) entry to a QImage. Returns a null image when the
// payload is malformed (size mismatch, zero dimensions).
QImage toImage(const SniImageData &data);

// Builds a multi-size QIcon from an IconPixmap vector.
QIcon toIcon(const SniImageVector &vector);
}
