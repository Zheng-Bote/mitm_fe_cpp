# Tasks: API Response Envelope & RMM Level 2 (core-layer #93)

Backend: `core-layer/mitm_http-server` 3.5.0 (branch `feature/issue-93-api-standardization`), contract in `core-layer/mitm_http-server/docs/api/api_v1.md` ("Response Envelope").
Issue: https://github.com/Zheng-Bote/mitm_core-layer/issues/93 – cross-layer spec: `mitm-2/specs/features/issue-93-api-standardization/`.

Every JSON response under `/api/v1/*` is `{id, status, code, title, detail, meta, data}`; payload in `data`, errors with `data: null`. FlatBuffers `200` responses stay binary. Removed routes: `POST /iam/assign-role`, `POST`/`DELETE /config/transformations/topic-dependencies`, bulk `POST /jobs`.

- [ ] **T5 – C++ Frontend Envelope Adaptation (`admin-frontend/mitm_fe_cpp`)**: Update `mitm_fe_cpp` HTTP response handling to unwrap `data` and surface `code`/`title`/`detail`. Detailed tasks: `admin-frontend/mitm_fe_cpp/specs/feature_issue_93_api_envelope_tasks.md` (branch `feature/issue-93-api-envelope`).
  - [ ] T5.1 `src/ApiClient.cpp` `handleReply`: pass `data` of 2xx JSON envelopes to `onSuccess` (not for `application/x-flatbuffers`); pass `detail`/`title` (+ `code`, `id`) to `onError` instead of `reply->errorString()`.
  - [ ] T5.2 Raw `QNetworkRequest` calls bypass `ApiClient`: `LoginDialog.cpp` (bootstrap/status, webauthn login/register start/finish) and `AuthManager.cpp` (`/auth/me`) → unwrap `data`; replace `errors[0].detail` parsing (`LoginDialog.cpp:381, 526, 581, 743`).
  - [ ] T5.3 `RbacWidget.cpp:192`: `POST /iam/assign-role` → `PUT /iam/users/:id/roles`.
  - [ ] T5.4 `TopicDependenciesWidget.cpp:120/141`: `POST` → `PUT /topic-dependencies/:topic` with `{required_sources}`; `DELETE ?topic=` → `DELETE /topic-dependencies/:topic` (URL-encode the topic).
  - [ ] T5.5 `SchedulerWidget.cpp:211/243`: add → `POST /jobs` with a single object; edit → `PUT /jobs/:name`.
  - [ ] T5.6 `BackupRestoreWidget.cpp:43`: save `data` (the backup payload) to the file, not the envelope; otherwise restore fails with 422.
  - [ ] T5.7 `SourcesWidget.cpp` / `TargetCredentialsWidget.cpp`: create answers `201 {id}`; use `PUT /:id` for updates; delete answers `200` instead of `204`.
  - [ ] T5.8 Check 403 handling (`ApiClient.cpp:53` emits `unauthorized` for 403 too); update tests.
