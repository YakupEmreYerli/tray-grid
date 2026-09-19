// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// Usage:
//   sni-dump                      list registered items (via our own host)
//   sni-dump --menus              ... and print their dbusmenu layouts
//   sni-dump --icons DIR          ... and save each resolved icon as PNG
//   sni-dump --watch SECONDS      keep running and print live changes

#include "dbusmenuclient.h"
#include "statusnotifierhost.h"
#include "statusnotifieritem.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDir>
#include <QIcon>
#include <QTextStream>
#include <QTimer>

static QTextStream out(stdout);

static void printMenu(const DBusMenuClient::Node &node, int depth)
{
    for (const auto &child : node.children) {
        const auto &p = child.properties;
        if (!p.value(QStringLiteral("visible"), true).toBool()) {
            continue;
        }
        out << QString(depth * 2 + 4, QLatin1Char(' '));
        if (p.value(QStringLiteral("type")).toString() == QLatin1String("separator")) {
            out << "----\n";
            continue;
        }
        out << "[" << child.id << "] " << p.value(QStringLiteral("label")).toString();
        if (!p.value(QStringLiteral("enabled"), true).toBool()) {
            out << " (disabled)";
        }
        const QString toggle = p.value(QStringLiteral("toggle-type")).toString();
        if (!toggle.isEmpty()) {
            out << " (" << toggle << "=" << p.value(QStringLiteral("toggle-state")).toInt() << ")";
        }
        out << "\n";
        printMenu(child, depth + 1);
    }
}

static void describe(StatusNotifierItem *item, bool menus, const QString &iconDir)
{
    out << item->registration() << "\n";
    out << "    id=" << item->itemId() << "  title=" << item->title() << "  status=" << item->status() << "  category=" << item->category() << "\n";
    out << "    tooltip=" << item->toolTipTitle() << " | " << item->toolTipSubTitle().left(80) << "\n";
    out << "    icon=" << item->iconOrigin() << "  itemIsMenu=" << item->itemIsMenu() << "  menu=" << item->menuPath() << "\n";
    if (!iconDir.isEmpty()) {
        const QVariant source = item->iconSource();
        QIcon icon = source.typeId() == QMetaType::QString ? QIcon::fromTheme(source.toString()) : source.value<QIcon>();
        QString name = item->itemId();
        name.replace(QLatin1Char('/'), QLatin1Char('_'));
        const QString file = QDir(iconDir).filePath(name + QStringLiteral(".png"));
        const bool ok = icon.pixmap(48, 48).save(file);
        out << "    saved " << file << (ok ? "" : " (FAILED)") << "\n";
    }
    if (menus && item->hasMenu()) {
        DBusMenuClient client(item->service(), item->menuPath());
        DBusMenuClient::Node root;
        QString error;
        if (client.fetchLayoutBlocking(&root, &error)) {
            printMenu(root, 0);
        } else {
            out << "    menu error: " << error << "\n";
        }
    }
    out.flush();
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption menusOpt(QStringLiteral("menus"), QStringLiteral("Print dbusmenu layouts"));
    QCommandLineOption iconsOpt(QStringLiteral("icons"), QStringLiteral("Save icons into DIR"), QStringLiteral("DIR"));
    QCommandLineOption watchOpt(QStringLiteral("watch"), QStringLiteral("Watch changes for N seconds"), QStringLiteral("SECONDS"));
    QCommandLineOption clickOpt(QStringLiteral("click"), QStringLiteral("Send Event(ID,\"clicked\") to the menu of item ITEMID"), QStringLiteral("ITEMID:ID"));
    parser.addOptions({menusOpt, iconsOpt, watchOpt, clickOpt});
    parser.process(app);

    const bool menus = parser.isSet(menusOpt);
    const QString iconDir = parser.value(iconsOpt);
    if (!iconDir.isEmpty()) {
        QDir().mkpath(iconDir);
    }
    const int watchSeconds = parser.value(watchOpt).toInt();

    StatusNotifierHost *host = StatusNotifierHost::self();
    out << "host service: " << host->hostServiceName() << "\n";

    // Give the host a moment to fetch the list and every item's properties.
    QTimer::singleShot(1500, &app, [&]() {
        out << "watcher available: " << host->isWatcherAvailable() << ", items: " << host->items().size() << "\n\n";
        for (StatusNotifierItem *item : host->items()) {
            describe(item, menus, iconDir);
        }
        if (parser.isSet(clickOpt)) {
            const QString spec = parser.value(clickOpt);
            const qsizetype colon = spec.lastIndexOf(QLatin1Char(':'));
            const QString itemId = spec.left(colon);
            const int id = spec.mid(colon + 1).toInt();
            for (StatusNotifierItem *item : host->items()) {
                if (item->itemId() == itemId && item->menuClient()) {
                    out << "clicking " << id << " in " << itemId << "\n";
                    item->menuClient()->sendClicked(id);
                }
            }
            out.flush();
        }
        if (watchSeconds <= 0) {
            QTimer::singleShot(300, &app, &QCoreApplication::quit);
            return;
        }
        out << "watching for " << watchSeconds << "s...\n";
        out.flush();
        QObject::connect(host, &StatusNotifierHost::itemAdded, &app, [](StatusNotifierItem *item) {
            QObject::connect(item, &StatusNotifierItem::ready, item, [item]() {
                out << "+ ADDED ";
                describe(item, false, QString());
            });
        });
        QObject::connect(host, &StatusNotifierHost::itemRemoved, &app, [](const QString &registration) {
            out << "- REMOVED " << registration << "\n";
            out.flush();
        });
        QObject::connect(host, &StatusNotifierHost::itemChanged, &app, [](StatusNotifierItem *item) {
            out << "* CHANGED " << item->itemId() << " status=" << item->status() << " icon=" << item->iconOrigin() << " tooltip=" << item->toolTipTitle() << "\n";
            out.flush();
        });
        QTimer::singleShot(watchSeconds * 1000, &app, &QCoreApplication::quit);
    });

    return app.exec();
}
