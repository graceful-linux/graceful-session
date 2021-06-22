#include "log.h"

#include <QtGlobal>

#if defined(NDEBUG)
Q_LOGGING_CATEGORY(SESSION, "graceful-session", QtInfoMsg)
#else
Q_LOGGING_CATEGORY(SESSION, "graceful-session")
#endif
