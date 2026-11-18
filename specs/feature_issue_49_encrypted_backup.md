# Feature Specification: Encrypted Configuration Backup – Passphrase Dialog (Issue #49)

Issue: https://github.com/Zheng-Bote/mitm_fe_cpp/issues/49 – server: https://github.com/Zheng-Bote/mitm_core-layer/pull/100 (mitm_http-server 3.6.0)

## Feature Intent

Frontend part of [mitm_core-layer#99](https://github.com/Zheng-Bote/mitm_core-layer/issues/99) (configuration backup incomplete / restore broken / backup encryption). The server will encrypt the whole backup with **AES-256-GCM** using an **Argon2id** key derived from an admin **passphrase** (R10). HTTPS is provided by the infrastructure in Int/Prod and not enforced by the server. The backup request changes from `GET` to a request with a body (expected `POST /api/v1/system/backup` with the passphrase; the final contract is defined in core-layer#99 / `docs/api/api_v1.md`).

Current behaviour (`src/BackupRestoreWidget.cpp`):

- Backup: `GET /api/v1/system/backup`, the response `data` is written **unencrypted** as `data/backup/<timestamp>_backup-<config>_<user>.json` with default file permissions.
- Restore: the selected `.json` is sent unchanged via `POST /api/v1/system/restore`.

**Depends on:** core-layer#99 (server must be released first or together; requires `mitm_http-server` with the new backup format).

## Requirements (EARS Syntax)

1. When the user starts a backup, the frontend shall ask for a passphrase twice (entry + confirmation, masked), enforce the server's minimum length (e.g. 12 characters) and send it with the backup request according to the core-layer#99 contract.
2. When the server returns the encrypted backup, the frontend shall save it unchanged (header + ciphertext) under `data/backup/` with owner-only file permissions; the file extension shall reflect the encrypted format (e.g. `.mitmbak`), and `.json` files of the old format shall be rejected on restore with a clear message.
3. When the user starts a restore, the frontend shall ask for the passphrase (masked) after the file selection and the confirmation, and send file and passphrase according to the contract.
4. The frontend shall keep the passphrase only in memory (`mitm::crypto::SecureString` or equivalent), wipe it after the request, and never log, store or display it.
5. When the server rejects the request, the frontend shall show the server's error detail, in particular: wrong passphrase / modified file, passphrase too short, restore rejected because it would lock out the current user, unsupported backup format.
6. The restore confirmation shall state that jobs missing in the backup are **deactivated** (not deleted) and that users, roles, RBAC rules and WebAuthn keys are restored as well.

## Scope

Admin Frontend (mitm_fe_cpp): `src/BackupRestoreWidget.{h,cpp}` (passphrase dialog, request/response handling, file handling), possibly a small reusable passphrase dialog, `docs/UserGuide.md` (backup/restore section), `CHANGELOG.md`, version bump (minor; requires the matching core-layer release).

## SpecDD Architecture Alignment (Drift Control)

- [x] **Architecture:** The layered architecture is maintained (no direct bypass from Collector to Delivery).
- [ ] **Architecture:** Feature affects architecture: SpecKit feature forces update of the SpecDD .sdd
- [x] **Security:** Envelope Encryption (AES-GCM) is NOT bypassed for PII data — the frontend never decrypts the backup; the passphrase stays in memory only.
- [x] **Data Model:** Core PostgreSQL schemas remain intact.
- [x] **Standards:** SPDX headers and English documentation will be maintained.

## Acceptance Criteria

- [ ] A backup asks for passphrase + confirmation and saves a file without plaintext table data, readable only by the owner.
- [ ] A restore with the correct passphrase succeeds; with a wrong passphrase or a modified file the server's error is shown and nothing is changed.
- [ ] Error messages for "passphrase too short", "would lock out current user" and "old .json format" are shown clearly.
- [ ] The passphrase does not appear in logs or files.
- [ ] CHANGELOG.md and docs/UserGuide.md are up-to-date.

