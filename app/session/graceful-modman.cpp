#include "graceful-modman.h"

#include <graceful/globals.h>
#include <graceful/settings.h>
#include <XdgAutoStart>
#include <XdgDirs>
#include <unistd.h>

#include <QCoreApplication>
#include <QMessageBox>
#include <QSystemTrayIcon>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QFileSystemWatcher>
#include <QDateTime>
#include "wm-select-dialog.h"
#include "window-manager.h"
#include <wordexp.h>
#include <graceful/log.h>

#include <KF5/KWindowSystem/netwm.h>
#include <KF5/KWindowSystem/KWindowSystem>

#include <QX11Info>

#define MAX_CRASHES_PER_APP 5

using namespace graceful;

GracefulModuleManager::GracefulModuleManager(QObject* parent) : QObject(parent),
    mWmProcess(new QProcess(this)),
    mThemeWatcher(new QFileSystemWatcher(this)),
    mWmStarted(false),
    mTrayStarted(false),
    mWaitLoop(nullptr)
{
    connect(mThemeWatcher, &QFileSystemWatcher::directoryChanged, this, &GracefulModuleManager::themeFolderChanged);
    connect(Settings::globalSettings(), &GlobalSettings::gracefulThemeChanged, this, &GracefulModuleManager::themeChanged);

    qApp->installNativeEventFilter(this);
    mProcReaper.start();
}

void GracefulModuleManager::setWindowManager(const QString & windowManager)
{
    mWindowManager = windowManager;
}

void GracefulModuleManager::startup(Settings& s)
{
    startConfUpdate();

    // Start window manager
    startWm(&s);

    startAutostartApps();

    QStringList paths;
    paths << XdgDirs::dataHome(false);
    paths << XdgDirs::dataDirs();

    for(const QString &path : qAsConst(paths)) {
        QFileInfo fi(QString::fromLatin1("%1/graceful/themes").arg(path));
        if (fi.exists())
            mThemeWatcher->addPath(fi.absoluteFilePath());
    }

    themeChanged();
}

void GracefulModuleManager::startAutostartApps()
{
    log_debug("XDG autostart ...");
    const XdgDesktopFileList fileList = XdgAutoStart::desktopFileList();
    QList<const XdgDesktopFile*> trayApps;
    for (XdgDesktopFileList::const_iterator i = fileList.constBegin(); i != fileList.constEnd(); ++i) {
        if (i->value(QSL("X-Graceful-Need-Tray"), false).toBool()) {
            trayApps.append(&(*i));
        } else {
            startProcess(*i);
            log_debug("start %s", i->fileName().toUtf8().constData());
        }
    }

    if (!trayApps.isEmpty()) {
        mTrayStarted = QSystemTrayIcon::isSystemTrayAvailable();
        if(!mTrayStarted) {
            QEventLoop waitLoop;
            mWaitLoop = &waitLoop;
            // add a timeout to avoid infinite blocking if a WM fail to execute.
            QTimer::singleShot(60 * 1000, &waitLoop, SLOT(quit()));
            waitLoop.exec();
            mWaitLoop = nullptr;
        }
        for (const XdgDesktopFile* const f : qAsConst(trayApps)) {
            log_debug("start tray app %s", f->fileName().toUtf8().constData());
            startProcess(*f);
        }
    }
}

void GracefulModuleManager::themeFolderChanged(const QString& /*path*/)
{
    QString newTheme;
    if (!QFileInfo::exists(mCurrentThemePath)) {
        const QList<GracefulTheme> &allThemes = gracefulTheme.allThemes();
        if (!allThemes.isEmpty()) {
            newTheme = allThemes[0].name();
        } else {
            return;
        }
    } else {
        newTheme = gracefulTheme.currentTheme().name();
    }

    Settings settings("graceful");
    if (newTheme == settings.value("theme")) {
        // force the same theme to be updated
        settings.setValue(QSL("__theme_updated__"), QDateTime::currentMSecsSinceEpoch());
    }
    else
        settings.setValue(QL1S("theme"), newTheme);

    sync();
}

void GracefulModuleManager::themeChanged()
{
    if (!mCurrentThemePath.isEmpty())
        mThemeWatcher->removePath(mCurrentThemePath);

    if (gracefulTheme.currentTheme().isValid()) {
        mCurrentThemePath = gracefulTheme.currentTheme().path();
        mThemeWatcher->addPath(mCurrentThemePath);
    }
}

void GracefulModuleManager::startWm(Settings *settings)
{
    // if the WM is active do not run WM.
    // all window managers must set their name according to the spec
    if (!QString::fromUtf8(NETRootInfo(QX11Info::connection(), NET::SupportingWMCheck).wmName()).isEmpty()) {
        mWmStarted = true;
        return;
    }

    if (mWindowManager.isEmpty()) {
        mWindowManager = settings->value(QL1S("window_manager")).toString();
    }

    // If previuos WM was removed, we show dialog.
    if (mWindowManager.isEmpty() || ! findProgram(mWindowManager.split(QL1C(' '))[0])) {
        mWindowManager = showWmSelectDialog();
        settings->setValue(QL1S("window_manager"), mWindowManager);
        settings->sync();
    }

    log_debug("window manager '%s' start...", mWindowManager.toUtf8().constData());
    mWmProcess->start(mWindowManager, QStringList());

    // other autostart apps will be handled after the WM becomes available

    // Wait until the WM loads
    QEventLoop waitLoop;
    mWaitLoop = &waitLoop;
    // add a timeout to avoid infinite blocking if a WM fail to execute.
    QTimer::singleShot(30 * 1000, &waitLoop, SLOT(quit()));
    waitLoop.exec();
    mWaitLoop = nullptr;
    // FIXME: blocking is a bad idea. We need to start as many apps as possible and
    //         only wait for the start of WM when it's absolutely needed.
    //         Maybe we can add a X-Wait-WM=true key in the desktop entry file?
}

void GracefulModuleManager::startProcess(const XdgDesktopFile& file)
{
    if (!file.value(QL1S("X-Graceful-Module"), false).toBool()) {
        file.startDetached();
        return;
    }
    QStringList args = file.expandExecString();
    if (args.isEmpty()) {
        log_debug("Wrong desktop file %s", file.fileName().toUtf8().constData());
        return;
    }
    GracefulModule* proc = new GracefulModule(file, this);
    connect(proc, &GracefulModule::moduleStateChanged, this, &GracefulModuleManager::moduleStateChanged);
    proc->start();

    QString name = QFileInfo(file.fileName()).fileName();
    mNameMap[name] = proc;

    connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &GracefulModuleManager::restartModules);
}

void GracefulModuleManager::startProcess(const QString& name)
{
    if (!mNameMap.contains(name)) {
        const auto files = XdgAutoStart::desktopFileList(false);
        for (const XdgDesktopFile& file : files) {
            if (QFileInfo(file.fileName()).fileName() == name) {
                startProcess(file);
                return;
            }
        }
    }
}

void GracefulModuleManager::stopProcess(const QString& name)
{
    if (mNameMap.contains(name))
        mNameMap[name]->terminate();
}

QStringList GracefulModuleManager::listModules() const
{
    return QStringList(mNameMap.keys());
}

void GracefulModuleManager::startConfUpdate()
{
    XdgDesktopFile desktop(XdgDesktopFile::ApplicationType, QSL(":graceful-confupdate"), QSL("graceful-confupdate --watch"));
    desktop.setValue(QSL("Name"), QSL("Graceful config updater"));
    desktop.setValue(QL1S("X-Graceful-Module"), true);
    startProcess(desktop);
}

void GracefulModuleManager::restartModules(int /*exitCode*/, QProcess::ExitStatus exitStatus)
{
    GracefulModule* proc = qobject_cast<GracefulModule*>(sender());
    if (nullptr == proc) {
        log_warn("Got an invalid (null) module to restart. Ignoring it");
        return;
    }

    if (!proc->isTerminating()) {
        QString procName = proc->file.name();
        switch (exitStatus) {
        case QProcess::NormalExit:
            log_debug("Process %s exited correctly.", procName.toUtf8().constData());
            break;
        case QProcess::CrashExit: {
            log_debug("Process %s has to be restarted", procName.toUtf8().constData());
            time_t now = time(nullptr);
            mCrashReport[proc].prepend(now);
            while (now - mCrashReport[proc].back() > 60)
                mCrashReport[proc].pop_back();
            if (mCrashReport[proc].length() >= MAX_CRASHES_PER_APP) {
                QMessageBox::warning(nullptr, tr("Crash Report"),
                                     tr("<b>%1</b> crashed too many times. Its autorestart has been disabled until next login.").arg(procName));
            } else {
                proc->start();
                return;
            }
            break;
        }
        }
    }
    mNameMap.remove(proc->fileName);
    proc->deleteLater();
}


GracefulModuleManager::~GracefulModuleManager()
{
    qApp->removeNativeEventFilter(this);

    // We disconnect the finished signal before deleting the process. We do
    // this to prevent a crash that results from a state change signal being
    // emmited while deleting a crashing module.
    // If the module is still connect restartModules will be called with a
    // invalid sender.

    ModulesMapIterator i(mNameMap);
    while (i.hasNext()) {
        i.next();

        auto p = i.value();
        disconnect(p);

        delete p;
        mNameMap[i.key()] = nullptr;
    }

    delete mWmProcess;
}

/**
* @brief this logs us out by terminating our session
**/
void GracefulModuleManager::logout(bool doExit)
{
    // modules
    ModulesMapIterator i(mNameMap);
    while (i.hasNext()) {
        i.next();
        log_debug("Module logout %s", i.key().toUtf8().constData());
        GracefulModule* p = i.value();
        p->terminate();
    }
    i.toFront();
    while (i.hasNext()) {
        i.next();
        GracefulModule* p = i.value();
        if (p->state() != QProcess::NotRunning && !p->waitForFinished(2000)) {
            log_debug("Module %s won't terminate ... killing.", qPrintable(i.key()));
            p->kill();
        }
    }

    // terminate all possible children except WM
    mProcReaper.stop({mWmProcess->processId()});

    mWmProcess->terminate();
    if (mWmProcess->state() != QProcess::NotRunning && !mWmProcess->waitForFinished(2000)) {
        log_debug("Window Manager won't terminate ... killing.");
        mWmProcess->kill();
    }

    if (doExit) {
        QCoreApplication::exit(0);
    }
}

QString GracefulModuleManager::showWmSelectDialog()
{
    WindowManagerList availableWM = getWindowManagerList(true);
    if (availableWM.count() == 1)
        return availableWM.at(0).command;

    WMSelectDialog dlg(availableWM);
    dlg.exec();
    return dlg.windowManager();
}

void GracefulModuleManager::resetCrashReport()
{
    mCrashReport.clear();
}

bool GracefulModuleManager::nativeEventFilter(const QByteArray & eventType, void * /*message*/, long * /*result*/)
{
    if (eventType != "xcb_generic_event_t") // We only want to handle XCB events
        return false;

    if(!mWmStarted && mWaitLoop) {
        // all window managers must set their name according to the spec
        if (!QString::fromUtf8(NETRootInfo(QX11Info::connection(), NET::SupportingWMCheck).wmName()).isEmpty()) {
            log_debug("Window Manager started");
            mWmStarted = true;
            if (mWaitLoop->isRunning())
                mWaitLoop->exit();
        }
    }

    if (!mTrayStarted && QSystemTrayIcon::isSystemTrayAvailable() && mWaitLoop) {
        log_debug("System Tray started");
        mTrayStarted = true;
        if (mWaitLoop->isRunning())
            mWaitLoop->exit();

        // window manager and system tray have started
        qApp->removeNativeEventFilter(this);
    }

    return false;
}

void graceful_setenv(const char *env, const QByteArray &value)
{
    wordexp_t p;
    wordexp(value.constData(), &p, 0);
    if (p.we_wordc == 1) {
        log_debug("Environment variable %s=%s", env, p.we_wordv[0]);
        qputenv(env, p.we_wordv[0]);
    } else {
        log_debug("Error expanding environment variable %s=%s", env, value.toStdString().c_str());
        qputenv(env, value);
    }
    wordfree(&p);
}

void graceful_setenv_prepend(const char *env, const QByteArray &value, const QByteArray &separator)
{
    QByteArray orig(qgetenv(env));
    orig = orig.prepend(separator);
    orig = orig.prepend(value);
    log_debug("Setting special %s=%s", env, orig.toStdString().c_str());
    graceful_setenv(env, orig);
}

GracefulModule::GracefulModule(const XdgDesktopFile& file, QObject* parent) :
    QProcess(parent),
    file(file),
    fileName(QFileInfo(file.fileName()).fileName()),
    mIsTerminating(false)
{
    QProcess::setProcessChannelMode(QProcess::ForwardedChannels);
    connect(this, &GracefulModule::stateChanged, this, &GracefulModule::updateState);
}

void GracefulModule::start()
{
    mIsTerminating = false;
    QStringList args = file.expandExecString();
    QString command = args.takeFirst();
    QProcess::start(command, args);
}

void GracefulModule::terminate()
{
    mIsTerminating = true;
    QProcess::terminate();
}

bool GracefulModule::isTerminating()
{
    return mIsTerminating;
}

void GracefulModule::updateState(QProcess::ProcessState newState)
{
    if (newState != QProcess::Starting)
        Q_EMIT moduleStateChanged(fileName, (newState == QProcess::Running));
}
