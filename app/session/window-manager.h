#ifndef WINDOWMANAGER_H
#define WINDOWMANAGER_H

#include <QMap>
#include <QString>

struct WindowManager
{
    QString name;
    QString command;
    QString comment;
    bool    exists;
};

/**
 * @brief
 */

typedef QList<WindowManager> WindowManagerList;
WindowManagerList  getWindowManagerList(bool onlyAvailable=true);

bool findProgram(const QString &program);



#endif // WINDOWMANAGER_H
