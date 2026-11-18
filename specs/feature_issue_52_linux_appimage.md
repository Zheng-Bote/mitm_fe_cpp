# Feature Specification: Linux AppImage Build Script (Issue #52)

Issue: https://github.com/Zheng-Bote/mitm_fe_cpp/issues/52

## Feature Intent

Provide `build_linux_appimage.sh` (counterpart to `build_macos_dmg.sh`) that builds the Release configuration and packages a self-contained AppImage. An AppImage runs from a read-only mount, so the app must not write next to its executable and must not load images relative to the working directory.

## Requirements (EARS Syntax)

1. The script shall build the Release configuration via Conan and the `conan-release` preset, unless `--no-build` is given.
2. The script shall package `build/Release/bin/mitm_fe_cpp` with linuxdeploy and linuxdeploy-plugin-qt (downloaded to `build/appimage/tools` if missing) including the Qt libraries/plugins, a desktop entry and the icon `img/logo_256x256.png`, as `build/appimage/mitm_fe_cpp-<version>-<arch>.AppImage`.
3. The script shall locate Qt via `QMAKE`/`QT_PREFIX`, otherwise via `Qt6_DIR` from `build/Release/CMakeCache.txt`, otherwise via `qmake6`/`qmake` in `PATH`; it shall not need FUSE.
4. When the app runs as an AppImage (`APPIMAGE` set), the program folder for `config.enc`, `configs/` and `data/` shall be the directory of the `.AppImage` file; otherwise it shall stay `applicationDirPath()`.
5. When the app restarts after selecting a configuration, it shall start `$APPIMAGE` when running as an AppImage, otherwise its own executable.
6. The window icon and the About logo shall be loaded from embedded Qt resources.

## Scope

Admin Frontend (mitm_fe_cpp): `build_linux_appimage.sh`, `src/AppPaths.h`, `src/main.cpp`, `src/Config.cpp`, `src/MainWindow.cpp`, `src/BackupRestoreWidget.cpp`, `resources.qrc`, `CMakeLists.txt`, test `AppPathsTest`, README, CHANGELOG, version 2.4.0.

## SpecDD Architecture Alignment (Drift Control)

- [x] **Architecture:** The layered architecture is maintained (no direct bypass from Collector to Delivery).
- [ ] **Architecture:** Feature affects architecture: SpecKit feature forces update of the SpecDD .sdd
- [x] **Security:** Envelope Encryption (AES-GCM) is NOT bypassed for PII data.
- [x] **Data Model:** Core PostgreSQL schemas remain intact.
- [x] **Standards:** SPDX headers and English documentation will be maintained.

## Acceptance Criteria

- [ ] `./build_linux_appimage.sh` creates a runnable AppImage.
- [ ] The AppImage shows the icon and stores `configs/`, `data/backup` next to the `.AppImage` file.
- [ ] Non-AppImage builds behave as before.
- [ ] CHANGELOG.md and README are up-to-date.
