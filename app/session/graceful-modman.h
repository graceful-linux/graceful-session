#ifndef GRACEFULMODMAN_H
#define GRACEFULMODMAN_H

#include <QAbstractNativeEventFilter>
#include <QProcess>
#include <QList>
#include <QMap>
#include <QTimer>
#include <XdgDesktopFile>
#include <QEventLoop>
#include <time.h>
#include "proc-reaper.h"

class GracefulModule;
namespace graceful {
class Settings;
}
class QFileSystemWatcher;

typedef QMap<QString,GracefulModule*>           ModulesMap;
typedef QList<time_t>                           ModuleCrashReport;
typedef QMap<QProcess*, ModuleCrashReport>      ModulesCrashReport;
typedef QMapIterator<QString,GracefulModule*>   ModulesMapIterator;


void graceful_setenv(const char *env, const QByteArray &value);
void graceful_setenv_prepend(const char *env, const QByteArray &value, const QByteArray &separator=":");


class GracefulModuleManager : public QObject, public QAbstractNativeEventFilter
{
    Q_OBJECT
public:
    GracefulModuleManager(QObject* parent = nullptr);
    ~GracefulModuleManager() override;

    void setWindowManager(const QString & windowManager);

    void stopProcess(const QString& name);
    void startProcess(const QString& name);

    QStringList listModules() const;

    void startup(graceful::Settings& s);

    bool nativeEventFilter(const QByteArray & eventType, void * message, long * result) override;

public Q_SLOTS:
    void logout(bool doExit);
    void restartModules(int /*exitCode*/, QProcess::ExitStatus exitStatus);

Q_SIGNALS:
    void moduleStateChanged(QString moduleName, bool state);

private:
    void startWm();
    void startBar();
    void wmStarted();
    void startDaemon();
    void startDesktop();
    void startNetworkPlugin();

    void startAutostartApps();

    QString showWmSelectDialog();

    void startConfUpdate();
    void startProcess(const XdgDesktopFile &file);

private Q_SLOTS:
    void resetCrashReport();

    void themeFolderChanged(const QString&);

    void themeChanged();

private:
    bool                    mWmStarted;
    bool                    mTrayStarted;

    ModulesMap              mNameMap;
    ModulesCrashReport      mCrashReport;

    QFileSystemWatcher*     mThemeWatcher;
    QString                 mCurrentThemePath;

    QEventLoop*             mWaitLoop;
    ProcReaper              mProcReaper;

    QString                 mBar;
    QString                 mDaemon;
    QString                 mDesktop;
    QString                 mWindowManager;
    QString                 mNetworkPlugin;
};

class GracefulModule : public QProcess
{
    Q_OBJECT
public:
    void start();
    void terminate();
    bool isTerminating();

    GracefulModule(const XdgDesktopFile& file, QObject* parent = nullptr);

    const XdgDesktopFile    file;
    const QString           fileName;

Q_SIGNALS:
    void moduleStateChanged(QString name, bool state);

private Q_SLOTS:
    void updateState(QProcess::ProcessState newState);

private:
    bool                    mIsTerminating;
};


#endif // GRACEFULMODMAN_H
