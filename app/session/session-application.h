#ifndef SESSIONAPPLICATION_H
#define SESSIONAPPLICATION_H

#include "lock-screen-manager.h"

#include <graceful/settings.h>
#include <graceful/application.h>

class LockScreenManager;
class GracefulModuleManager;

class SessionApplication : public graceful::Application
{
    Q_OBJECT
public:
    SessionApplication(int& argc, char** argv);
    ~SessionApplication() override;
    void setWindowManager(const QString & windowManager);
    void setConfigName(const QString & configName);

private Q_SLOTS:
    bool startup();

private:
    void loadEnvironmentSettings(graceful::Settings& settings);
    void loadKeyboardSettings(graceful::Settings& settings);
    void loadMouseSettings(graceful::Settings& settings);

    void setxkbmap(QString layout, QString variant, QString model, QStringList options);

    void mergeXrdb(const char* content, int len);
    void setLeftHandedMouse(bool mouse_left_handed);

private:
    QString                 configName;
    LockScreenManager*      lockScreenManager;
    GracefulModuleManager*  modman;
};


#endif // SESSIONAPPLICATION_H
