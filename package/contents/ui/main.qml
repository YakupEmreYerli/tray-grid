// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick

import org.kde.plasma.plasmoid
import org.kde.plasma.core as PlasmaCore

// The C++ part (StatusNotifierHost + DBusMenu client) ships inside this
// package; see src/ and the README for why it is a directory import.
import "../lib" as TrayGrid

PlasmoidItem {
    id: root

    readonly property alias trayModel: trayModel

    TrayGrid.StatusNotifierModel {
        id: trayModel
        excludedIds: Plasmoid.configuration.excludedIds
        showPassive: Plasmoid.configuration.showPassive

        onItemActivated: root.closeAfterAction()
        onMenuActionTriggered: root.closeAfterAction()
    }

    function closeAfterAction() {
        if (Plasmoid.configuration.closeOnActivate) {
            Qt.callLater(() => { root.expanded = false; });
        }
    }

    preferredRepresentation: compactRepresentation
    compactRepresentation: CompactRepresentation {}
    fullRepresentation: FullRepresentation {}

    // Keep the popup open while one of the items' context menus is shown:
    // the menu is a separate popup window and takes the focus.
    hideOnWindowDeactivate: !trayModel.menuOpen

    Plasmoid.status: trayModel.count > 0 || expanded ? PlasmaCore.Types.ActiveStatus : PlasmaCore.Types.PassiveStatus
    Plasmoid.icon: "arrow-up-symbolic"

    toolTipMainText: i18nc("@info:tooltip", "Hidden icons")
    toolTipSubText: trayModel.count > 0
        ? i18ncp("@info:tooltip", "%1 application in the tray", "%1 applications in the tray", trayModel.count)
        : i18nc("@info:tooltip", "No applications in the tray")
}
