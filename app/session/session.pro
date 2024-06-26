TEMPLATE    = app
TARGET      = graceful-session

QT          += core network gui gui-private xml widgets dbus x11extras KWindowSystem

CONFIG      += c++11 link_pkgconfig no_keywords
PKGCONFIG   += graceful gio-2.0 glib-2.0
LIBS        += -lX11 -lprocps -lXss

PKGCONFIG   += udev Qt5Xdg
include($$PWD/../common/common.pri)

SOURCES     += \
    $$PWD/main.cpp                                      \
    $$PWD/num-lock.cpp                                  \
    $$PWD/proc-reaper.cpp                               \
    $$PWD/window-manager.cpp                            \
    $$PWD/graceful-modman.cpp                           \
    $$PWD/wm-select-dialog.cpp                          \
    $$PWD/lock-screen-manager.cpp                       \
    $$PWD/session-application.cpp                       \
    $$PWD/session-dbus-adaptor.cpp                      \


HEADERS     += \
    $$PWD/num-lock.h                                    \
    $$PWD/proc-reaper.h                                 \
    $$PWD/window-manager.h                              \
    $$PWD/graceful-modman.h                             \
    $$PWD/wm-select-dialog.h                            \
    $$PWD/lock-screen-manager.h                         \
    $$PWD/session-application.h                         \
    $$PWD/session-dbus-adaptor.h                        \


FORMS       += \
    $$PWD/wm-select-dialog.ui


OTHER_FILES += \
    $$PWD/data/graceful-session.desktop                 \
    $$PWD/data/start-graceful-session                   \
    $$PWD/data/windowmanagers.conf


GRACEFUL_SESSION_DESKTOP.files = \
    $$PWD/data/graceful-session.desktop


GRACEFUL_SESSION_DESKTOP.path = /usr/share/xsessions/


GRACEFUL_WM.files = $$PWD/data/windowmanagers.conf
GRACEFUL_WM.path = /usr/share/graceful/

GRACEFUL_SESSION_TARGET.files += \
    $$PWD/data/start-graceful-session                   \
    $$OUT_PWD/graceful-session

GRACEFUL_SESSION_TARGET.path = /usr/bin/


INSTALLS += GRACEFUL_SESSION_TARGET GRACEFUL_WM GRACEFUL_SESSION_DESKTOP
