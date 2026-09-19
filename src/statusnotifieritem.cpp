// SPDX-FileCopyrightText: 2026 Yakup Emre Yerli <yakupemreyerli0@gmail.com>
// SPDX-License-Identifier: GPL-2.0-or-later

#include "statusnotifieritem.h"
#include "dbusmenuclient.h"
#include "popuphelper.h"
#include "sniimage.h"
#include "snitypes.h"

#include <KIconEngine>
#include <KIconLoader>

#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusMessage>
#include <QDBusObjectPath>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>
#include <QQuickItem>
#include <QQuickWindow>

Q_LOGGING_CATEGORY(TRAYGRID_SNI, "traygrid.sni", QtWarningMsg)

using namespace Qt::StringLiterals;

static const QString s_kdeInterface = u"org.kde.StatusNotifierItem"_s;
static const QString s_fdoInterface = u"org.freedesktop.StatusNotifierItem"_s;

StatusNotifierItem::StatusNotifierItem(const QString &registration, QObject *parent)
    : QObject(parent)
    , m_registration(registration)
    , m_interface(s_kdeInterface)
{
    SniTypes::registerMetaTypes();

    const qsizetype slash = registration.indexOf(u'/');
    if (slash < 0) {
        m_service = registration;
        m_path = u"/StatusNotifierItem"_s;
    } else {
        m_service = registration.left(slash);
        m_path = registration.mid(slash);
    }
    m_valid = !m_service.isEmpty();
    if (!m_valid) {
        return;
    }

    m_refreshTimer.setSingleShot(true);
    m_refreshTimer.setInterval(10);
    connect(&m_refreshTimer, &QTimer::timeout, this, &StatusNotifierItem::performRefresh);

    QDBusConnection bus = QDBusConnection::sessionBus();
    for (const QString &iface : {s_kdeInterface, s_fdoInterface}) {
        for (const char *signal : {"NewTitle", "NewIcon", "NewAttentionIcon", "NewOverlayIcon", "NewToolTip"}) {
            bus.connect(m_service, m_path, iface, QString::fromLatin1(signal), this, SLOT(scheduleRefresh()));
        }
        bus.connect(m_service, m_path, iface, u"NewStatus"_s, this, SLOT(onNewStatus(QString)));
        bus.connect(m_service, m_path, iface, u"NewMenu"_s, this, SLOT(onNewMenu()));
    }

    performRefresh();
}

StatusNotifierItem::~StatusNotifierItem() = default;

KIconLoader *StatusNotifierItem::iconLoader() const
{
    return m_customIconLoader ? m_customIconLoader : KIconLoader::global();
}

void StatusNotifierItem::scheduleRefresh()
{
    if (!m_refreshTimer.isActive()) {
        m_refreshTimer.start();
    }
}

void StatusNotifierItem::onNewStatus(const QString &status)
{
    if (m_status == status) {
        return;
    }
    m_status = status;
    Q_EMIT changed();
}

void StatusNotifierItem::onNewMenu()
{
    if (m_menuClient) {
        m_menuClient->close();
        m_menuClient->deleteLater();
        m_menuClient = nullptr;
    }
    scheduleRefresh();
}

void StatusNotifierItem::performRefresh()
{
    if (m_refreshing) {
        m_refreshAgain = true;
        return;
    }
    m_refreshing = true;

    QDBusMessage call = QDBusMessage::createMethodCall(m_service, m_path, u"org.freedesktop.DBus.Properties"_s, u"GetAll"_s);
    call << m_interface;
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(call, 5000), this);
    connect(watcher, &QDBusPendingCallWatcher::finished, this, &StatusNotifierItem::refreshFinished);
}

void StatusNotifierItem::refreshFinished(QDBusPendingCallWatcher *watcher)
{
    watcher->deleteLater();
    m_refreshing = false;
    if (m_refreshAgain) {
        m_refreshAgain = false;
        performRefresh();
        return;
    }

    QDBusPendingReply<QVariantMap> reply = *watcher;
    QVariantMap properties;
    if (!reply.isError()) {
        properties = reply.value();
    }
    if (properties.isEmpty() && m_interface == s_kdeInterface) {
        // A few implementations only speak the freedesktop interface name.
        m_interface = s_fdoInterface;
        performRefresh();
        return;
    }
    if (properties.isEmpty()) {
        qCDebug(TRAYGRID_SNI) << "GetAll failed for" << m_registration << reply.error().message();
        if (!m_ready) {
            m_ready = true; // still show something (a generic icon)
            Q_EMIT ready();
        }
        return;
    }

    m_rawId = properties.value(u"Id"_s).toString();
    m_itemId = m_rawId;
    if (m_rawId.startsWith(u"dropbox-client-"_s)) {
        m_itemId = u"dropbox-client-PID"_s;
    } else if (m_rawId.startsWith(u"chrome_status_icon_"_s)) {
        const QString process = resolveProcessName();
        if (!process.isEmpty()) {
            m_itemId = m_rawId + u'@' + process;
        }
    }
    m_title = properties.value(u"Title"_s).toString();
    m_category = properties.value(u"Category"_s).toString();
    m_status = properties.value(u"Status"_s).toString();
    m_itemIsMenu = properties.value(u"ItemIsMenu"_s).toBool();

    QString menuPath = properties.value(u"Menu"_s).value<QDBusObjectPath>().path();
    if (menuPath == u"/NO_DBUSMENU"_s || menuPath == u"/"_s) {
        menuPath.clear();
    }
    if (menuPath != m_menuPath && m_menuClient) {
        m_menuClient->deleteLater();
        m_menuClient = nullptr;
    }
    m_menuPath = menuPath;

    SniToolTip toolTip;
    const QVariant toolTipVariant = properties.value(u"ToolTip"_s);
    if (toolTipVariant.canConvert<QDBusArgument>()) {
        toolTipVariant.value<QDBusArgument>() >> toolTip;
    }
    m_toolTipTitle = toolTip.title;
    m_toolTipSubTitle = toolTip.title.isEmpty() ? QString() : toolTip.subTitle;
    if (m_title.isEmpty()) {
        m_title = !m_toolTipTitle.isEmpty() ? m_toolTipTitle : m_rawId;
    }

    resolveIcons(properties);

    Q_EMIT changed();
    if (!m_ready) {
        m_ready = true;
        Q_EMIT ready();
    }
}

void StatusNotifierItem::resolveIcons(const QVariantMap &properties)
{
    const QString themePath = properties.value(u"IconThemePath"_s).toString();
    if (!themePath.isEmpty() && themePath != m_iconThemePath) {
        if (!m_customIconLoader) {
            m_customIconLoader = new KIconLoader(QString(), QStringList(), this);
        }
        // Same approach as Plasma's tray: "<prefix>/<app>/icons" gives the app
        // name; icons may be flat in the directory or in hicolor layout.
        QString appName;
        auto tokens = QStringView(themePath).split(u'/', Qt::SkipEmptyParts);
        if (tokens.size() >= 3 && tokens.takeLast() == u"icons") {
            appName = tokens.takeLast().toString();
        }
        m_customIconLoader->reconfigure(appName, QStringList(themePath));
        m_customIconLoader->addAppDir(appName.isEmpty() ? u"unused"_s : appName, themePath);
    }
    m_iconThemePath = themePath;

    m_overlayIconName = properties.value(u"OverlayIconName"_s).toString();

    auto load = [this, &properties](const QString &nameKey, const QString &pixmapKey, QString *nameOut, QIcon *iconOut, QString *origin) {
        *nameOut = QString();
        *iconOut = QIcon();
        QString name = properties.value(nameKey).toString();
        if (!name.isEmpty()) {
            if (QDir::isAbsolutePath(name)) {
                if (QFile::exists(name)) {
                    *iconOut = QIcon(name);
                    *origin = u"file:"_s + name;
                    return;
                }
            } else if (m_customIconLoader && m_customIconLoader->hasIcon(name)) {
                // Private icon theme shipped by the application.
                *iconOut = QIcon(new KIconEngine(name, m_customIconLoader));
                *origin = u"themepath:"_s + name;
                return;
            } else {
                // Prefer a symbolic variant, like Plasma's own tray does, but
                // only if it really exists: KIconLoader::hasIcon() also
                // answers true for "foo-symbolic" when only "foo" exists.
                const QString symbolic = name + u"-symbolic"_s;
                if (!name.endsWith(u"-symbolic"_s) && KIconLoader::global()->iconPath(symbolic, KIconLoader::Panel, true).contains(symbolic)) {
                    name = symbolic;
                }
                if (!KIconLoader::global()->iconPath(name, KIconLoader::Panel, true).isEmpty() || QIcon::hasThemeIcon(name)) {
                    *nameOut = name;
                    *origin = u"theme:"_s + name;
                    return;
                }
            }
        }
        SniImageVector pixmaps;
        const QVariant pixmapVariant = properties.value(pixmapKey);
        if (pixmapVariant.canConvert<QDBusArgument>()) {
            pixmapVariant.value<QDBusArgument>() >> pixmaps;
        }
        if (!pixmaps.isEmpty()) {
            *iconOut = SniImage::toIcon(pixmaps);
            QStringList sizes;
            for (const auto &p : std::as_const(pixmaps)) {
                sizes << QStringLiteral("%1x%2").arg(p.width).arg(p.height);
            }
            *origin = u"pixmap:"_s + sizes.join(u',');
            return;
        }
        if (!name.isEmpty()) {
            // Unknown to us; let Kirigami.Icon try (it may still find it).
            *nameOut = name;
            *origin = u"unresolved:"_s + name;
        }
    };

    QString origin;
    load(u"IconName"_s, u"IconPixmap"_s, &m_iconName, &m_icon, &origin);
    m_iconOrigin = origin;
    QString attentionOrigin;
    load(u"AttentionIconName"_s, u"AttentionIconPixmap"_s, &m_attentionIconName, &m_attentionIcon, &attentionOrigin);
}

QString StatusNotifierItem::configId() const
{
    if (m_path.startsWith(u"/org/ayatana/NotificationItem/"_s) && m_path.section(u'/', -1) == m_rawId && !m_title.isEmpty()) {
        return m_title;
    }
    return m_itemId;
}

QVariant StatusNotifierItem::iconSource() const
{
    if (m_status == u"NeedsAttention"_s) {
        if (!m_attentionIconName.isEmpty()) {
            return m_attentionIconName;
        }
        if (!m_attentionIcon.isNull()) {
            return QVariant::fromValue(m_attentionIcon);
        }
    }
    if (!m_iconName.isEmpty()) {
        return m_iconName;
    }
    if (!m_icon.isNull()) {
        return QVariant::fromValue(m_icon);
    }
    return u"application-x-executable"_s;
}

QString StatusNotifierItem::resolveProcessName() const
{
    const auto pidReply = QDBusConnection::sessionBus().interface()->servicePid(m_service);
    if (!pidReply.isValid()) {
        return {};
    }
    const QString procBase = u"/proc/"_s + QString::number(pidReply.value()) + u'/';

    // Electron apps all report "electron" as comm; use the .asar location.
    QFile cmdline(procBase + u"cmdline"_s);
    if (cmdline.open(QIODevice::ReadOnly)) {
        const QList<QByteArray> args = cmdline.readAll().split('\0');
        for (const QByteArray &arg : args) {
            if (arg.endsWith(".asar")) {
                const QFileInfo asar(QString::fromUtf8(arg));
                return QFileInfo(asar.path()).fileName();
            }
        }
    }
    QFile comm(procBase + u"comm"_s);
    if (comm.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return QString::fromUtf8(comm.readAll()).trimmed();
    }
    return {};
}

void StatusNotifierItem::provideActivationToken(const QString &token)
{
    if (token.isEmpty()) {
        return;
    }
    QDBusMessage call = QDBusMessage::createMethodCall(m_service, m_path, m_interface, u"ProvideXdgActivationToken"_s);
    call << token;
    QDBusConnection::sessionBus().call(call, QDBus::NoBlock);
}

void StatusNotifierItem::callMethod(const QString &method, const QPoint &pos)
{
    QDBusMessage call = QDBusMessage::createMethodCall(m_service, m_path, m_interface, method);
    call << pos.x() << pos.y();
    QDBusConnection::sessionBus().call(call, QDBus::NoBlock);
}

void StatusNotifierItem::activate(QQuickItem *item, const QPointF &localPos)
{
    const QPoint globalPos = PopupHelper::globalPosition(item, localPos);
    QPointer<QQuickItem> itemGuard(item);
    PopupHelper::withActivationToken(item ? item->window() : nullptr, this, [this, globalPos, itemGuard, localPos](const QString &token) {
        provideActivationToken(token);
        QDBusMessage call = QDBusMessage::createMethodCall(m_service, m_path, m_interface, u"Activate"_s);
        call << globalPos.x() << globalPos.y();
        auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(call, 5000), this);
        connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, itemGuard, localPos](QDBusPendingCallWatcher *w) {
            w->deleteLater();
            QDBusPendingReply<> reply = *w;
            if (reply.isError()) {
                // libappindicator items have no Activate; Plasma falls back
                // to the context menu in that case, and so do we.
                qCDebug(TRAYGRID_SNI) << "Activate failed for" << m_registration << reply.error().message() << "- opening menu";
                if (itemGuard) {
                    showContextMenu(itemGuard, localPos);
                }
                return;
            }
            Q_EMIT activated();
        });
    });
}

void StatusNotifierItem::secondaryActivate(QQuickItem *item, const QPointF &localPos)
{
    const QPoint globalPos = PopupHelper::globalPosition(item, localPos);
    PopupHelper::withActivationToken(item ? item->window() : nullptr, this, [this, globalPos](const QString &token) {
        provideActivationToken(token);
        callMethod(u"SecondaryActivate"_s, globalPos);
    });
}

void StatusNotifierItem::scroll(int delta, Qt::Orientation orientation)
{
    QDBusMessage call = QDBusMessage::createMethodCall(m_service, m_path, m_interface, u"Scroll"_s);
    call << delta << (orientation == Qt::Horizontal ? u"horizontal"_s : u"vertical"_s);
    QDBusConnection::sessionBus().call(call, QDBus::NoBlock);
}

DBusMenuClient *StatusNotifierItem::menuClient()
{
    if (m_menuPath.isEmpty()) {
        return nullptr;
    }
    if (!m_menuClient) {
        m_menuClient = new DBusMenuClient(m_service, m_menuPath, this);
        connect(m_menuClient, &DBusMenuClient::menuShown, this, &StatusNotifierItem::menuShown);
        connect(m_menuClient, &DBusMenuClient::menuHidden, this, &StatusNotifierItem::menuHidden);
        connect(m_menuClient, &DBusMenuClient::actionTriggered, this, &StatusNotifierItem::menuActionTriggered);
        connect(m_menuClient, &DBusMenuClient::activationTokenReady, this, &StatusNotifierItem::provideActivationToken);
    }
    return m_menuClient;
}

void StatusNotifierItem::showContextMenu(QQuickItem *item, const QPointF &localPos)
{
    if (DBusMenuClient *client = menuClient()) {
        client->popup(item, localPos);
        return;
    }
    // No dbusmenu: ask the application to show its own menu.
    const QPoint globalPos = PopupHelper::globalPosition(item, localPos);
    PopupHelper::withActivationToken(item ? item->window() : nullptr, this, [this, globalPos](const QString &token) {
        provideActivationToken(token);
        callMethod(u"ContextMenu"_s, globalPos);
    });
}
