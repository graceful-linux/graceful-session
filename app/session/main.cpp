#include <QCommandLineParser>

#include <graceful/globals.h>
#include "session-application.h"

int main (int argc, char* argv[])
{
    SessionApplication app(argc, argv);

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
