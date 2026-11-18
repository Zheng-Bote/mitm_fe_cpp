# Feature Specification: Version Check Without GitHub API (Issue #45)

Issue: https://github.com/Zheng-Bote/mitm_fe_cpp/issues/45

## Feature Intent

In environments with a network proxy, the user configures *Settings → Network-Proxy*. After that, *Info → User Guide* loads the guide from the internet, but the version check in *Info → About* fails with "Update check failed. Note: You may need to configure a proxy in the Settings."

Analysis:

- Both requests use the same proxy (`ConfigManager::GetProxy()` vs. an identical copy in `showUserGuideDialog`), so the proxy configuration is not the difference.
- The User Guide is fetched from `raw.githubusercontent.com`; the version check calls the **GitHub REST API** `api.github.com/repos/<repo>/releases/latest`. Unauthenticated API calls are limited to **60 requests per hour per public IP** (`x-ratelimit-limit: 60`). Behind a corporate proxy/NAT all users share one egress IP, and the frontend calls the API on **every application start** plus every About dialog → the limit is quickly exhausted and GitHub answers `403`. (Alternatively the proxy may block `api.github.com` specifically.)
- The About dialog shows the proxy hint for **every** error (`MainWindow.cpp:369`), hiding the real cause.

Fix:

1. Determine the latest version without the API: `HEAD https://github.com/<repo>/releases/latest` answers `302` with `Location: …/releases/tag/v<version>` (no API rate limit, only `github.com` must be reachable).
2. Show the real error (HTTP status / `errorString()`); show the proxy hint only for proxy errors.
3. Use `ConfigManager::GetProxy()` for the User Guide as well (single proxy implementation).

## Requirements (EARS Syntax)

1. When the version check runs (application start and About dialog), the frontend shall send `HEAD <PROJECT_HOMEPAGE_URL>/releases/latest` through the configured proxy without following redirects and shall take the latest version from the `…/releases/tag/<tag>` redirect target (leading `v` removed).
2. The frontend shall not call `api.github.com` for the version check.
3. If the version check fails with a proxy error (connection refused/closed, not found, timeout, authentication required, unknown proxy error), the About dialog shall show the error and the hint to check *Settings → Network-Proxy*.
4. If the version check fails otherwise, the About dialog shall show the HTTP status or the network error text without the proxy hint.
5. The User Guide request shall use `ConfigManager::GetProxy()`.

## Scope

Admin Frontend (mitm_fe_cpp): new `src/UpdateCheck.h`, `src/MainWindow.cpp` (startup check, About dialog, User Guide), `CMakeLists.txt` (test target, version 2.2.1), `include/rz_config.hpp` (version), `tests/UpdateCheckTest.cpp`, `CHANGELOG.md`.

## SpecDD Architecture Alignment (Drift Control)

- [x] **Architecture:** The layered architecture is maintained (no direct bypass from Collector to Delivery).
- [ ] **Architecture:** Feature affects architecture: SpecKit feature forces update of the SpecDD .sdd
- [x] **Security:** Envelope Encryption (AES-GCM) is NOT bypassed for PII data.
- [x] **Data Model:** Core PostgreSQL schemas remain intact.
- [x] **Standards:** SPDX headers and English documentation will be maintained.

## Acceptance Criteria

- [ ] Unit test: the version is parsed from a `releases/tag/v2.1.0` URL; non-tag URLs yield no version.
- [ ] Unit test: the proxy hint appears only for proxy errors; HTTP errors show the status.
- [ ] About dialog shows the latest version behind a proxy where `api.github.com` is rate-limited or blocked.
- [ ] CHANGELOG.md and README.md are up-to-date.
