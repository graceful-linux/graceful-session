#include <QStandardPaths>
#include <QCommandLineParser>

#include <graceful/log.h>
#include <graceful/globals.h>
#include "session-application.h"

int main (int argc, char* argv[])
{
    SessionApplication app(argc, argv);

    QString logPath = QStandardPaths::writableLocation(QStandardPaths::HomeLocation) + "/.local/log/graceful-session.log";
    FILE* lf = fopen(logPath.toUtf8().constData(), "w+");
    log_set_level(LOG_DEBUG);
    if (lf)     log_add_fp(lf, LOG_DEBUG);
    log_info("start graceful-session");

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Graceful Session"));
    const QString VERINFO = QStringLiteral(QT_VERSION_STR);
    app.setApplicationVersion(VERINFO);
    const QCommandLineOption config_opt{{("c"), ("config")}, SessionApplication::tr("Configuration file path."), SessionApplication::tr("file")};
    const QCommandLineOption wm_opt{{("w"), ("window-manager")}, SessionApplication::tr("Window manager to use."), SessionApplication::tr("file")};
    const auto version_opt = parser.addVersionOption();
    const auto help_opt = parser.addHelpOption();
    parser.addOptions({config_opt, wm_opt});
    parser.process(app);

    app.setConfigName(parser.value(config_opt));
    app.setWindowManager(parser.value(wm_opt));

    app.setQuitOnLastWindowClosed(false);

    return app.exec();
}
