#include <QApplication>
#include <spdlog/spdlog.h>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    // Setup spdlog
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("Starting MitM Admin Frontend...");

    // Setup Qt Application
    QApplication app(argc, argv);
    app.setApplicationName("MitM Admin");
    app.setApplicationVersion("0.1.0");

    // Optional: Set a dark style if supported by OS, or force fusion
    app.setStyle("Fusion");

    MainWindow window;
    window.show();

    int execCode = app.exec();
    spdlog::info("Application exiting with code {}", execCode);
    
    return execCode;
}
