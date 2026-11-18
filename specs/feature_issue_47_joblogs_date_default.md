# Feature Specification: Jobs-Logs Date Filter Active by Default (Issue #47)

Issue: https://github.com/Zheng-Bote/mitm_fe_cpp/issues/47

## Feature Intent

In *Logs → Jobs-Logs* the "Filter by Date" checkbox is off by default and the range is preset to today−7 … today (disabled). Opening the view therefore loads up to 1000 job audit entries of all days. The view shall start with the date filter **active** and the range set to **today … today**.

## Requirements (EARS Syntax)

1. When the Jobs-Logs view is created, the "Filter by Date" checkbox shall be checked.
2. When the Jobs-Logs view is created, the start and end date shall be the current date and both date fields shall be enabled.
3. The first load (and auto-refresh) shall use the date range (`from=<today>&to=<today>`); unchecking the box shall load without date filter as before.

## Scope

Admin Frontend (mitm_fe_cpp): `src/AuditLogsWidget.cpp`, `CHANGELOG.md`, version 2.2.2 (`CMakeLists.txt`, `include/rz_config.hpp`).

## SpecDD Architecture Alignment (Drift Control)

- [x] **Architecture:** The layered architecture is maintained (no direct bypass from Collector to Delivery).
- [ ] **Architecture:** Feature affects architecture: SpecKit feature forces update of the SpecDD .sdd
- [x] **Security:** Envelope Encryption (AES-GCM) is NOT bypassed for PII data.
- [x] **Data Model:** Core PostgreSQL schemas remain intact.
- [x] **Standards:** SPDX headers and English documentation will be maintained.

## Acceptance Criteria

- [ ] Opening Logs → Jobs-Logs shows "Filter by Date" checked and today … today, and only today's entries are loaded.
- [ ] Unchecking the box loads entries without date filter.
- [ ] CHANGELOG.md is up-to-date.
