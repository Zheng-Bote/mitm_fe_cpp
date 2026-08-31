# MitM FE C++: Prioritized ToDo List

Based on the Code Quality, Security, and Performance Report (2026-08-27), the following prioritized tasks have been identified for the `mitm_fe_cpp` repository.

## Immediate Priority (Sofort)

- [x] **Remove Hardcoded Fallback Token:** Remove the hardcoded fallback token (`helo_linux`) and prevent any backend requests if valid credentials are not configured.
- [x] **Enforce Backend Authorization:** Verify and enforce backend-side authorization for all administrative routes. The UI Role-Based Access Control (RBAC) must not be treated as a security boundary.
- [x] **Fix Use-After-Free Risk in About Dialog:** Secure the update thread in the About dialog against use-after-free vulnerabilities by utilizing `QPointer<QLabel>` or QObject-bound asynchronous logic.
- [x] **Sanitize OS Username in File Paths:** Stop using the unvalidated OS username (`USER`/`USERNAME` environment variables) directly as part of the configuration file path to prevent path traversal or unintended file access.

## Short-term Priority (Kurzfristig)

- [ ] **Unify API Client Error Handling:** Introduce a common API client module to uniformly handle HTTP status codes, schemas, timeouts, and error responses.
- [ ] **Improve Secret Memory Management:** Reduce the lifetime of secrets in memory. Implement secure memory wiping (`sodium_memzero`) for password and proxy buffers.
- [ ] **Secure Proxy Credentials:** Avoid passing proxy credentials as URL strings in plain text. Do not use dummy credentials (`user:pass`) for unauthenticated proxies.
- [ ] **Consistent UI Role Checks:** Validate user roles before allowing sensitive UI actions and handle `401 Unauthorized` / `403 Forbidden` responses consistently (e.g., prompt for re-authentication).
- [ ] **Pin Dependencies:** Pin the `QXlsx` dependency in CMake to a verified release tag or commit hash instead of tracking the `master` branch.

## Medium-term Priority (Mittelfristig)

- [ ] **Setup Automated Testing:** Establish CTest and Continuous Integration (CI) pipelines for configuration, cryptography, role UX, API error handling, and thread lifecycles.
- [ ] **Dependency Security:** Introduce dependency lockfiles (SBOM) and implement regular vulnerability scanning.
- [ ] **Metrics Collection:** Gather network and UI metrics, including request latencies, error rates, authentication failures, and update checks.
- [ ] **Consolidate Secret Management:** Align the frontend's configuration and credential management with the MitM Scheduler's overall secret management strategy.
