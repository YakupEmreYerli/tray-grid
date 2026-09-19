// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.plasma.extras as PlasmaExtras
import org.kde.plasma.plasmoid

// The popup: icons only, in a fixed number of columns.
Item {
    id: full

    readonly property int columns: Math.max(1, Plasmoid.configuration.columns)
    readonly property int iconSize: {
        switch (Plasmoid.configuration.iconSize) {
        case 0:
            return Kirigami.Units.iconSizes.small;
        case 2:
            return Kirigami.Units.iconSizes.medium;
        default:
            return Kirigami.Units.iconSizes.smallMedium;
        }
    }
    readonly property int cellSize: iconSize + Kirigami.Units.largeSpacing * 2
    readonly property int visibleColumns: Math.max(1, Math.min(columns, grid.count))
    readonly property int rows: Math.max(1, Math.ceil(grid.count / columns))
    readonly property bool empty: grid.count === 0

    readonly property int contentWidth: empty ? Kirigami.Units.gridUnit * 12 : visibleColumns * cellSize
    readonly property int contentHeight: empty ? Kirigami.Units.gridUnit * 6 : rows * cellSize

    Layout.minimumWidth: contentWidth
    Layout.preferredWidth: contentWidth
    Layout.maximumWidth: contentWidth
    Layout.minimumHeight: contentHeight
    Layout.preferredHeight: contentHeight
    Layout.maximumHeight: contentHeight

    // Reset keyboard selection whenever the popup opens.
    Connections {
        target: root
        function onExpandedChanged() {
            if (root.expanded) {
                grid.currentIndex = -1;
                grid.forceActiveFocus();
            }
        }
    }

    GridView {
        id: grid
        anchors.centerIn: parent
        width: full.visibleColumns * full.cellSize
        height: full.rows * full.cellSize
        visible: !full.empty

        model: root.trayModel
        cellWidth: full.cellSize
        cellHeight: full.cellSize
        interactive: false
        currentIndex: -1
        keyNavigationEnabled: true
        keyNavigationWraps: true
        activeFocusOnTab: true
        highlightFollowsCurrentItem: true
        highlightMoveDuration: 0
        highlight: PlasmaExtras.Highlight {
            visible: grid.activeFocus && grid.currentIndex >= 0
        }

        Accessible.role: Accessible.Grouping
        Accessible.name: i18nc("@info:whatsthis", "Tray applications")

        delegate: TrayIconDelegate {
            width: grid.cellWidth
            height: grid.cellHeight
            iconSize: full.iconSize
        }
    }

    PlasmaExtras.PlaceholderMessage {
        anchors.centerIn: parent
        width: parent.width - Kirigami.Units.largeSpacing * 2
        visible: full.empty
        iconName: "preferences-desktop-notification"
        text: root.trayModel.watcherAvailable
            ? i18nc("@info", "No tray applications")
            : i18nc("@info", "The status notifier service is not running")
    }
}
