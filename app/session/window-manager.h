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
 * 根据预配的窗管查找窗管是否存在，然后供用户选择
 */

typedef QList<WindowManager> WindowManagerList;
WindowManagerList  getWindowManagerList(bool onlyAvailable=true);

bool findProgram(const QString &program);



#endif // WINDOWMANAGER_H
