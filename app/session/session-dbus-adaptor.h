#ifndef SESSIONDBUSADAPTOR_H
#define SESSIONDBUSADAPTOR_H
#include <QtDBus>
#include <graceful/power.h>

#include "graceful-modman.h"


class SessionDBusAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.graceful.session")

public:
    SessionDBusAdaptor(GracefulModuleManager * manager)
        : QDBusAbstractAdaptor(manager),
        m_manager(manager),
        m_power(false/*don't use ourself, just all other power providers*/)
    {
        connect(m_manager, &GracefulModuleManager::moduleStateChanged, this, &SessionDBusAdaptor::moduleStateChanged);
    }

Q_SIGNALS:
    void moduleStateChanged(QString moduleName, bool state);

public Q_SLOTS:
    bool canLogout()
    {
        return true;
    }

    bool canReboot()
    {
        return m_power.canReboot();
    }

    bool canPowerOff()
    {
        return m_power.canShutdown();
    }

    Q_NOREPLY void logout()
    {
        m_manager->logout(true);
    }

    Q_NOREPLY void reboot()
    {
        m_manager->logout(false);
        m_power.reboot();
        QCoreApplication::exit(0);
    }

    Q_NOREPLY void powerOff()
    {
        m_manager->logout(false);
        m_power.shutdown();
        QCoreApplication::exit(0);
    }

    QDBusVariant listModules()
    {
        return QDBusVariant(m_manager->listModules());
    }

    Q_NOREPLY void startModule(const QString& name)
    {
        m_manager->startProcess(name);
    }

    Q_NOREPLY void stopModule(const QString& name)
    {
        m_manager->stopProcess(name);
    }

private:
    GracefulModuleManager*          m_manager;
    graceful::Power                 m_power;
};

#endif // SESSIONDBUSADAPTOR_H
