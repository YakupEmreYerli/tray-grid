// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

// A StatusNotifierItem for testing tray-grid without touching real apps.
// Prints one line per event it receives on stdout:
//   READY / ACTIVATE x y / SECONDARY x y / SCROLL delta orientation /
//   MENU <action text> / TOGGLE <state>
//
//   test-item [--id ID] [--pixmap] [--cycle MS] [--quit-after SECONDS]
//             [--title TEXT] [--icon-file PATH]   (the last two stage README screenshots)

#include <KStatusNotifierItem>

#include <QApplication>
#include <QCommandLineParser>
#include <QImage>
#include <QMenu>
#include <QPainter>
#include <QTextStream>
#include <QTimer>

static QTextStream out(stdout);

static void emitLine(const QString &line)
{
    out << line << Qt::endl;
}

static QImage solidImage(const QColor &color)
{
    QImage image(32, 32, QImage::Format_ARGB32);
    image.fill(Qt::transparent);
    QPainter p(&image);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawEllipse(2, 2, 28, 28);
    return image;
}

int main(int argc, char **argv)
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.addHelpOption();
    QCommandLineOption idOpt(QStringLiteral("id"), QStringLiteral("Item id"), QStringLiteral("ID"), QStringLiteral("traygrid-test-item"));
    QCommandLineOption pixmapOpt(QStringLiteral("pixmap"), QStringLiteral("Publish IconPixmap instead of IconName"));
    QCommandLineOption cycleOpt(QStringLiteral("cycle"), QStringLiteral("Change the icon every MS milliseconds"), QStringLiteral("MS"));
    QCommandLineOption quitOpt(QStringLiteral("quit-after"), QStringLiteral("Exit after SECONDS"), QStringLiteral("SECONDS"));
    QCommandLineOption titleOpt(QStringLiteral("title"), QStringLiteral("Title and tooltip"), QStringLiteral("TEXT"));
    QCommandLineOption iconFileOpt(QStringLiteral("icon-file"), QStringLiteral("Publish this image file as the icon pixmap"), QStringLiteral("PATH"));
    parser.addOptions({idOpt, pixmapOpt, cycleOpt, quitOpt, titleOpt, iconFileOpt});
    parser.process(app);

    const bool pixmap = parser.isSet(pixmapOpt);
    auto *sni = new KStatusNotifierItem(parser.value(idOpt), &app);
    const QString title = parser.isSet(titleOpt) ? parser.value(titleOpt) : QStringLiteral("tray-grid test item");
    sni->setTitle(title);
    sni->setToolTip(QStringLiteral("dialog-information"), title, QStringLiteral("Tooltip <b>subtitle</b>"));
    sni->setCategory(KStatusNotifierItem::ApplicationStatus);
    sni->setStatus(KStatusNotifierItem::Active);
    sni->setStandardActionsEnabled(false);

    const QStringList names = {QStringLiteral("dialog-information"), QStringLiteral("dialog-warning")};
    const QList<QColor> colors = {QColor(0x3d, 0xae, 0xe9), QColor(0xda, 0x44, 0x53)};
    int phase = 0;
    auto applyIcon = [&]() {
        if (parser.isSet(iconFileOpt)) {
            sni->setIconByPixmap(QIcon(parser.value(iconFileOpt)));
        } else if (pixmap) {
            sni->setIconByPixmap(QIcon(QPixmap::fromImage(solidImage(colors.at(phase)))));
        } else {
            sni->setIconByName(names.at(phase));
        }
        emitLine(QStringLiteral("ICON %1").arg(phase));
    };
    applyIcon();

    auto *menu = new QMenu();
    QObject::connect(menu->addAction(QStringLiteral("Test &action")), &QAction::triggered, [] {
        emitLine(QStringLiteral("MENU Test action"));
    });
    QAction *toggle = menu->addAction(QStringLiteral("Toggle"));
    toggle->setCheckable(true);
    QObject::connect(toggle, &QAction::toggled, [](bool on) {
        emitLine(QStringLiteral("TOGGLE %1").arg(on ? 1 : 0));
    });
    menu->addSeparator();
    QMenu *sub = menu->addMenu(QStringLiteral("Submenu"));
    QObject::connect(sub->addAction(QStringLiteral("Nested action")), &QAction::triggered, [] {
        emitLine(QStringLiteral("MENU Nested action"));
    });
    sni->setContextMenu(menu);

    QObject::connect(sni, &KStatusNotifierItem::activateRequested, [](bool, const QPoint &pos) {
        emitLine(QStringLiteral("ACTIVATE %1 %2").arg(pos.x()).arg(pos.y()));
    });
    QObject::connect(sni, &KStatusNotifierItem::secondaryActivateRequested, [](const QPoint &pos) {
        emitLine(QStringLiteral("SECONDARY %1 %2").arg(pos.x()).arg(pos.y()));
    });
    QObject::connect(sni, &KStatusNotifierItem::scrollRequested, [](int delta, Qt::Orientation o) {
        emitLine(QStringLiteral("SCROLL %1 %2").arg(delta).arg(o == Qt::Horizontal ? QStringLiteral("horizontal") : QStringLiteral("vertical")));
    });

    if (parser.isSet(cycleOpt)) {
        auto *timer = new QTimer(&app);
        timer->setInterval(parser.value(cycleOpt).toInt());
        QObject::connect(timer, &QTimer::timeout, [&]() {
            phase = 1 - phase;
            applyIcon();
        });
        timer->start();
    }
    if (parser.isSet(quitOpt)) {
        QTimer::singleShot(parser.value(quitOpt).toInt() * 1000, &app, &QCoreApplication::quit);
    }

    emitLine(QStringLiteral("READY"));
    return app.exec();
}
