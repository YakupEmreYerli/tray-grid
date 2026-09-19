// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick

import org.kde.kirigami as Kirigami
import org.kde.plasma.core as PlasmaCore
import org.kde.plasma.extras as PlasmaExtras

// One cell: the icon, a hover highlight and a tooltip. No text label.
PlasmaCore.ToolTipArea {
    id: cell

    required property int index
    required property var model
    property int iconSize: Kirigami.Units.iconSizes.smallMedium

    readonly property string tipTitle: model.toolTipTitle || model.title || ""
    readonly property string tipText: model.toolTipSubTitle !== tipTitle ? (model.toolTipSubTitle || "") : ""

    mainText: tipTitle
    subText: tipText
    textFormat: Text.AutoText
    location: PlasmaCore.Types.Floating
    active: tipTitle.length > 0 || tipText.length > 0

    Accessible.name: tipTitle
    Accessible.description: tipText
    Accessible.role: Accessible.Button
    Accessible.onPressAction: activate(width / 2, height / 2)

    function activate(x, y) {
        root.trayModel.activate(index, cell, x, y);
    }
    function contextMenu(x, y) {
        cell.hideImmediately();
        root.trayModel.showContextMenu(index, cell, x, y);
    }

    Keys.onPressed: event => {
        switch (event.key) {
        case Qt.Key_Space:
        case Qt.Key_Enter:
        case Qt.Key_Return:
        case Qt.Key_Select:
            activate(width / 2, height / 2);
            event.accepted = true;
            break;
        case Qt.Key_Menu:
            contextMenu(width / 2, height / 2);
            event.accepted = true;
            break;
        }
    }

    PlasmaExtras.Highlight {
        anchors.fill: parent
        hovered: true
        pressed: mouse.pressed
        visible: mouse.containsMouse
    }

    MouseArea {
        id: mouse
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.LeftButton | Qt.MiddleButton | Qt.RightButton

        onClicked: event => {
            switch (event.button) {
            case Qt.LeftButton:
                cell.activate(event.x, event.y);
                break;
            case Qt.MiddleButton:
                root.trayModel.secondaryActivate(cell.index, cell, event.x, event.y);
                break;
            case Qt.RightButton:
                cell.contextMenu(event.x, event.y);
                break;
            }
        }

        onWheel: wheel => {
            if (wheel.angleDelta.y !== 0) {
                root.trayModel.scroll(cell.index, wheel.angleDelta.y, false);
            }
            if (wheel.angleDelta.x !== 0) {
                root.trayModel.scroll(cell.index, wheel.angleDelta.x, true);
            }
        }
    }

    Kirigami.Icon {
        id: icon
        anchors.centerIn: parent
        width: cell.iconSize
        height: cell.iconSize
        source: cell.model.iconSource
        active: mouse.containsMouse
        animated: false

        Kirigami.Icon {
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: Math.round(parent.width / 2)
            height: width
            visible: source !== ""
            source: cell.model.overlayIconName || ""
        }
    }
}
