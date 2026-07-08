# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v0.15.0] - 2026-07-08

### Added
- **Scheduler Next Run**: Added a "Next Run" column to the Scheduler tab that dynamically displays the upcoming execution time of enabled jobs in the user's local timezone.

### Fixed
- **Transformation Errors (DLQ) Crash**: Fixed a critical 500 error on the backend (`/admin/transformation/errors`) caused by a leftover `raw_ingestion_id` reference in the SQL query after the Stateful Aggregation schema migration. The Dashboard "Transformation Errors" indicator is now fully functional again.

## [v0.14.0] - 2026-07-06

### Added
- **Local Timezone Support**: Enhanced Timestamp and "Created At" columns in Admin-Logs, Jobs-Logs, System-Logs, and Transformation Sources to automatically convert and display raw backend UTC timestamps in the system's local time (including the timezone ID in the header).
- **Transformation Rule Live Preview**: Implemented local execution simulation for `parse_date` and `regex_replace` directly within the Rule Editor's "Test Rule" preview feature using Qt's `QDateTime` and `QRegularExpression`.

### Changed
- **Parse Date Configuration**: Updated the `parse_date` rule builder template to omit the now-optional `input_format` parameter by default, correctly leveraging the backend's new auto-detection capabilities.

## [v0.13.0] - 2026-06-30

### Changed
- **Version Bump**: Bumped version to v0.13.0 to align with backend enhancements and system stability patches.

## [v0.12.0] - 2026-06-22

### Changed
- **Version Bump**: Bumped version to v0.12.0 to align with backend system updates and full compatibility with the repaired `GET /admin/transformation/errors` API.

## [v0.11.0] - 2026-06-21

### Added
- **Topic Dependencies UI**: Added a new "Topic Dependencies" tab within the Transformation Layer widget to manage Stateful Aggregation requirements via the backend API.

### Changed
- **Transformation Errors (DLQ)**: Updated the Transformation Errors table header and JSON parsing to display `correlation_id` instead of the deprecated `raw_ingestion_id`, accommodating the new Stateful Aggregation architecture.

## [v0.10.0] - 2026-06-17

### Added
- **HTTPS Support**: Added `scheduler_use_https` property to the configuration file, allowing the frontend to communicate with the scheduler via HTTPS FQDNs.

### Changed
- **RBAC Visibility**: "Source Credentials" and "Target Credentials" tabs are now strictly hidden from users with the `VIEWER` role and require `ADMIN` privileges.

## [v0.9.0] - 2026-06-16

### Added
- **Transformation Errors**: Added a new "Transformation errors" tab under the Logs section, integrating with the `/admin/transformation/errors` API and supporting CSV export.
- **Dashboard Stats**: Expanded the Dashboard with summary cards for Admin Audit Logs, System Logs, Job Audit Logs, and Transformation Errors. Cards now show total counts and format the oldest entry timestamp in a readable `YYYY-MM-DD HH:mm:ss` format.
- **Windows Hello**: Implemented seamless biometric authentication via WinRT (`UserConsentVerifier` and `IUserConsentVerifierInterop`) inside a dedicated MTA `std::thread` to safely unlock the application without freezing the Qt STA main thread.
- **Sortable Transformation Tables**: Enabled column sorting for all tables across the Transformation Layer (Sources, Target Fields, Rules, Transformations, Validations).

### Fixed
- **Qt Table Sorting Bug**: Fixed data corruption bugs in `RbacWidget` and `TransformationWidget` where row items were randomly displaced because Qt aggressively sorted rows during the insertion process.
- **Config Initialization**: Corrected JSON keys (`scheduler_host`/`scheduler_port`) in the `config.json` initialization logic.
- **DLQ Lazy Loading**: Disabled unconditional API fetching in the `DlqWidget` constructor, ensuring data is only requested when the user manually clicks "Refresh".

## [v0.8.0] - 2026-06-15

### Changed
- **Version Bump**: Bumped version to v0.8.0 to align with backend system updates.

## [v0.7.0] - 2026-06-14

### Added
- **Manual Upload Widget**: New tab allowing users with the `UPLOADER` or `ADMIN` role to upload `.csv` and `.xlsx` files manually. The files are securely transmitted to the `/admin/upload/source_file` API and automatically trigger the standalone `mitm_collector_csv-xls` worker for immediate processing.

## [v0.6.0] - 2026-06-14

### Added
- **RBAC Management UI**: Fully functional RBAC widget replacing the previous placeholder. Includes dynamic "Add User", "Remove User", and role assignment capabilities communicating with the Go backend API.
- **Dynamic Authorization**: Frontend now verifies OS user permissions on startup via `/admin/rbac/os_user_roles`. 
- **Read-Only / Viewer Mode**: UI dynamically adapts to user roles. Non-admin users with the `VIEWER` role can view tables but have all editing buttons disabled (save, add, delete, etc.). Unknown users have their access completely restricted to the dashboard, scheduler status, and logs.

## [v0.5.0] - 2026-06-10

### Added
- **Audit Logs Component**: Added a new "Component" column to the Audit Logs table to track which component generated an event. The CSV export was also updated to include this column.
- **DLQ Integration**: Connected the `DlqWidget` to the actual `/admin/dlq` backend API, replacing previous mock data with live Dead Letter Queue records.

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
