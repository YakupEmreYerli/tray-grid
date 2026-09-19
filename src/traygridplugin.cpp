// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "statusnotifiermodel.h"

#include <QQmlExtensionPlugin>
#include <QtQml>

// Registered under whatever URI the importing directory resolves to; see
// the comment in the top-level CMakeLists.txt.
class TrayGridPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QQmlExtensionInterface_iid)

public:
    void registerTypes(const char *uri) override
    {
        qmlRegisterType<StatusNotifierModel>(uri, 1, 0, "StatusNotifierModel");
    }
};

#include "traygridplugin.moc"
