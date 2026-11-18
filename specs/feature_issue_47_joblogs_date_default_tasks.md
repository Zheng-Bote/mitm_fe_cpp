# Tasks: Jobs-Logs Date Filter Active by Default (Issue #47)

Spec: `specs/feature_issue_47_joblogs_date_default.md` – branch `feature/issue-47-joblogs-date-default`.

- [x] T1 `src/AuditLogsWidget.cpp`: "Filter by Date" checked, start/end date = today, date fields enabled. (R1, R2, R3)
- [x] T2 Version 2.2.2 (`CMakeLists.txt`, `include/rz_config.hpp`), `CHANGELOG.md`.
- [x] T3 Build and `ctest`.

Note: the backend (`mitm_http-server` `get_job_audit_logs`) interprets date-only `from`/`to` as UTC 00:00:00 / 23:59:59 (inclusive).
