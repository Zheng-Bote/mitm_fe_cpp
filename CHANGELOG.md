# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [v1.3.0] - 2026-10-10

### Added
- **JSON Pre-Validation**: Introduced a global `JsonValidator` and integrated it into all UI dialogs (Rules, Transformations, Validations, Jobs) to validate JSON input before saving. This prevents silent data loss of malformed JSON strings and provides direct visual error feedback to the user.
- **UI Actions**: Added a "Delete Selected" button to the Source Credentials and Target Credentials widgets to easily remove configurations via the UI.

## [v1.2.1] - 2026-10-07

### Fixed

- **API V1 Refactor**: Migrated broken legacy `/admin/*` API calls to `/api/v1/*` in `MainWindow.cpp`, `UploadWidget.cpp`, `AuditLogsWidget.cpp`, and `TopicDependenciesWidget.cpp` to restore functionality after the backend removed the v0 API.

## [v1.2.0] - 2026-10-06

### Removed

- **Authentication**: Removed the obsolete `admins` array parsing from configuration files and the frontend-side authentication logic. Admin rights are now evaluated exclusively by the backend API v1 via "Trust Proxy" logic based on the OS User identity.

## [v1.1.0] - 2026-10-05

### Changed

- **API V1 Refactor**: Fully updated all API calls across the frontend to align with the backend's new resource-oriented REST architecture (e.g., `/api/v1/system/dashboard`, `/api/v1/jobs`, `/api/v1/logs/system`).
- **Dashboard Widget**: Migrated Dashboard stats to consume the unified JSON from `/api/v1/system/dashboard`, rendering the old FlatBuffer counting logic obsolete.
- **Content Negotiation**: Added conditional `Accept: application/x-flatbuffers` headers specifically to log extraction GET requests, restoring compatibility with the refactored endpoints.

## [v1.0.0] - 2026-10-03

### Changed

- **Authentication**: Migrated the frontend authentication flow from legacy API `v0` (Basic Auth) to the new `v1` REST API (Session-Based Auth).
- **Security**: The session token is now securely stored exclusively in volatile memory (`SecureString` in RAM) and is no longer persisted to disk, fully complying with the `admin-frontend.sdd` constraints.
- **Architecture**: `ApiClient` now automatically handles API v1 Content Negotiation via `Accept: application/json` and injects the dynamic session token as a Bearer header.
- **Error Handling**: The application now gracefully handles `401/403` Unauthorized events (e.g. idle timeouts) by immediately triggering the Windows Hello re-authentication flow, closing the UI safely if re-authentication fails.

## [v0.32.0] - 2026-10-03

### Added

- **UI**: The password dialog now displays the name of the last successfully loaded configuration (retrieved from unencrypted local app settings).

## [v0.31.0] - 2026-09-29

### Added

- Added "Auto-Refresh" checkbox for Dashboard, Scheduler, Audit Logs, and System Logs to periodically refresh data every 5 seconds without UI flicker. Preferences are stored per-user via QSettings.

## [v0.30.0] - 2026-09-08

### Added

- **UI**: Added a new "Data Decryptor" tab under "System / Settings" allowing administrators to decrypt Envelope-Encrypted JSON payloads securely in the browser (Zero-Knowledge Architecture).
- **Crypto**: Natively implemented AES-GCM Envelope Decryption in C++ via libsodium to unwrap DEKs using the MASTER_KEY.
- **Key Vault**: Implemented core memory state and UI controls (Unlock/Lock) for the "Settings & Key Vault" tab to securely store and wipe the `MASTER_KEY` (KEK) using `SecureString` and `libsodium` (Issue #6).

## [v0.29.0] - 2026-09-02

### Changed

- **UI Structure**: Grouped the "Source Credentials", "Target Credentials", "RBAC", "Settings & Key Vault", and "Backup/Restore" tabs under a new centralized "System / Settings" top-level tab (Issue #7).

### Fixed

- **Export Report**: Fixed a bug where the Job-Audit-Log Export Report could be completely empty if the requested report date range fell outside the currently filtered UI data view. The export generation now safely and accurately fetches the full API payload directly based on the chosen export dates (Issue #6).
- **CI Pipeline**: Fixed the GitHub Actions `ci.yml` build paths to correctly recognize the repository root when running as a standalone repository instead of a monorepo subdirectory.

## [v0.28.0] - 2026-09-02

### Added

- **Dashboard**: Extended the Dashboard to display Database Information (name, version, size) and Dead Letter Queue (DLQ) cursor counts, integrating with the new `/admin/dashboard/stats` backend API endpoint (Issue #4).
- **Audit Logs**: Added an optional date range filter (start and end date) for fetching Job-Audit-Logs via the `/admin/logs/job-audit_bin` API (Issue #3).
- **About Dialog**: The "Info -> About" dialog now provides a clickable hyperlink to the latest GitHub release if an update is available, opening the system default browser (Issue #5).

### Changed

- **Export Report**: The default "Start Date" and "End Date" in the Job-Log Export Report dialog now default to yesterday, and the suggested filename format has been updated to `<yyyy-mm-dd>__<topic>_report__<start_date>-<end_date>.xlsx` (Issue #2).

## [v0.27.0] - 2026-09-01

### Added

- **Testing & CI**: Implemented CTest unit testing for cryptographic logic (`CryptoTest.cpp`) and integrated a GitHub Actions continuous integration pipeline (`ci.yml`).
- **Security**: Added Conan dependency lockfiles (`conan.lock`) to serve as a Software Bill of Materials (SBOM) and integrated Aqua Security Trivy filesystem vulnerability scanning into the CI pipeline.
- **Telemetry**: Added network telemetry tracking via `QElapsedTimer` and `spdlog` to record latency and HTTP response codes within the centralized `ApiClient` and update checker.

### Changed

- **Dependencies**: Removed `gh-update-checker` and implemented native, asynchronous GitHub API update checking using `QNetworkAccessManager` to prevent unauthenticated proxy credential enforcement.
- **Architecture**: Completed migration of the remaining UI widgets to the new `ApiClient`, entirely replacing ad-hoc `QNetworkAccessManager` usage.

### Fixed

- **Security**: Fixed UI-layer authorization bypasses. Execution flows in `SchedulerWidget` and `UploadWidget` now strictly enforce local `ADMIN` / `UPLOADER` role validation before triggering sensitive actions.
- **Security**: The application now seamlessly detects `401`/`403` HTTP status codes via `ApiClient` and gracefully halts, prompting the user to restart and re-authenticate.
- **Update Checker**: Fixed a logic bug in semantic version comparison where the application incorrectly reported newer local builds as outdated compared to older remote GitHub tags.

## [v0.26.0] - 2026-08-31

### Added

- **Security**: Implemented `SecureString` wrapper leveraging `sodium_malloc` and `sodium_memzero` for secure memory allocation and wiping of proxy credentials and master password.
- **Architecture**: Introduced a centralized `ApiClient` singleton to unify HTTP status code handling, schemas, timeouts, and error responses across UI components.

### Changed

- **Architecture**: Refactored 9 core widgets (including Dashboard, RBAC, Scheduler) to utilize the new `ApiClient`, deprecating direct `QNetworkAccessManager` usage.
- **Build System**: Increased the `cmake_minimum_required` version to `3.25` and updated the C++ standard to `C++26` for enhanced modern language features.

## [v0.25.0] - 2026-08-31

### Fixed

- **Security**: Removed the hardcoded fallback authentication token (`helo_linux`) in `ConfigManager::GetAuthHeader`. Unauthenticated requests now fail instead of silently passing.
- **Security**: Fixed a use-after-free risk in the About dialog's asynchronous update check by guarding the status label with `QPointer`.
- **Security**: Sanitized the OS username (`USER`/`USERNAME`) against path traversal before using it in configuration file paths.

### Changed

- **Documentation**: Synced the in-app User Guide with the recent UI and security changes. Removed `docs/todo.md` in favor of `admin-frontend.sdd`.

## [v0.24.0] - 2026-08-24

### Added

- **User Guide**: Integrated an in-app "User Guide" viewer under the "Info" menu. It fetches the remote Markdown documentation over HTTP (respecting the user's optional network proxy) and natively renders it inside a `QTextBrowser`. It dynamically rewrites GitHub repository URLs to their `raw.githubusercontent.com` endpoints.
- **Export Report**: The "Save Report" dialog now remembers the last used directory via `QSettings` and defaults to it for subsequent exports.
- **Export Report**: Added a horizontal summary block in the "Upload-Report" sheet (starting at `I3`) that uses native Excel `=SUM(...)` formulas to dynamically calculate the totals for all numeric columns (Records Total, Added, Updated, Skipped, Rejected, Errors).

### Fixed

- **Export Report**: Fixed a bug where duplicate rows appeared in the "Upload-Report" sheet. The regex parser now actively ignores the raw JSON `Response:` payload to prevent double-counting when parsing SaaS statistics.

## [v0.23.0] - 2026-08-23

### Added

- **Export Report**: Added "Export Report" functionality to the Job-Logs tab. It generates a formatted Excel file (`.xlsx`) using the `QXlsx` library containing aggregated Upload Statistics (Records Added, Updated, Skipped, Rejected, Errors).
- **Export Report**: Included a Pie Chart in the Excel export to visualize the distribution of upload statistics.
- **Export Report**: Added dynamic parsing of unstructured log messages via Regex to deduce statistics.
- **Dependencies**: Added `QXlsx` (QtExcel) dependency for native `.xlsx` generation.

### Fixed

- **Export Report**: Fixed `sharedStrings.xml` corruption in Excel exports by properly truncating massive SaaS log responses exceeding the 32,767 character limit.

## [v0.22.0] - 2026-08-10

### Added

- **DLQ & Cursors**: Implemented API integration for the `"Requeue Selected"` action, triggering a POST request to `/admin/dlq/requeue?id=...`. Included RBAC enforcement restricting usage to the `ADMIN` role.
- **DLQ & Cursors**: Added a new `"ID"` column to the DLQ table to make the unique FlatBuffer entry identifier visible and selectable.

## [v0.21.0] - 2026-08-10

### Added

- **Scheduler**: Added a new `"▶ Execute Selected"` button allowing administrators to manually trigger job execution via the `/admin/execute-job` API endpoint. Includes RBAC enforcement restricting usage to the `ADMIN` role.

## [v0.20.0] - 2026-08-10

### Added

- **Scheduler**: Added `*/6`, `*/8`, and `*/12` hour intervals to the cron expression dropdown in the Job Editor.

### Changed

- **DLQ & Cursors**: Improved UI rendering and interaction performance by truncating massive `Error Message` and `Payload Snippet` strings to 256 characters within the QTableWidget.

## [v0.19.0] - 2026-08-09

### Added

- **Backup & Restore**: Added a new "Backup/Restore" tab to seamlessly export and import the complete system configuration (jobs, sources, targets, rules) as JSON via the backend API.
- **RBAC**: The new tab is restricted to users with the `BACKUP-RESTORE` or `ADMIN` role.
- **Local Backup Storage**: Downloaded configurations are automatically saved to `<Binary-Folder>/data/backup/<yyyy-mm-dd_HHmmss>_backup-<name>_<user>.json`.

## [MVP-2.7.0-1-g772c928] - 2026-07-29

### Added

- **Delivery Layer**: Implemented configurable `slowdown` and `timeout` parameters for the `CORITY_SAAS` delivery adapter.

### Changed

- **Database**: Synced PostgreSQL database schema IST-Zustand across all layer `.sql` migrations (`setup.sql`, `transformation-layer`, `delivery-layer`, `scheduler`).
- **Components Logging**: Refactored component version logging mechanism across all layers (Collectors, Transformation, Delivery, Scheduler) to consistently output a clean `Major.Minor.Patch` version format.

### Fixed

- **Scheduler**: Resolved an HTTP 500 error on the `/admin/transformation/errors_bin` API endpoint by updating the query to correctly reference the `raw_ingestion_id` column and gracefully handle null values.

## [v0.18.0] - 2026-07-27

### Added

- **Job Cancellation / Stopping**: Added a `"⏹ Stop Selected"` button to the Scheduler tab allowing administrators to terminate running jobs via the `/admin/stop-job` API endpoint.
- **Active State Column**: Added an `"Active State"` column to the Scheduler jobs table displaying real-time execution status (e.g., `"Running ⚙️ (PID <pid>)"` or `"Idle"`).
- **RBAC Enforcement**: Restricted job stopping functionality strictly to users with the `ADMIN` role. The stop button is automatically disabled for non-admin users, and an active confirmation check prevents unauthorized execution attempts.

## [v0.17.0] - 2026-07-26

### Added

- **FlatBuffers Support**: Added `flatbuffers` (v23.5.26) dependency via Conan and integrated generated schema headers in `include/schematas/`.

### Changed

- **Binary Log APIs**: Updated all log and audit monitoring widgets (`DlqWidget`, `SystemLogsWidget`, `AuditLogsWidget`, `AdminLogsWidget`, `TransformationErrorsWidget`, and `DashboardWidget`) to consume FlatBuffers-serialized binary endpoints (`/admin/dlq_bin`, `/admin/logs/system_bin`, `/admin/logs/job-audit_bin`, `/admin/logs/admin-audit_bin`, `/admin/transformation/errors_bin`) instead of JSON for improved deserialization performance and reduced network payload sizes.

## [v0.16.0] - 2026-07-19

### Added

- **Configuration Profiles**: Added a "Select Configuration..." option in the Settings menu to switch between different encrypted environment configurations (`*.enc`). The selection is persisted via `QSettings` and loaded automatically on the next startup.
- **Active Configuration Display**: The application status bar now displays the name of the currently active configuration, parsed from a new `name` field within the encrypted JSON config.

### Changed

- **User Config Location**: User-specific proxy configurations (`<username>_config.enc`) are now saved cleanly inside a dedicated `<Programm-Ordner>/configs/` directory instead of the binary root folder.

