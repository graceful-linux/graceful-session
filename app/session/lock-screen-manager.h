#ifndef LOCKSCREENMANAGER_H
#define LOCKSCREENMANAGER_H

#include <QObject>
#include <QDBusInterface>
#include <QScopedPointer>
#include <graceful/screensaver.h>

class QDBusUnixFileDescriptor;

class LockScreenProvider : public QObject
{
    Q_OBJECT
public:
    ~LockScreenProvider() override {}

    virtual bool isValid() = 0;
    virtual bool inhibit() = 0;
    virtual void release() = 0;

Q_SIGNALS:
    void aboutToSleep(bool beforeSleep);
    void lockRequested();
};

class LogindProvider : public LockScreenProvider
{
    Q_OBJECT
public:
    explicit LogindProvider();
    ~LogindProvider() override;

    bool isValid() override;
    bool inhibit() override;
    void release() override;

private:
    QDBusInterface                              mInterface;
    QScopedPointer<QDBusUnixFileDescriptor>     mFileDescriptor;
};

class ConsoleKit2Provider : public LockScreenProvider
{
    Q_OBJECT

public:
    explicit ConsoleKit2Provider();
    ~ConsoleKit2Provider() override;

    bool isValid() override;
    bool inhibit() override;
    void release() override;

private:
    QDBusInterface mInterface;
    bool mMethodInhibitPresent;
    QScopedPointer<QDBusUnixFileDescriptor> mFileDescriptor;
};

class LockScreenManager : public QObject
{
    Q_OBJECT

public:
    explicit LockScreenManager(QObject *parent = nullptr);
    ~LockScreenManager() override;

    bool startup(bool lockBeforeSleep, int powerAfterLockDelay/*!< ms*/);

private:
    void inhibit();

private:
    LockScreenProvider*         mProvider;

    // screensaver
    graceful::ScreenSaver       mScreenSaver;
    bool                        mLockedBeforeSleep;
};
#endif // LOCKSCREENMANAGER_H
