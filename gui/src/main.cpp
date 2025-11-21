#include "gui/Compositor.h"

#include <QGuiApplication>
#include <QCommandLineParser>
#include <QLoggingCategory>

int main(int argc, char *argv[])
{
    // Set up application
    QGuiApplication app(argc, argv);
    app.setApplicationName("SolixOS Compositor");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SolixOS");
    
    // Enable OpenGL debugging
    QSurfaceFormat format;
    format.setMajorVersion(3);
    format.setMinorVersion(3);
    format.setProfile(QSurfaceFormat::CoreProfile);
    format.setOption(QSurfaceFormat::DebugContext);
    QSurfaceFormat::setDefaultFormat(format);
    
    // Parse command line arguments
    QCommandLineParser parser;
    parser.setApplicationDescription("SolixOS Compositor");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption fullscreenOption("f", "Start in fullscreen mode");
    parser.addOption(fullscreenOption);
    
    parser.process(app);
    
    // Create and initialize compositor
    SolixOS::GUI::Compositor compositor;
    compositor.initialize();
    
    // Set fullscreen if requested
    if (parser.isSet(fullscreenOption)) {
        compositor.showFullScreen();
    } else {
        compositor.show();
    }
    
    return app.exec();
}
