<div id="top" align="center">
<h1>MitM Admin Frontend</h1>

<p>Admin Frontend for the MitM Data Aggregator project. It connects securely to the Go-based Scheduler via REST APIs and high-performance FlatBuffers binary endpoints to manage jobs, mapping rules, and view system and audit logs.</p>

![License](https://img.shields.io/badge/license-Apache_2.0-green)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Windows%20%7C%20macOS-lightgrey.svg)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/Zheng-Bote/mitm_fe_cpp?logo=GitHub)](https://github.com/Zheng-Bote/mitm_fe_cpp/releases)
<br/>
[Report Issue](https://github.com/Zheng-Bote/mitm_fe_cpp/issues) · [Request Feature](https://github.com/Zheng-Bote/mitm_fe_cpp/pulls)

</div>

---

<!-- START doctoc generated TOC please keep comment here to allow auto update -->

<details>
<summary>Table of Contents</summary>

- [Features](#features)
  - [Status](#status)
- [Requirements](#requirements)
- [Building the Project](#building-the-project)
  - [1. Install Conan Dependencies & Generate Presets](#1-install-conan-dependencies-generate-presets)
  - [2. Configure with CMake](#2-configure-with-cmake)
  - [3. Build](#3-build)
  - [4. Configuration Setup](#4-configuration-setup)
  - [5. Run](#5-run)
  - [6. Package as macOS DMG](#6-package-as-macos-dmg)
  - [7. Package as Linux AppImage](#7-package-as-linux-appimage)

</details>

<!-- END doctoc generated TOC please keep comment here to allow auto update -->

## Features

- **Encrypted Configuration**: Master configuration (`config.enc`) and user-specific configurations (`<username>_config.enc`) are protected via AES-256-GCM and Argon2id (libsodium). At startup, the application prompts for a master password.
- **Configuration Profiles**: Switch seamlessly between multiple encrypted environments directly from the UI. The active configuration is persisted and displayed in the status bar.
- **Automatic Version Check**: Uses `gh-update-checker` to asynchronously verify if a new release of the frontend is available on GitHub and displays the status in the "About" dialog.
- **Data Decryptor & Key Vault**: Safely decrypt Envelope-Encrypted JSON payloads securely in the browser. The "Settings & Key Vault" securely stores the `MASTER_KEY` (KEK) in memory (protected by `libsodium`) for Zero-Knowledge operations.
- **Dynamic RBAC & Read-Only Mode**: Dynamically fetches the OS user's roles from the backend on startup. Modifies the UI to restrict or allow access (e.g. read-only "Viewer" mode disables all edits, full access for "Admin").
- **Manual File Upload**: Allows authorized users (UPLOADER/ADMIN) to securely upload and process `.csv` and `.xlsx` raw data files on the fly.
- **WebAuthn FIDO2 Hardware Authentication**: Modern cross-platform passkey and security key (YubiKey) authentication powered by `libfido2` and `libcbor`. Features interactive PIN entry, hardware presence/lock detection, and physical touch guidance.
- **Dedicated Security Key Management**: Logged-in users can enroll new or secondary security keys directly from `Settings -> Register YubiKey / Security Key...`.
- **RBAC Department Management**: Department field integration across user management tables and Add/Edit user dialogs.
- **Windows Hello Authentication**: Supports fallback biometric authentication via WinRT for accessing sensitive areas on Windows.
- **Network Proxy Settings**: Configurable per-user network proxy settings that override defaults and support authenticated HTTP/HTTPS proxies. Used for the User Guide (`raw.githubusercontent.com`) and the version check (`github.com/<repo>/releases/latest`, no GitHub API rate limit).
- **Advanced Logging & Dashboard**: Interactive dashboard with real-time stats and comprehensive data grids for Admin, System, Job, and Transformation logs.
- **Auto-Refresh**: Background periodic auto-refresh (5s interval) for Dashboard, Scheduler, and Log grids, configurable per user without UI flickering.
- **Dynamic API & Auth**: Derives the Scheduler host and authentication header directly from the decrypted config.
- **HTTPS Support**: Configurable HTTPS connectivity (`scheduler_use_https`) to securely interface with FQDNs.
- **Local Timezone Formatting**: Seamlessly translates UTC timestamps to the user's local timezone directly within data grids.
- **Scheduler Next Run**: Dynamically calculates and displays upcoming job execution times in the user's local timezone directly in the jobs grid.
- **Scheduler Active State & Control**: Displays real-time process execution status (Running PID / Idle) and allows users with the `ADMIN` role to terminate active jobs directly from the UI (`⏹ Stop Selected`).
- **Rule Sandbox**: Real-time "Test Preview" sandbox to safely simulate complex data transformations (e.g., Dates and Regex) before saving them to the backend.
- **Encrypted Backup & Restore**: Exports and imports the complete system configuration via the backend API (`POST /api/v1/system/backup` / `restore`, requires `mitm_http-server` ≥ 3.6.0): jobs, scheduler settings, sources, targets, transformation rules, storage keys, users with roles and WebAuthn keys, and the Casbin RBAC rules.
  - The backup is encrypted by the server with AES-256-GCM; the key is derived via Argon2id from a passphrase (at least 12 characters, entered twice) that is never stored. Without the passphrase a backup cannot be restored.
  - Saved as `data/backup/<timestamp>_backup-<config>_<user>.mitmbak`, readable only by the current OS user. Old unencrypted `.json` backups are rejected.
  - Restore shows the restored tables/rows; jobs that are not in the backup are deactivated, not deleted. The server refuses a restore that would lock out the restoring user.
  - Available to `ADMIN`. The *Backup/Restore* tab is also shown to the `BACKUP-RESTORE` role, which additionally needs Casbin policies for both endpoints (see [backup_restore.md](https://github.com/Zheng-Bote/mitm_core-layer/blob/main/mitm_http-server/docs/backup_restore.md)).
- **Self-contained Packages**: Linux AppImage (`build_linux_appimage.sh`) and macOS DMG (`build_macos_dmg.sh`), each with app icon and all Qt libraries bundled.
- **Modern C++26 Stack**: Built with Qt6, FlatBuffers, nlohmann_json, spdlog, libsodium, and cpp-httplib.

- **Spec Driven Development** for manual development or **AI Engineering** for AI Driven Specification and Development using [SpecDD](https://specdd.ai) and [Github Spec-Kit](https://github.github.com/spec-kit/) supported.

See [AI_Workflow_DE.md](https://github.com/Zheng-Bote/mitm-2/blob/main/docs/AI_Workflow_DE.md) or [AI_Workflow_EN.md](https://github.com/Zheng-Bote/mitm-2/blob/main/docs/AI_Workflow_EN.md).

### Status

![GitHub Created At](https://img.shields.io/github/created-at/Zheng-Bote/mitm_fe_cpp?logo=GitHub)
![GitHub Release Date](https://img.shields.io/github/release-date/Zheng-Bote/mitm_fe_cpp?logo=GitHub)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/Zheng-Bote/mitm_fe_cpp?logo=GitHub)](https://github.com/Zheng-Bote/mitm_fe_cpp/releases)
![GitHub repo size](https://img.shields.io/github/repo-size/zheng-bote/mitm_fe_cpp)
![Status](https://img.shields.io/badge/Status-stable-green)

## Requirements

- **Compiler**: C++23/C++26 compatible (GCC 13+, Clang 16+, MSVC 19.38+)
- **Build System**: CMake 3.25+
- **Package Manager**: Conan 2.x
- **Framework**: Qt6 (Core, Gui, Widgets, Network)
- **System Dependencies**: OpenSSL, libfido2 (`libfido2-dev` on Linux, `brew install libfido2` on macOS)

## Building the Project

We use Conan to fetch third-party C++ libraries (`flatbuffers`, `spdlog`, `nlohmann_json`, `libsodium`, `cpp-httplib`, `openssl`, `libcbor`) and CMake to build the Qt6 application. `libfido2` headers and `QXlsx` are fetched via CMake's `FetchContent`.

### 1. Install Conan Dependencies & Generate Presets

**Windows**

Run this from the `admin-frontend/mitm_fe_cpp` directory:

```bash
conan install . --build=missing -s build_type=Release
```

**Linux**

Run this from the `admin-frontend/mitm_fe_cpp` directory:

```bash
conan install . --build=missing -pr:b default
```

**macOS (Apple Silicon, Homebrew)**

Prerequisites: Xcode Command Line Tools and Homebrew. CMake 3.30+ is required, because older versions do not know the C++26 flag for AppleClang.

```bash
brew install qt cmake ninja conan libfido2
conan profile detect   # only once
conan install . --build=missing -s build_type=Release -c tools.cmake.cmaketoolchain:generator=Ninja
```

### 2. Configure with CMake

Conan generates modern CMake presets. Use them directly from the project root:

**Windows**

```bash
cmake --preset conan-default -DCMAKE_VS_GLOBALS=VcpkgEnabled=false
```

**Linux**

```bash
cmake --preset conan-release
```

**macOS**

```bash
cmake --preset conan-release -DCMAKE_PREFIX_PATH=/opt/homebrew
```

_Note: If Qt6 is not in your system path, you may need to add `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64` to the configure command._

### 3. Build

```bash
cmake --build --preset conan-release -j
```

### 4. Configuration Setup

Before running the application for the first time, you need a valid encrypted `config.enc`. This is usually generated by the backend or via a dedicated script using the same Master Key and Argon2id parameters. The frontend will ask for the password to decrypt this file at startup. By default, it expects to find `config.enc` in the same directory as the executable, or you can pass the path as a command-line argument. Once loaded, the selection is saved and remembered for future startups. User-specific proxy configurations are saved under the `configs/` directory.

When running as AppImage, "the directory of the executable" is the directory that contains the `.AppImage` file (the AppImage itself is mounted read-only): `config.enc`, `configs/` and `data/` (incl. `data/backup`) are located there.

### 5. Run

```bash
./build/Release/bin/mitm_fe_cpp [path/to/config.enc]
```

**macOS** (an app bundle is built)

```bash
./build/Release/bin/mitm_fe_cpp.app/Contents/MacOS/mitm_fe_cpp [path/to/config.enc]
```

**Linux AppImage** (see [7. Package as Linux AppImage](#7-package-as-linux-appimage))

```bash
chmod +x mitm_fe_cpp-<version>-x86_64.AppImage   # once, after copying/downloading
./mitm_fe_cpp-<version>-x86_64.AppImage [path/to/config.enc]
```

**Windows**

```bash
<your_Qt_path>\windeployqt6.exe --release -force-openssl mitm_fe_cpp.exe
```

### 6. Package as macOS DMG

`build_macos_dmg.sh` builds the Release configuration and packages it as a self-contained disk image (Apple Silicon, Homebrew prerequisites as in step 1):

```bash
./build_macos_dmg.sh            # build (Conan + CMake) and create the DMG
./build_macos_dmg.sh --no-build # only package an existing build/Release
```

Output: `build/dmg/mitm_fe_cpp-<version>-macos-arm64.dmg`

The script

- creates the app icon (`.icns`) from `img/logo_256x256.png` and sets version, bundle ID and icon in `Info.plist`,
- bundles the Qt frameworks and Homebrew libraries (e.g. libfido2) via `macdeployqt`,
- removes the SVG icon-engine and PDF image-format plugins: Homebrew keeps their frameworks in separate kegs that `macdeployqt` cannot resolve (the resulting `Cannot resolve rpath … QtPdf/QtSvg` messages can be ignored), and the app needs neither,
- signs the bundle ad hoc and creates the DMG with an `Applications` shortcut.

_Note: The DMG is not notarized. On other Macs, Gatekeeper blocks the first launch; open the app via right-click → **Open**. Distribution to other users requires a Developer ID signature and notarization._

### 7. Package as Linux AppImage

`build_linux_appimage.sh` builds the Release configuration and packages it as a self-contained AppImage (prerequisites as in step 1, plus `curl` and `file`):

```bash
./build_linux_appimage.sh            # build (Conan + CMake) and create the AppImage
./build_linux_appimage.sh --no-build # only package an existing build/Release
QT_PREFIX=~/Qt/6.12.0/gcc_64 ./build_linux_appimage.sh   # Qt from the Qt installer
```

Output: `build/appimage/mitm_fe_cpp-<version>-x86_64.AppImage`

The script

- finds Qt via `QMAKE`, `QT_PREFIX`, `Qt6_DIR` in `build/Release/CMakeCache.txt` or `qmake6`/`qmake` in `PATH`,
- downloads [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy) and [linuxdeploy-plugin-qt](https://github.com/linuxdeploy/linuxdeploy-plugin-qt) once to `build/appimage/tools` (no FUSE needed),
- bundles the Qt libraries and plugins (xcb, and Wayland if the Qt installation ships it: `libqwayland.so` since Qt 6.8, `libqwayland-generic.so` before) and libfido2, adds a desktop entry and the icon `img/logo_256x256.png`,
- removes its temporary `AppDir`; the downloaded tools stay in `build/appimage/tools` for the next run (delete them to fetch newer versions).

Tested with Qt 6.12.0 (Qt installer) on Ubuntu 26.04 LTS.

The AppImage keeps its files next to the `.AppImage` file, just like the program folder of a normal installation: `config.enc`, `configs/<user>_config.enc`, `data/` (configurations, `data/backup`).

_Note: The AppImage runs on distributions with the same or a newer glibc than the build host; build on the oldest distribution to support. OpenSSL is used from the target system (Qt loads it at runtime)._
