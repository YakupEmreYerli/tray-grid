// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// End-to-end test of the installed plasmoid package against a controllable
// StatusNotifierItem (traygrid-test-item):
//   - the plugin loads through the package's directory import,
//   - the item appears in the grid with its icon, and icon changes are live,
//   - left / middle click and the wheel reach Activate / SecondaryActivate /
//     Scroll,
//   - right click builds the dbusmenu as a QMenu; triggering entries
//     (top level, checkable, nested) reaches the application,
//   - the item disappears from the grid when the application exits.
// Meant to run inside `dbus-run-session`; it provides its own
// StatusNotifierWatcher there so the user's tray is never touched.
//
//   gridtest PACKAGE_DIR TEST_ITEM_BINARY [OUTPUT_DIR]
//   gridtest --preview PACKAGE_DIR OUTPUT_PNG
//       renders the grid of the items registered on the *current* session
//       bus (read-only: nothing is clicked) and lists them.

#include "mockwatcher.h"

#include <QApplication>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDir>
#include <QElapsedTimer>
#include <QMenu>
#include <QProcess>
#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTest>
#include <QTextStream>

static QTextStream out(stdout);
static int failures = 0;

static void check(bool ok, const QString &what)
{
    out << (ok ? "PASS " : "FAIL ") << what << Qt::endl;
    if (!ok) {
        ++failures;
    }
}

static bool waitFor(const std::function<bool()> &predicate, int timeoutMs = 5000)
{
    QElapsedTimer timer;
    timer.start();
    while (!predicate()) {
        if (timer.elapsed() > timeoutMs) {
            return false;
        }
        QTest::qWait(20);
    }
    return true;
}

class TestItem
{
public:
    explicit TestItem(const QString &binary, const QStringList &args)
    {
        m_process.setProgram(binary);
        m_process.setArguments(args);
        m_process.setProcessChannelMode(QProcess::ForwardedErrorChannel);
        m_process.start();
    }
    ~TestItem()
    {
        stop();
    }
    void stop()
    {
        if (m_process.state() != QProcess::NotRunning) {
            m_process.terminate();
            m_process.waitForFinished(3000);
        }
    }
    bool waitLine(const QString &prefix, int timeoutMs = 5000)
    {
        return waitFor(
            [&]() {
                m_buffer += QString::fromUtf8(m_process.readAllStandardOutput());
                const QStringList lines = m_buffer.split(QLatin1Char('\n'));
                for (qsizetype i = 0; i < lines.size() - 1; ++i) {
                    if (lines.at(i).startsWith(prefix)) {
                        m_buffer = lines.mid(i + 1).join(QLatin1Char('\n'));
                        return true;
                    }
                }
                return false;
            },
            timeoutMs);
    }

private:
    QProcess m_process;
    QString m_buffer;
};

// Reads a model role through the delegate's "model" object.
static QVariant role(QQuickItem *cell, const char *name)
{
    if (!cell) {
        return {};
    }
    QObject *model = cell->property("model").value<QObject *>();
    return model ? model->property(name) : QVariant();
}

static QMenu *openMenu()
{
    for (QWidget *w : QApplication::topLevelWidgets()) {
        if (auto *menu = qobject_cast<QMenu *>(w); menu && menu->isVisible()) {
            return menu;
        }
    }
    return nullptr;
}

static QAction *findAction(QMenu *menu, const QString &text)
{
    for (QAction *action : menu->actions()) {
        if (action->text().remove(QLatin1Char('&')) == text) {
            return action;
        }
    }
    return nullptr;
}

int main(int argc, char **argv)
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
    QApplication app(argc, argv);
    QStringList args = app.arguments();
    const bool preview = args.size() > 1 && args.at(1) == QStringLiteral("--preview");
    if (preview) {
        args.removeAt(1);
    }
    if (args.size() < 3) {
        out << "usage: gridtest PACKAGE_DIR TEST_ITEM_BINARY [OUTPUT_DIR]" << Qt::endl;
        return 2;
    }
    const QString packageDir = QDir(args.at(1)).absolutePath();
    const QString testItemBinary = args.at(2);
    const QString outputDir = args.size() > 3 ? args.at(3) : QString();

    MockWatcher watcher;
    if (!preview && QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.StatusNotifierWatcher"))) {
        out << "A StatusNotifierWatcher already runs on this bus; run me inside dbus-run-session." << Qt::endl;
        return 2;
    }
    if (!preview) {
        check(watcher.start(), QStringLiteral("mock StatusNotifierWatcher registered"));
    } else if (!QDBusConnection::sessionBus().interface()->isServiceRegistered(QStringLiteral("org.kde.StatusNotifierWatcher"))) {
        // --preview on a private bus (README screenshots): bring our own watcher.
        watcher.start();
    }

    // Same imports as the plasmoid: package ui directory + bundled plugin.
    const QString uiUrl = QUrl::fromLocalFile(packageDir + QStringLiteral("/contents/ui")).toString();
    const QString libUrl = QUrl::fromLocalFile(packageDir + QStringLiteral("/contents/lib")).toString();
    const QByteArray qml = QStringLiteral(R"(
import QtQuick
import org.kde.kirigami as Kirigami
import "%1"
import "%2" as TrayGrid
Rectangle {
    id: root
    property alias trayModel: sniModel
    property bool expanded: true
    width: 4 * 44
    height: 2 * 44
    color: Kirigami.Theme.backgroundColor
    TrayGrid.StatusNotifierModel { id: sniModel }
    Grid {
        id: grid
        columns: 4
        Repeater {
            id: repeater
            model: sniModel
            delegate: TrayIconDelegate { width: 44; height: 44 }
        }
    }
    function cellAt(i) { return repeater.itemAt(i); }
    function cellFor(configId) {
        for (let i = 0; i < repeater.count; ++i) {
            const cell = repeater.itemAt(i);
            if (cell && cell.model.configId === configId) {
                return cell;
            }
        }
        return null;
    }
}
)")
                             .arg(uiUrl, libUrl)
                             .toUtf8();

    QQmlEngine engine;
    QQmlComponent component(&engine);
    component.setData(qml, QUrl::fromLocalFile(QDir::current().filePath(QStringLiteral("gridtest.qml"))));
    auto *rootItem = qobject_cast<QQuickItem *>(component.create());
    check(rootItem != nullptr, QStringLiteral("package QML + bundled plugin load"));
    if (!rootItem) {
        out << component.errorString() << Qt::endl;
        return 1;
    }
    QQuickWindow window;
    window.resize(int(rootItem->width()), int(rootItem->height()));
    rootItem->setParentItem(window.contentItem());
    window.show();

    auto cellFor = [&](const QString &id) -> QQuickItem * {
        QVariant result;
        QMetaObject::invokeMethod(rootItem, "cellFor", Q_RETURN_ARG(QVariant, result), Q_ARG(QVariant, id));
        return qobject_cast<QQuickItem *>(result.value<QObject *>());
    };
    auto cellCenter = [&](QQuickItem *cell) {
        return cell->mapToScene(QPointF(cell->width() / 2, cell->height() / 2)).toPoint();
    };

    if (preview) {
        QTest::qWait(1500);
        const int count = rootItem->property("trayModel").value<QObject *>()->property("count").toInt();
        rootItem->setHeight(44 * qMax(1, (count + 3) / 4));
        window.resize(int(rootItem->width()), int(rootItem->height()));
        QTest::qWait(300);
        for (int i = 0; i < count; ++i) {
            QVariant cell;
            QMetaObject::invokeMethod(rootItem, "cellAt", Q_RETURN_ARG(QVariant, cell), Q_ARG(QVariant, i));
            auto *c = qobject_cast<QQuickItem *>(cell.value<QObject *>());
            out << i << ": " << role(c, "configId").toString() << "  tooltip=" << c->property("tipTitle").toString()
                << "  icon=" << role(c, "iconOrigin").toString() << Qt::endl;
        }
        window.grabWindow().save(args.at(2));
        out << "saved " << args.at(2) << Qt::endl;
        return 0;
    }

    // --- 1: icon-name item with a live icon change -------------------------
    {
        TestItem item(testItemBinary, {QStringLiteral("--id"), QStringLiteral("named"), QStringLiteral("--cycle"), QStringLiteral("400")});
        check(item.waitLine(QStringLiteral("READY")), QStringLiteral("test item started"));
        const QString id = QStringLiteral("traygrid-test-item_named");
        QQuickItem *cell = nullptr;
        check(waitFor([&]() { return (cell = cellFor(id)) != nullptr; }), QStringLiteral("named item appears in the grid"));
        if (!cell) {
            return 1;
        }
        auto currentIcon = [&]() {
            QQuickItem *c = cellFor(id);
            return c ? c->property("tipTitle").toString() + QLatin1Char('|') + role(c, "iconSource").toString() : QString();
        };
        const QString before = currentIcon();
        check(before.contains(QStringLiteral("tray-grid test item")), QStringLiteral("tooltip title comes from the item: ") + before);
        QString after;
        const bool changed = waitFor(
            [&]() {
                after = currentIcon();
                return after != before;
            },
            3000);
        check(changed, QStringLiteral("icon updates live (NewIcon): ") + before + QStringLiteral(" -> ") + after);

        if (!outputDir.isEmpty()) {
            QTest::qWait(100);
            window.grabWindow().save(QDir(outputDir).filePath(QStringLiteral("grid.png")));
        }

        cell = cellFor(id);
        QTest::mouseClick(&window, Qt::LeftButton, {}, cellCenter(cell));
        check(item.waitLine(QStringLiteral("ACTIVATE")), QStringLiteral("left click -> Activate"));
        cell = cellFor(id);
        QTest::mouseClick(&window, Qt::MiddleButton, {}, cellCenter(cell));
        check(item.waitLine(QStringLiteral("SECONDARY")), QStringLiteral("middle click -> SecondaryActivate"));

        cell = cellFor(id);
        const QPoint center = cellCenter(cell);
        QWheelEvent wheel(center, window.mapToGlobal(center), QPoint(), QPoint(0, 120), Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(&window, &wheel);
        check(item.waitLine(QStringLiteral("SCROLL 120 vertical")), QStringLiteral("wheel -> Scroll"));

        // Right click: menu.
        cell = cellFor(id);
        QTest::mouseClick(&window, Qt::RightButton, {}, cellCenter(cell));
        QMenu *menu = nullptr;
        check(waitFor([&]() { return (menu = openMenu()) != nullptr; }), QStringLiteral("right click -> dbusmenu shown as QMenu"));
        if (menu) {
            QStringList texts;
            for (QAction *a : menu->actions()) {
                texts << (a->isSeparator() ? QStringLiteral("----") : a->text());
            }
            check(texts.join(QLatin1Char(',')) == QStringLiteral("Test &action,Toggle,----,Submenu"), QStringLiteral("menu layout: ") + texts.join(QLatin1Char(',')));
            if (!outputDir.isEmpty()) {
                menu->grab().save(QDir(outputDir).filePath(QStringLiteral("menu.png")));
            }
            QAction *toggle = findAction(menu, QStringLiteral("Toggle"));
            check(toggle && toggle->isCheckable() && !toggle->isChecked(), QStringLiteral("checkable entry mapped"));
            QAction *submenu = findAction(menu, QStringLiteral("Submenu"));
            check(submenu && submenu->menu() && findAction(submenu->menu(), QStringLiteral("Nested action")), QStringLiteral("submenu mapped"));

            findAction(menu, QStringLiteral("Test action"))->trigger();
            menu->close();
            check(item.waitLine(QStringLiteral("MENU Test action")), QStringLiteral("menu entry -> Event clicked reaches the app"));
        }

        // Second menu open: toggle + nested entry, and the check state reflects the app's state.
        cell = cellFor(id);
        QTest::mouseClick(&window, Qt::RightButton, {}, cellCenter(cell));
        menu = nullptr;
        waitFor([&]() { return (menu = openMenu()) != nullptr; });
        if (menu) {
            findAction(menu, QStringLiteral("Toggle"))->trigger();
            check(item.waitLine(QStringLiteral("TOGGLE 1")), QStringLiteral("checkable entry toggles in the app"));
            QMenu *sub = findAction(menu, QStringLiteral("Submenu"))->menu();
            findAction(sub, QStringLiteral("Nested action"))->trigger();
            check(item.waitLine(QStringLiteral("MENU Nested action")), QStringLiteral("nested entry reaches the app"));
            menu->close();
        }
        cell = cellFor(id);
        QTest::mouseClick(&window, Qt::RightButton, {}, cellCenter(cell));
        menu = nullptr;
        waitFor([&]() { return (menu = openMenu()) != nullptr; });
        QAction *toggle = menu ? findAction(menu, QStringLiteral("Toggle")) : nullptr;
        check(toggle && toggle->isChecked(), QStringLiteral("menu is re-read on every open (toggle now checked)"));
        if (menu) {
            menu->close();
        }

        item.stop();
        check(waitFor([&]() { return cellFor(id) == nullptr; }), QStringLiteral("item disappears when the app exits"));
    }

    // --- 2: pixmap item --------------------------------------------------
    {
        TestItem item(testItemBinary, {QStringLiteral("--id"), QStringLiteral("pixmap"), QStringLiteral("--pixmap")});
        item.waitLine(QStringLiteral("READY"));
        const QString id = QStringLiteral("traygrid-test-item_pixmap");
        QQuickItem *cell = nullptr;
        check(waitFor([&]() { return (cell = cellFor(id)) != nullptr; }), QStringLiteral("pixmap item appears in the grid"));
        if (cell) {
            const QVariant source = role(cell, "iconSource");
            check(source.metaType() == QMetaType::fromType<QIcon>() && !source.value<QIcon>().isNull(), QStringLiteral("IconPixmap delivered to QML as QIcon"));
            if (!outputDir.isEmpty()) {
                QTest::qWait(100);
                window.grabWindow().save(QDir(outputDir).filePath(QStringLiteral("grid-pixmap.png")));
            }
        }
    }

    out << (failures ? "FAILED: " : "ALL PASSED") << (failures ? QString::number(failures) : QString()) << Qt::endl;
    return failures ? 1 : 0;
}
