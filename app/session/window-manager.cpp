#include "window-manager.h"

#include <QObject>
#include <QStringList>
#include <QFileInfo>
#include <QDir>
#include <graceful/globals.h>
#include <graceful/settings.h>
#include <QDebug>


bool findProgram(const QString &program)
{
    QFileInfo fi(program);
    if (fi.isExecutable()) {
        return true;
    }

    const QStringList paths = QFile::decodeName(qgetenv("PATH")).split(':');
    for(const QString &dir : paths) {
        QFileInfo fi= QFileInfo(dir + QDir::separator() + program);
        if (fi.isExecutable()) {
            return true;
        }
    }

    return false;
}

WindowManagerList getWindowManagerList(bool onlyAvailable)
{
    graceful::Settings cfg("windowmanagers");
    cfg.beginGroup("KnownManagers");
    const QStringList names = cfg.childGroups();

    WindowManagerList ret;

    for (const QString &name : names) {
        bool exists = findProgram(name);
        if (!onlyAvailable || exists) {
            cfg.beginGroup(name);
            WindowManager wm;
            wm.command = name;
            wm.name = cfg.localizedValue("Name", wm.command).toString();
            wm.comment = cfg.localizedValue("Comment").toString();
            wm.exists = exists;
            ret << wm;
            cfg.endGroup();
        }
    }

    return ret;
}
