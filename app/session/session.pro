TEMPLATE = app
TARGET = graceful-session

QT += core network gui gui-private xml widgets dbus x11extras KWindowSystem

CONFIG += c++11 link_pkgconfig no_keywords
LIBS += -lgio-2.0 -lglib-2.0 -lX11 -lgraceful -lprocps -lXss

PKGCONFIG += udev Qt5Xdg

SOURCES += \
    $$PWD/log.cpp                                       \
    $$PWD/main.cpp                                      \
    $$PWD/num-lock.cpp                                  \
    $$PWD/proc-reaper.cpp                               \
    $$PWD/window-manager.cpp                            \
    $$PWD/graceful-modman.cpp                           \
    $$PWD/wm-select-dialog.cpp                          \
    $$PWD/lock-screen-manager.cpp                       \
    $$PWD/session-application.cpp                       \
    $$PWD/session-dbus-adaptor.cpp                      \


HEADERS += \
    $$PWD/log.h                                         \
    $$PWD/num-lock.h                                    \
    $$PWD/proc-reaper.h                                 \
    $$PWD/window-manager.h                              \
    $$PWD/graceful-modman.h                             \
    $$PWD/wm-select-dialog.h                            \
    $$PWD/lock-screen-manager.h                         \
    $$PWD/session-application.h                         \
    $$PWD/session-dbus-adaptor.h                        \


FORMS += \
    $$PWD/wm-select-dialog.ui


OTHER_FILES += \
    $$PWD/data/graceful-session.desktop                 \
    $$PWD/data/start-graceful-session


GRACEFUL_SESSION_DESKTOP.files = \
    $$PWD/data/graceful-session.desktop


GRACEFUL_SESSION_DESKTOP.path = /usr/share/xsessions/

target.files += $$PWD/data/start-graceful-session
target.path = /bin/


INSTALLS += target GRACEFUL_SESSION_DESKTOP
