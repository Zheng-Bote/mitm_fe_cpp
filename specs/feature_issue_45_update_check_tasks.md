# Tasks: Version Check Without GitHub API (Issue #45)

Spec: `specs/feature_issue_45_update_check.md` – branch `feature/issue-45-update-check`.

- [x] T1 `src/UpdateCheck.h`: `latestReleaseUrl`, `versionFromReleaseTagUrl`, `isProxyError`, `failureText`, `fetchLatestVersion` (HEAD, manual redirect, configured proxy). (R1, R3, R4)
- [x] T2 `src/MainWindow.cpp`: startup check and About dialog use `fetchLatestVersion`; User Guide uses `ConfigManager::GetProxy()`. (R1, R2, R5)
- [x] T3 `tests/UpdateCheckTest.cpp` + `CMakeLists.txt` test target.
- [x] T4 Version 2.2.1 (`CMakeLists.txt`, `include/rz_config.hpp`), `CHANGELOG.md`.
- [x] T5 Build and run tests (`ctest`).

## Verification
- `ctest`: CryptoTest, ApiEnvelopeTest, UpdateCheckTest passed.
- Probe against real GitHub with the Qt implementation: no proxy → `2.1.0`; through an authenticating HTTP CONNECT proxy → `2.1.0` (only `github.com:443` tunnelled); wrong proxy password → "Proxy requires authentication" + proxy hint; unreachable proxy → "Proxy connection refused" + proxy hint.
