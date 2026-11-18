/**
 * SPDX-FileComment: Tests for the AppImage-aware program folder (issue #52)
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */

#include <QtTest>
#include "AppPaths.h"

using namespace mitm::paths;

class AppPathsTest : public QObject {
    Q_OBJECT
private slots:
    void baseDirWithoutAppImageIsProgramFolder() {
        QCOMPARE(baseDir(QString(), "/opt/mitm/bin"), QString("/opt/mitm/bin"));
    }
    void baseDirWithAppImageIsFolderOfAppImageFile() {
        QCOMPARE(baseDir("/home/alice/apps/mitm_fe_cpp-2.4.0-x86_64.AppImage", "/tmp/.mount_abc/usr/bin"),
                 QString("/home/alice/apps"));
    }
    void restartProgramWithoutAppImageIsExecutable() {
        QCOMPARE(restartProgram(QString(), "/opt/mitm/bin/mitm_fe_cpp"), QString("/opt/mitm/bin/mitm_fe_cpp"));
    }
    void restartProgramWithAppImageIsAppImageFile() {
        QCOMPARE(restartProgram("/home/alice/apps/m.AppImage", "/tmp/.mount_abc/usr/bin/mitm_fe_cpp"),
                 QString("/home/alice/apps/m.AppImage"));
    }
};

QTEST_GUILESS_MAIN(AppPathsTest)
#include "AppPathsTest.moc"
