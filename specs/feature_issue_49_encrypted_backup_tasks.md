# Tasks: Encrypted Configuration Backup (Issue #49)

Spec: `specs/feature_issue_49_encrypted_backup.md` – branch `feature/issue-49-encrypted-backup`. Server contract: `core-layer/mitm_http-server/docs/api/api_v1.md` → "Configuration backup and restore" (3.6.0).

- [x] T1 `src/BackupFile.h`: passphrase validation (min. 12, confirmation), backup classification (encrypted / legacy JSON / invalid), restore request body, file name `.mitmbak`, restore summary. (R1–R3, R5)
- [x] T2 `src/BackupRestoreWidget.cpp`: passphrase dialog (masked, confirmation on backup), `POST /api/v1/system/backup {passphrase}`, save `data` unchanged with owner-only permissions, restore with `{passphrase, backup}`, show server error details, confirmation text (deactivation, users/RBAC). (R1–R6)
- [x] T3 `tests/BackupFileTest.cpp` + `CMakeLists.txt`.
- [x] T4 `docs/UserGuide.md`, `CHANGELOG.md`, version 2.3.0 (requires mitm_http-server ≥ 3.6.0).
- [x] T5 Build + `ctest`.

## Verification
- Build + `ctest`: CryptoTest, ApiEnvelopeTest, UpdateCheckTest, BackupFileTest passed.
- Contract checked against `mitm_http-server` 3.6.0 (`BackupDocument` fields `format`, `kdf`, `cipher`, `ciphertext`; `RestoreRequest {passphrase, backup}`; response `{tables, deactivated_programs, scheduler_reloaded}`); no end-to-end run against a live server yet.
- Passphrase wiping is best effort: `QString`/`QByteArray` copies made by Qt (line edit, network request) cannot be wiped reliably.
