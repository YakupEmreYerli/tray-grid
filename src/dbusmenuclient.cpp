// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "dbusmenuclient.h"
#include "popuphelper.h"

#include <QActionGroup>
#include <QDBusArgument>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDBusVariant>
#include <QDateTime>
#include <QIcon>
#include <QLoggingCategory>
#include <QMenu>
#include <QPixmap>
#include <QQuickItem>
#include <QQuickWindow>

Q_LOGGING_CATEGORY(TRAYGRID_MENU, "traygrid.dbusmenu", QtWarningMsg)

using namespace Qt::StringLiterals;

static const QString s_menuInterface = u"com.canonical.dbusmenu"_s;

DBusMenuClient::DBusMenuClient(const QString &service, const QString &path, QObject *parent)
    : QObject(parent)
    , m_service(service)
    , m_path(path)
{
}

DBusMenuClient::~DBusMenuClient()
{
    delete m_menu.data();
}

QString DBusMenuClient::service() const
{
    return m_service;
}

QString DBusMenuClient::path() const
{
    return m_path;
}

bool DBusMenuClient::isOpen() const
{
    return m_menu && m_menu->isVisible();
}

void DBusMenuClient::close()
{
    if (m_menu) {
        m_menu->close();
    }
}

DBusMenuClient::Node DBusMenuClient::parseLayout(const QDBusArgument &arg)
{
    Node node;
    arg.beginStructure();
    arg >> node.id >> node.properties;
    arg.beginArray();
    while (!arg.atEnd()) {
        QDBusVariant childVariant;
        arg >> childVariant;
        const QDBusArgument childArg = childVariant.variant().value<QDBusArgument>();
        node.children.append(parseLayout(childArg));
    }
    arg.endArray();
    arg.endStructure();
    return node;
}

QString DBusMenuClient::convertMnemonic(const QString &label)
{
    // dbusmenu: '_' marks the mnemonic, "__" is a literal underscore.
    // QAction: '&' marks the mnemonic, "&&" is a literal ampersand.
    QString result;
    result.reserve(label.size() + 4);
    for (qsizetype i = 0; i < label.size(); ++i) {
        const QChar c = label.at(i);
        if (c == u'&') {
            result += u"&&"_s;
        } else if (c == u'_') {
            if (i + 1 < label.size() && label.at(i + 1) == u'_') {
                result += u'_';
                ++i;
            } else {
                result += u'&';
            }
        } else {
            result += c;
        }
    }
    return result;
}

static QDBusMessage layoutCall(const QString &service, const QString &path, int parentId)
{
    QDBusMessage call = QDBusMessage::createMethodCall(service, path, s_menuInterface, u"GetLayout"_s);
    call << parentId << -1 << QStringList();
    return call;
}

bool DBusMenuClient::fetchLayoutBlocking(Node *root, QString *error)
{
    QDBusMessage aboutToShow = QDBusMessage::createMethodCall(m_service, m_path, s_menuInterface, u"AboutToShow"_s);
    aboutToShow << 0;
    QDBusConnection::sessionBus().call(aboutToShow, QDBus::Block, 2000);

    const QDBusMessage reply = QDBusConnection::sessionBus().call(layoutCall(m_service, m_path, 0), QDBus::Block, 5000);
    if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().size() < 2) {
        if (error) {
            *error = reply.errorMessage();
        }
        return false;
    }
    *root = parseLayout(reply.arguments().at(1).value<QDBusArgument>());
    return true;
}

void DBusMenuClient::popup(QQuickItem *item, const QPointF &localPos)
{
    m_anchorItem = item;
    const int serial = ++m_requestSerial;

    // Give the application a chance to update the menu first. Some
    // implementations do not implement AboutToShow; that's fine.
    QDBusMessage aboutToShow = QDBusMessage::createMethodCall(m_service, m_path, s_menuInterface, u"AboutToShow"_s);
    aboutToShow << 0;
    auto *aboutWatcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(aboutToShow, 1000), this);

    connect(aboutWatcher, &QDBusPendingCallWatcher::finished, this, [this, serial, localPos](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        if (serial != m_requestSerial) {
            return;
        }
        auto *layoutWatcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(layoutCall(m_service, m_path, 0), 5000), this);
        connect(layoutWatcher, &QDBusPendingCallWatcher::finished, this, [this, serial, localPos](QDBusPendingCallWatcher *lw) {
            lw->deleteLater();
            if (serial != m_requestSerial) {
                return;
            }
            const QDBusMessage reply = lw->reply();
            if (reply.type() != QDBusMessage::ReplyMessage || reply.arguments().size() < 2) {
                qCWarning(TRAYGRID_MENU) << "GetLayout failed for" << m_service << m_path << reply.errorMessage();
                Q_EMIT failed(reply.errorMessage());
                return;
            }
            const Node root = parseLayout(reply.arguments().at(1).value<QDBusArgument>());

            if (m_menu) {
                m_menu->close();
                m_menu->deleteLater();
            }
            m_menu = new QMenu();
            m_menu->setAttribute(Qt::WA_DeleteOnClose, false);
            fillMenu(m_menu, root);
            if (m_menu->isEmpty()) {
                Q_EMIT failed(u"empty menu"_s);
                return;
            }

            QMenu *menu = m_menu;
            connect(menu, &QMenu::aboutToHide, this, [this, menu]() {
                sendEvent(0, u"closed"_s);
                Q_EMIT menuHidden();
                menu->deleteLater();
            });
            sendEvent(0, u"opened"_s);
            if (!m_anchorItem) {
                return;
            }
            PopupHelper::showMenu(menu, m_anchorItem, localPos);
            Q_EMIT menuShown();
        });
    });
}

void DBusMenuClient::fillMenu(QMenu *menu, const Node &node)
{
    QActionGroup *radioGroup = nullptr;

    for (const Node &child : node.children) {
        const QVariantMap &p = child.properties;
        if (!p.value(u"visible"_s, true).toBool()) {
            continue;
        }
        if (p.value(u"type"_s).toString() == u"separator"_s) {
            menu->addSeparator();
            radioGroup = nullptr;
            continue;
        }

        const QString text = convertMnemonic(p.value(u"label"_s).toString());
        QAction *action = nullptr;

        const bool isSubmenu = p.value(u"children-display"_s).toString() == u"submenu"_s || !child.children.isEmpty();
        if (isSubmenu) {
            QMenu *sub = menu->addMenu(text);
            action = sub->menuAction();
            fillMenu(sub, child);
            const int id = child.id;
            connect(sub, &QMenu::aboutToShow, this, [this, sub, id]() {
                sendEvent(id, u"opened"_s);
                refreshSubmenu(sub, id);
            });
            connect(sub, &QMenu::aboutToHide, this, [this, id]() {
                sendEvent(id, u"closed"_s);
            });
        } else {
            action = menu->addAction(text);
            const int id = child.id;
            connect(action, &QAction::triggered, this, [this, id]() {
                triggered(id);
            });
        }

        action->setEnabled(p.value(u"enabled"_s, true).toBool());

        const QString iconName = p.value(u"icon-name"_s).toString();
        if (!iconName.isEmpty()) {
            action->setIcon(QIcon::fromTheme(iconName));
        } else {
            const QByteArray iconData = p.value(u"icon-data"_s).toByteArray();
            if (!iconData.isEmpty()) {
                QPixmap pixmap;
                if (pixmap.loadFromData(iconData)) {
                    action->setIcon(QIcon(pixmap));
                }
            }
        }

        const QString toggleType = p.value(u"toggle-type"_s).toString();
        if (toggleType == u"checkmark"_s || toggleType == u"radio"_s) {
            action->setCheckable(true);
            action->setChecked(p.value(u"toggle-state"_s).toInt() == 1);
            if (toggleType == u"radio"_s) {
                if (!radioGroup) {
                    radioGroup = new QActionGroup(menu);
                    radioGroup->setExclusive(true);
                }
                radioGroup->addAction(action);
            } else {
                radioGroup = nullptr;
            }
        } else {
            radioGroup = nullptr;
        }
    }
}

void DBusMenuClient::refreshSubmenu(QMenu *submenu, int id)
{
    QPointer<QMenu> guard(submenu);
    QDBusMessage aboutToShow = QDBusMessage::createMethodCall(m_service, m_path, s_menuInterface, u"AboutToShow"_s);
    aboutToShow << id;
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(aboutToShow, 1000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, guard, id](QDBusPendingCallWatcher *w) {
        w->deleteLater();
        QDBusPendingReply<bool> reply = *w;
        if (!guard || reply.isError() || !reply.value()) {
            return; // nothing changed (or not supported)
        }
        auto *layoutWatcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(layoutCall(m_service, m_path, id), 5000), this);
        connect(layoutWatcher, &QDBusPendingCallWatcher::finished, this, [this, guard](QDBusPendingCallWatcher *lw) {
            lw->deleteLater();
            const QDBusMessage msg = lw->reply();
            if (!guard || msg.type() != QDBusMessage::ReplyMessage || msg.arguments().size() < 2) {
                return;
            }
            const Node node = parseLayout(msg.arguments().at(1).value<QDBusArgument>());
            guard->clear();
            fillMenu(guard, node);
        });
    });
}

void DBusMenuClient::sendEvent(int id, const QString &eventId)
{
    QDBusMessage event = QDBusMessage::createMethodCall(m_service, m_path, s_menuInterface, u"Event"_s);
    event << id << eventId << QVariant::fromValue(QDBusVariant(QString())) << uint(QDateTime::currentSecsSinceEpoch());
    QDBusConnection::sessionBus().asyncCall(event);
}

void DBusMenuClient::sendClicked(int id)
{
    sendEvent(id, u"clicked"_s);
}

void DBusMenuClient::triggered(int id)
{
    Q_EMIT actionTriggered(id);
    QWindow *window = m_anchorItem ? m_anchorItem->window() : nullptr;
    // Hand the application an activation token first so that entries like
    // "Show window" may raise it on Wayland.
    PopupHelper::withActivationToken(window, this, [this, id](const QString &token) {
        if (!token.isEmpty()) {
            Q_EMIT activationTokenReady(token);
        }
        sendClicked(id);
    });
}
