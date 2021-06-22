#include "lock-screen-manager.h"

#include "log.h"

#include <graceful/globals.h>

#include <QTimer>
#include <QDBusReply>
#include <QDebug>
#include <QDBusUnixFileDescriptor>
#include <unistd.h>

LockScreenManager::LockScreenManager(QObject *parent) :
    QObject(parent)
    , mProvider{nullptr}
    , mLockedBeforeSleep{false}
{
}

LockScreenManager::~LockScreenManager()
{
    delete mProvider;
}

bool LockScreenManager::startup(bool lockBeforeSleep, int powerAfterLockDelay)
{
    mProvider = new LogindProvider;
    if (!mProvider->isValid()) {
        mProvider->deleteLater();
        mProvider = new ConsoleKit2Provider;
        if (!mProvider->isValid()) {
            mProvider->deleteLater();
            mProvider = nullptr;
        }
    }

    if (!mProvider) {
        qCDebug(SESSION) << "LockScreenManager: no valid provider";
        return false;
    }

    qCDebug(SESSION) << "LockScreenManager:"
                     << mProvider->metaObject()->className()
                     << "will be used";

    connect(&mScreenSaver, &graceful::ScreenSaver::done, this, [this, powerAfterLockDelay] {
        if (mLockedBeforeSleep) {
            mLockedBeforeSleep = false;
            QTimer::singleShot(powerAfterLockDelay, this, [this] {
                mProvider->release();
                qCDebug(SESSION) << "LockScreenManager: after release";
            });
        }
    });

    connect(mProvider, &LockScreenProvider::lockRequested, this, [this] {
        qCDebug(SESSION) << "LockScreenManager: lock requested";
        mScreenSaver.lockScreen();
    });

    if (lockBeforeSleep) {
        connect(mProvider, &LockScreenProvider::aboutToSleep, this, [this] (bool beforeSleep) {
            if (beforeSleep) {
                qCDebug(SESSION) << "LockScreenManager: system is about to sleep";
                mLockedBeforeSleep = true;
                mScreenSaver.lockScreen();
                qCDebug(SESSION) << "LockScreenManager: after lockScreen";
            } else {
                inhibit();
            }
        });

        inhibit();
    }

    return true;
}

void LockScreenManager::inhibit()
{
    if (!mProvider->inhibit())
        qCWarning(SESSION) << "LockScreenManager: could not inhibit session provider";
}


LogindProvider::LogindProvider() :
    mInterface(QStringLiteral("org.freedesktop.login1"),
               QStringLiteral("/org/freedesktop/login1"),
               QStringLiteral("org.freedesktop.login1.Manager"),
               QDBusConnection::systemBus())
{
    if (mInterface.isValid()) {
        // SIGNAL/SLOT macros are needed here
        connect(&mInterface, SIGNAL(PrepareForSleep(bool)), this, SIGNAL(aboutToSleep(bool)));

        QString sessionId = QDBusInterface(
                                QStringLiteral("org.freedesktop.login1"),
                                QStringLiteral("/org/freedesktop/login1/session/self"),
                                QStringLiteral("org.freedesktop.login1.Session"),
                                QDBusConnection::systemBus())
                                .property("Id").toString();

        // listen to Lock signal as well
        mInterface.connection().connect(
            QStringLiteral("org.freedesktop.login1"),
            QStringLiteral("/org/freedesktop/login1/session/") + sessionId,
            QStringLiteral("org.freedesktop.login1.Session"),
            QStringLiteral("Lock"),
            this,
            SIGNAL(lockRequested()));
    }
}

LogindProvider::~LogindProvider()
{
    release();
}

bool LogindProvider::isValid()
{
    return mInterface.isValid();
}

bool LogindProvider::inhibit()
{
    if (mFileDescriptor)
        return false;

    QDBusReply<QDBusUnixFileDescriptor> reply = mInterface.call(
        QStringLiteral("Inhibit"),
        QStringLiteral("sleep"),
        QStringLiteral("Graceful Session"),
        QStringLiteral("Start screen locker before sleep."),
        QStringLiteral("delay"));

    if (!reply.isValid()) {
        qCDebug(SESSION) << "LockScreenManager: " << reply.error();
        return false;
    }

    mFileDescriptor.reset(new QDBusUnixFileDescriptor{reply.value()});
    return true;
}

void LogindProvider::release()
{
    mFileDescriptor.reset(nullptr);
}

/*
 * ConsoleKit2 provider
 */

ConsoleKit2Provider::ConsoleKit2Provider() :
    mInterface(QStringLiteral("org.freedesktop.ConsoleKit"),
               QStringLiteral("/org/freedesktop/ConsoleKit/Manager"),
               QStringLiteral("org.freedesktop.ConsoleKit.Manager"),
               QDBusConnection::systemBus()),
    mMethodInhibitPresent(false)

{
    // validate interface, look for melhod Inhibit
    if (mInterface.isValid()) {
        // this is needed to differentiate ConsoleKit2 from ConsoleKit
        QDBusReply<QString> reply = QDBusInterface(
                                        QStringLiteral("org.freedesktop.ConsoleKit"),
                                        QStringLiteral("/org/freedesktop/ConsoleKit/Manager"),
                                        QStringLiteral("org.freedesktop.DBus.Introspectable"),
                                        QDBusConnection::systemBus())
                                        .call(QStringLiteral("Introspect"));
        mMethodInhibitPresent = reply.value().contains("Inhibit");

        if (mMethodInhibitPresent) {
            // SIGNAL/SLOT macros are needed here
            connect(&mInterface, SIGNAL(PrepareForSleep(bool)), this, SIGNAL(aboutToSleep(bool)));

            QDBusReply<QDBusObjectPath> sessionObjectPath = mInterface.call("GetCurrentSession");

            // listen to Lock signal as well
            mInterface.connection().connect(
                QStringLiteral("org.freedesktop.ConsoleKit"),
                sessionObjectPath.value().path(),
                QStringLiteral("org.freedesktop.ConsoleKit.Session"),
                QStringLiteral("Lock"),
                this,
                SIGNAL(lockRequested()));
        }
    }
}

ConsoleKit2Provider::~ConsoleKit2Provider()
{
    release();
}

bool ConsoleKit2Provider::isValid()
{
    return mInterface.isValid() && mMethodInhibitPresent;
}

bool ConsoleKit2Provider::inhibit()
{
    if (mFileDescriptor)
        return false;

    QDBusReply<QDBusUnixFileDescriptor> reply = mInterface.call(
        QStringLiteral("Inhibit"),
        QStringLiteral("sleep"),
        QStringLiteral("Graceful Power Management"),
        QStringLiteral("Start screen locker before sleep."),
        QStringLiteral("delay"));

    if (!reply.isValid()) {
        qCDebug(SESSION) << "LockScreenWatcher: " << reply.error();
        return false;
    }

    mFileDescriptor.reset(new QDBusUnixFileDescriptor{reply.value()});
    return true;
}

void ConsoleKit2Provider::release()
{
    mFileDescriptor.reset(nullptr);
}
