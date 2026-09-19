// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts

import org.kde.kcmutils as KCM
import org.kde.kirigami as Kirigami

import "../lib" as TrayGrid

KCM.SimpleKCM {
    id: page

    property alias cfg_columns: columnsSpin.value
    property alias cfg_iconSize: iconSizeCombo.currentIndex
    property alias cfg_showPassive: showPassiveCheck.checked
    property alias cfg_closeOnActivate: closeCheck.checked
    property var cfg_excludedIds: []

    // Unused defaults the config dialog injects; declared to avoid warnings.
    property int cfg_columnsDefault
    property int cfg_iconSizeDefault
    property bool cfg_showPassiveDefault
    property bool cfg_closeOnActivateDefault
    property var cfg_excludedIdsDefault

    TrayGrid.StatusNotifierModel {
        id: allItems
        includeExcluded: true
        showPassive: true
        excludedIds: page.cfg_excludedIds
    }

    Kirigami.FormLayout {
        QQC2.SpinBox {
            id: columnsSpin
            Kirigami.FormData.label: i18nc("@label:spinbox", "Columns:")
            from: 1
            to: 8
        }

        QQC2.ComboBox {
            id: iconSizeCombo
            Kirigami.FormData.label: i18nc("@label:listbox", "Icon size:")
            model: [
                i18nc("@item:inlistbox icon size", "Small"),
                i18nc("@item:inlistbox icon size", "Small-medium"),
                i18nc("@item:inlistbox icon size", "Medium"),
            ]
        }

        QQC2.CheckBox {
            id: showPassiveCheck
            Kirigami.FormData.label: i18nc("@title:group", "Behavior:")
            text: i18nc("@option:check", "Show items that report themselves as passive")
        }

        QQC2.CheckBox {
            id: closeCheck
            text: i18nc("@option:check", "Close the popup after activating an item")
        }

        Item {
            Kirigami.FormData.isSection: true
        }

        ColumnLayout {
            Kirigami.FormData.label: i18nc("@title:group", "Shown in the grid:")
            Kirigami.FormData.labelAlignment: Qt.AlignTop
            spacing: Kirigami.Units.smallSpacing

            QQC2.Label {
                visible: allItems.count === 0
                text: i18nc("@info", "No tray applications are running right now.")
                opacity: 0.7
            }

            Repeater {
                model: allItems
                delegate: QQC2.CheckBox {
                    required property string configId
                    required property string title
                    required property var iconSource
                    required property bool excluded

                    checked: !excluded
                    text: title || configId
                    icon.name: typeof iconSource === "string" ? iconSource : ""
                    onToggled: {
                        const ids = page.cfg_excludedIds.filter(id => id !== configId);
                        if (!checked) {
                            ids.push(configId);
                        }
                        page.cfg_excludedIds = ids;
                    }
                }
            }

            QQC2.Label {
                Layout.fillWidth: true
                Layout.maximumWidth: Kirigami.Units.gridUnit * 20
                wrapMode: Text.Wrap
                opacity: 0.7
                font: Kirigami.Theme.smallFont
                text: i18nc("@info", "Unchecked applications are hidden from the grid. The list shows the applications that are running now; the setting is remembered for later runs.")
            }
        }
    }
}
