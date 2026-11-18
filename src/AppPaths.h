/**
 * SPDX-FileComment: Program folder and restart target, AppImage-aware (issue #52)
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file AppPaths.h
 * @brief An AppImage runs from a read-only mount; its files belong next to the .AppImage file.
 */

#pragma once

#include <QCoreApplication>
#include <QFileInfo>
#include <QString>

namespace mitm::paths {

/// Folder for config.enc, configs/ and data/: the folder of the AppImage file if appImage is set, else appDir.
inline QString baseDir(const QString& appImage, const QString& appDir) {
    return appImage.isEmpty() ? appDir : QFileInfo(appImage).absolutePath();
}

/// Program to start for a restart: the AppImage file if appImage is set, else appFile.
inline QString restartProgram(const QString& appImage, const QString& appFile) {
    return appImage.isEmpty() ? appFile : appImage;
}

inline QString appImageEnv() {
    return qEnvironmentVariable("APPIMAGE");
}

/// Program folder of the running application.
inline QString baseDir() {
    return baseDir(appImageEnv(), QCoreApplication::applicationDirPath());
}

/// Program to start for a restart of the running application.
inline QString restartProgram() {
    return restartProgram(appImageEnv(), QCoreApplication::applicationFilePath());
}

} // namespace mitm::paths
