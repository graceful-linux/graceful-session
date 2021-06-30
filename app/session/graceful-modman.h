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

    void startProcess(const QString& name);

    void stopProcess(const QString& name);

    QStringList listModules() const;

    void startup(graceful::Settings& s);

    bool nativeEventFilter(const QByteArray & eventType, void * message, long * result) override;

public Q_SLOTS:
    void logout(bool doExit);
    void restartModules(int /*exitCode*/, QProcess::ExitStatus exitStatus);

Q_SIGNALS:
    void moduleStateChanged(QString moduleName, bool state);

private:
    void startWm(graceful::Settings *settings);
    void wmStarted();

    void startAutostartApps();

    QString showWmSelectDialog();

    void startProcess(const XdgDesktopFile &file);

    void startConfUpdate();

    ModulesMap mNameMap;

    QProcess* mWmProcess;

    ModulesCrashReport mCrashReport;

    QFileSystemWatcher *mThemeWatcher;
    QString mCurrentThemePath;

    bool mWmStarted;
    bool mTrayStarted;
    QEventLoop* mWaitLoop;

    ProcReaper mProcReaper;

private Q_SLOTS:
    void resetCrashReport();

    void themeFolderChanged(const QString&);

    void themeChanged();
};

class GracefulModule : public QProcess
{
    Q_OBJECT
public:
    GracefulModule(const XdgDesktopFile& file, QObject *parent = nullptr);
    void start();
    void terminate();
    bool isTerminating();

    const XdgDesktopFile file;
    const QString fileName;

Q_SIGNALS:
    void moduleStateChanged(QString name, bool state);

private Q_SLOTS:
    void updateState(QProcess::ProcessState newState);

private:
    bool mIsTerminating;
};


#endif // GRACEFULMODMAN_H
