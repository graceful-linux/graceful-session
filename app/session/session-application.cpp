#include "session-application.h"

#include "session-dbus-adaptor.h"
#include "graceful-modman.h"
#include "num-lock.h"
#include "lock-screen-manager.h"
#include <unistd.h>
#include <csignal>
#include <graceful/settings.h>
#include <graceful/globals.h>
#include <QProcess>
#include "log.h"

#include <QX11Info>
#include <X11/XKBlib.h>

#include <graceful/settings.h>

using namespace graceful;

SessionApplication::SessionApplication(int& argc, char** argv) :
    graceful::Application(argc, argv),
    lockScreenManager(new LockScreenManager(this))
{
    listenToUnixSignals({SIGINT, SIGTERM, SIGQUIT, SIGHUP});

    modman = new GracefulModuleManager;
    connect(this, &graceful::Application::unixSignal, modman, [this] { modman->logout(true); });
    new SessionDBusAdaptor(modman);
    // connect to D-Bus and register as an object:
    QDBusConnection::sessionBus().registerService(QSL("org.graceful.session"));
    QDBusConnection::sessionBus().registerObject(QSL("/GracefulSession"), modman);

    // Wait until the event loop starts
    QTimer::singleShot(0, this, SLOT(startup()));
}

SessionApplication::~SessionApplication()
{
    delete modman;
}

void SessionApplication::setWindowManager(const QString& windowManager)
{
    modman->setWindowManager(windowManager);
}

void SessionApplication::setConfigName(const QString& configName)
{
    if(configName.isEmpty())
        this->configName = QSL("session");

    // tell the world which config file we're using.
    qputenv("GRACEFUL_SESSION_CONFIG", this->configName.toLocal8Bit());
}

bool SessionApplication::startup()
{
    Settings settings(configName);
    qCDebug(SESSION) << __FILE__ << ":" << __LINE__ << "Session" << configName << "about to launch (default 'session')";

    loadEnvironmentSettings(settings);
    // loadFontSettings(settings);
    loadKeyboardSettings(settings);
    loadMouseSettings(settings);

    if (lockScreenManager->startup(settings.value(QLatin1String("lock_screen_before_power_actions"), true).toBool()
                                       , settings.value(QLatin1String("power_actions_after_lock_delay"), 0).toInt()))
        qCDebug(SESSION) << "LockScreenManager started successfully";
    else
        qCWarning(SESSION) << "LockScreenManager couldn't start";

    // launch module manager and autostart apps
    modman->startup(settings);

    return true;
}

void SessionApplication::mergeXrdb(const char* content, int len)
{
    qCDebug(SESSION) << "xrdb:" << content;
    QProcess xrdb;
    xrdb.setProgram(QSL("xrdb"));
    xrdb.setArguments(QStringList(QSL("-merge -")));
    xrdb.start();
    xrdb.write(content, len);
    xrdb.closeWriteChannel();
    xrdb.waitForFinished();
}

void SessionApplication::loadEnvironmentSettings(Settings& settings)
{
    // first - set some user defined environment variables (like TERM...)
    settings.beginGroup(QSL("Environment"));
    QByteArray envVal;
    const QStringList keys = settings.childKeys();
    for(const QString& i : keys)
    {
        envVal = settings.value(i).toByteArray();
        graceful_setenv(i.toLocal8Bit().constData(), envVal);
    }
    settings.endGroup();
}

// FIXME: how to set keyboard layout in Wayland?
void SessionApplication::setxkbmap(QString layout, QString variant, QString model, QStringList options) {
    QStringList args;
    if(!model.isEmpty()) {
        args << QStringLiteral("-model");
        args << model;
    }
    if(!layout.isEmpty()) {
        args << QStringLiteral("-layout");
        args << layout;

        if(!variant.isEmpty()) {
            args << QStringLiteral("-variant");
            args << variant;
        }
    }
    if(!options.isEmpty()) {
        for(const QString& option : qAsConst(options)) {
            args << QStringLiteral("-option");
            args << option;
        }
    }
    // execute the command line
    if (!args.isEmpty())
        QProcess::startDetached(QStringLiteral("setxkbmap"), args);
}

void SessionApplication::loadKeyboardSettings(Settings& settings)
{
    qCDebug(SESSION) << settings.fileName();
    settings.beginGroup(QSL("Keyboard"));
    XKeyboardControl values;
    /* Keyboard settings */
    unsigned int delay = 0;
    unsigned int interval = 0;
    if(XkbGetAutoRepeatRate(QX11Info::display(), XkbUseCoreKbd, (unsigned int*) &delay, (unsigned int*) &interval))
    {
        delay = settings.value(QSL("delay"), delay).toUInt();
        interval = settings.value(QSL("interval"), interval).toUInt();
        XkbSetAutoRepeatRate(QX11Info::display(), XkbUseCoreKbd, delay, interval);
    }

    // turn on/off keyboard beep
    bool beep = settings.value(QSL("beep")).toBool();
    values.bell_percent = beep ? -1 : 0;
    XChangeKeyboardControl(QX11Info::display(), KBBellPercent, &values);

    // turn on numlock as needed
    if(settings.value("numlock").toBool())
        enableNumlock();

    // keyboard layout support using setxkbmap
    QString layout = settings.value(QSL("layout")).toString();
    QString variant = settings.value(QSL("variant")).toString();
    QString model = settings.value(QSL("model")).toString();
    QStringList options = settings.value(QSL("options")).toStringList();
    setxkbmap(layout, variant, model, options);

    settings.endGroup();
}

void SessionApplication::loadMouseSettings(Settings& settings)
{
    settings.beginGroup(QSL("Mouse"));

    // mouse cursor (does this work?)
    QString cursorTheme = settings.value(QSL("cursor_theme")).toString();
    int cursorSize = settings.value(QSL("cursor_size")).toInt();
    QByteArray buf;
    if(!cursorTheme.isEmpty()) {
        buf += QBAL("Xcursor.theme:");
        buf += cursorTheme.toLocal8Bit();
        buf += QBAL("\n");
    }
    if(cursorSize > 0) {
        buf += QBAL("Xcursor.size:");
        buf += QByteArray::number(cursorSize);
        buf += QBAL("\n");
    }
    if(!buf.isEmpty()) {
        buf += QBAL("Xcursor.theme_core:true\n");
        mergeXrdb(buf.constData(), buf.length());
    }

    // other mouse settings
    int accel_factor = settings.value(QSL("accel_factor")).toInt();
    int accel_threshold = settings.value(QSL("accel_threshold")).toInt();
    if(accel_factor || accel_threshold)
        XChangePointerControl(QX11Info::display(), accel_factor != 0, accel_threshold != 0, accel_factor, 10, accel_threshold);

    // left handed mouse?
    bool left_handed = settings.value(QSL("left_handed"), false).toBool();
    setLeftHandedMouse(left_handed);

    settings.endGroup();
}

/* This function is taken from Gnome's control-center 2.6.0.3 (gnome-settings-mouse.c) and was modified*/
#define DEFAULT_PTR_MAP_SIZE 128
void SessionApplication::setLeftHandedMouse(bool mouse_left_handed)
{
    unsigned char *buttons = nullptr;
    unsigned char *more_buttons = nullptr;
    int n_buttons = 0;
    int i = 0;
    int idx_1 = 0, idx_3 = 1;

    buttons = (unsigned char*)malloc(DEFAULT_PTR_MAP_SIZE);
    if (!buttons)
    {
        return;
    }
    n_buttons = XGetPointerMapping(QX11Info::display(), buttons, DEFAULT_PTR_MAP_SIZE);
    if (n_buttons > DEFAULT_PTR_MAP_SIZE)
    {
        more_buttons = (unsigned char*)realloc(buttons, n_buttons);
        if (!more_buttons)
        {
            free(buttons);
            return;
        }
        buttons = more_buttons;
        n_buttons = XGetPointerMapping(QX11Info::display(), buttons, n_buttons);
    }

    for (i = 0; i < n_buttons; i++)
    {
        if (buttons[i] == 1)
            idx_1 = i;
        else if (buttons[i] == ((n_buttons < 3) ? 2 : 3))
            idx_3 = i;
    }

    if ((mouse_left_handed && idx_1 < idx_3) ||
        (!mouse_left_handed && idx_1 > idx_3))
    {
        buttons[idx_1] = ((n_buttons < 3) ? 2 : 3);
        buttons[idx_3] = 1;
        XSetPointerMapping(QX11Info::display(), buttons, n_buttons);
    }
    free(buttons);
}