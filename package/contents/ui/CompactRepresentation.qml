// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Layouts

import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.plasmoid

// The panel button: a small arrow pointing away from the panel edge that
// flips when the popup is open, like the expander of Plasma's own tray.
MouseArea {
    id: button

    readonly property bool vertical: Plasmoid.formFactor === PlasmaCore.Types.Vertical
    readonly property int arrowSize: Kirigami.Units.iconSizes.smallMedium

    Layout.minimumWidth: vertical ? 0 : arrowSize
    Layout.preferredWidth: vertical ? -1 : arrowSize
    Layout.maximumWidth: vertical ? Infinity : arrowSize
    Layout.minimumHeight: vertical ? arrowSize : 0
    Layout.preferredHeight: vertical ? arrowSize : -1
    Layout.maximumHeight: vertical ? arrowSize : Infinity

    hoverEnabled: true
    activeFocusOnTab: true

    property bool wasExpanded: false
    onPressed: wasExpanded = root.expanded
    onClicked: root.expanded = !wasExpanded

    Keys.onPressed: event => {
        switch (event.key) {
        case Qt.Key_Space:
        case Qt.Key_Enter:
        case Qt.Key_Return:
        case Qt.Key_Select:
            root.expanded = !root.expanded;
            event.accepted = true;
            break;
        }
    }

    Accessible.name: i18nc("@action:button", "Show hidden icons")
    Accessible.role: Accessible.Button
    Accessible.onPressAction: root.expanded = !root.expanded

    readonly property string outwardIcon: {
        switch (Plasmoid.location) {
        case PlasmaCore.Types.TopEdge:
            return "arrow-down-symbolic";
        case PlasmaCore.Types.LeftEdge:
            return "arrow-right-symbolic";
        case PlasmaCore.Types.RightEdge:
            return "arrow-left-symbolic";
        default:
            return "arrow-up-symbolic";
        }
    }

    Kirigami.Icon {
        anchors.centerIn: parent
        width: button.arrowSize
        height: button.arrowSize
        source: button.outwardIcon
        active: button.containsMouse || button.activeFocus
        rotation: root.expanded ? 180 : 0
        Behavior on rotation {
            RotationAnimation {
                duration: Kirigami.Units.shortDuration
                easing.type: Easing.InOutQuad
            }
        }
    }
}
