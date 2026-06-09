# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.4.0] - 2026-06-09

### Added
- **Auto-Map**: Added "✨ Auto-Map (Smart Suggest)" button to the Transformation Rules UI, which automatically generates mapping rules from comma-separated headers.
- **Admin Logs**: Added a "details" column to display granular event information from the backend.

### Fixed
- **Status Bar**: Fixed a bug where opening `Info` -> `About` would wipe out the persistent status bar showing the current user and program version.

## [0.2.0] - 2026-06-07

### Added

- **Proxy Configuration**: Added `Settings` -> `Network-Proxy` dialog for configuring HTTP/HTTPS proxies. Proxies are stored securely in `<username>_config.enc` using libsodium (AES-GCM/Argon2id).
- **Asynchronous Update Checking**: Integrated `gh-update-checker` via `FetchContent` to asynchronously check for newer releases via the GitHub API, honoring user proxy configurations.
- **About Dialog**: Added `Info` -> `About` menu displaying version, copyright, and update status.
- **App Icons**: Generated and applied modern MitM Shield vector icons (`img/logo_256x256.png` and `img/logo.ico`) to the main application window and About dialog.
- **libsodium Encryption Support**: Implemented `crypto::Encrypt()` alongside `crypto::Decrypt()` for persistent, secure user configuration updates.

### Changed

- **Status Bar**: Cleaned up the status bar to only display the current app version and the detected OS user.
- **Menu Bar**: Forced the native menu bar rendering to `false` ensuring consistent visibility of menus across Linux Desktop Environments.
- **CMake Scripts**: Refactored include directory targeting to properly prefer internal headers over global system includes.
- **Logo**: Replaced the placeholder "M" logo with the modern MitM Shield vector logo on the About dialog.

## [0.1.0] - 2026-06-06

### Added

- Initial C++23/Qt6 project skeleton with CMake and Conan package manager setup.
- **Dashboard Tab:** Live status monitoring querying `/health`, `/info`, and `/admin/jobs`.
- **Scheduler Tab:** Interactive management of scheduled jobs (Create, Edit, Delete) with a user-friendly Cron Expression Editor.
- **System Logs Tab:** Integration with `/admin/logs/system` featuring table sorting and CSV export functionality.
- **Audit Logs Tab:** Integration with `/admin/logs/job-audit` featuring table sorting and CSV export functionality.
- **Rules & Mapping Tab:** UI concept with mock data for Transformation Rules.
- **DLQ & Cursors Tab:** UI concept with mock data for the Dead Letter Queue.
- **Settings & Key Vault Tab:** UI Konzept for safely injecting the Envelope Encryption `MASTER_KEY` via Windows Hello / Password.
- Transparent dynamic authentication using the `USER`/`USERNAME` OS variable to parse tokens directly from `config.json`.
