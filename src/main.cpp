/**
 * SPDX-FileComment: main
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file main.cpp
 * @brief main
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include <QApplication>
#include <spdlog/spdlog.h>
#include <QCommandLineParser>
#include <QInputDialog>
#include <QMessageBox>
#include <QDir>
#include "MainWindow.h"
#include "Config.h"

int main(int argc, char *argv[]) {
    // Setup spdlog (default before config loads)
    spdlog::set_level(spdlog::level::info);
    spdlog::info("Starting MitM Admin Frontend...");

    // Setup Qt Application
    QApplication app(argc, argv);
    app.setApplicationName("MitM Admin");
    app.setApplicationVersion("0.1.0");
    app.setWindowIcon(QIcon("img/logo.ico"));

    // Parse command line for config path
    QCommandLineParser parser;
    parser.setApplicationDescription("MitM Admin Frontend");
    parser.addHelpOption();
    parser.addVersionOption();
    
    QCommandLineOption configOption(QStringList() << "c" << "config",
                                    "Path to encrypted config.json",
                                    "configPath");
    parser.addOption(configOption);
    parser.process(app);

    QString configPath = parser.value(configOption);
    if (configPath.isEmpty()) {
        configPath = QCoreApplication::applicationDirPath() + QDir::separator() + "config.enc";
    }

    // Prompt for password
    bool ok;
    QString password = QInputDialog::getText(nullptr, "MitM Admin",
                                             "Enter password to decrypt configuration:",
                                             QLineEdit::Password,
                                             "", &ok);
    if (!ok) {
        spdlog::warn("Configuration decryption cancelled by user. Exiting.");
        return 0; // User cancelled
    }

    // Load configuration
    try {
        mitm::config::ConfigManager::GetInstance().LoadEncryptedConfig(configPath.toStdString(), password.toStdString());
        spdlog::info("Successfully loaded and decrypted configuration.");
        
        // Set log level from config
        std::string logLevel = mitm::config::ConfigManager::GetInstance().GetConfig().log_level;
        if (logLevel == "debug") spdlog::set_level(spdlog::level::debug);
        else if (logLevel == "trace") spdlog::set_level(spdlog::level::trace);
        else if (logLevel == "warn") spdlog::set_level(spdlog::level::warn);
        else if (logLevel == "err" || logLevel == "error") spdlog::set_level(spdlog::level::err);
        else spdlog::set_level(spdlog::level::info);

    } catch (const std::exception& e) {
        spdlog::error("Failed to load config: {}", e.what());
        QMessageBox::critical(nullptr, "Configuration Error",
                              QString("Failed to decrypt or parse configuration:\n%1").arg(e.what()));
        return 1;
    }

    // Optional: Set a dark style if supported by OS, or force fusion
    app.setStyle("Fusion");

    MainWindow window;
    window.show();

    int execCode = app.exec();
    spdlog::info("Application exiting with code {}", execCode);
    
    return execCode;
}

