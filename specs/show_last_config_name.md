# Feature Spec: Show Last Config Name on Password Dialog

## 1. Problem
Currently, when the user starts the C++26/Qt6 admin frontend (`mitm_fe_cpp`), they are presented with a password dialog to decrypt the configuration file. However, it is not clear *which* configuration they are decrypting because the `name` of the configuration is stored within the encrypted JSON and cannot be read before decryption. The user requested to see the name of the last loaded configuration in this password dialog.

## 2. Scope
- **Component:** `admin-frontend/mitm_fe_cpp` (Isolated feature)
- **Affected Areas:**
  - Configuration Management (`Config.cpp` / `Config.h`): To save the `name` of the successfully loaded configuration.
  - User Interface (Password Dialog): To retrieve and display the saved name.

## 3. User Goals & Non-Goals
### Goals
- Extract the `name` field from the configuration (e.g., "mein Konfig-Name") upon successful decryption.
- Save this name locally in an unencrypted format (e.g., using `QSettings`).
- On subsequent application starts, read the unencrypted name and display it in the password dialog so the user knows what they are unlocking.

### Non-Goals
- We will not alter the encryption mechanism of the configuration file itself.
- We will not extract or cache any sensitive data unencrypted (only the configuration `name` field).
- We will not support displaying names of configurations that haven't been successfully decrypted at least once by this client.

## 4. Acceptance Criteria
- [ ] **Caching:** After a successful config decryption, the config `name` is written to `QSettings` (or equivalent unencrypted local app setting).
- [ ] **Retrieval & Display:** When the application starts and the password dialog appears, the dialog retrieves the `name` from `QSettings` and displays it prominently (e.g., "Letzte Konfiguration: mein Konfig-Name").
- [ ] **Fallback:** If no `name` is found in `QSettings` (e.g., first start or before the first successful decryption), the dialog defaults to a generic text or omits the name gracefully.
- [ ] **Security:** No secrets, tokens, or other sensitive configuration details are written to `QSettings`.

## 5. Technical Design (Architecture Plan)
### Target Path
`admin-frontend/mitm_fe_cpp`

### Design Decisions & Architecture Check
1. **Inherited Constraints (`mitm-2.sdd`, `admin-frontend.sdd`, `mitm_fe_cpp.sdd`):**
   - *Rule:* "Never persist plain text credentials, tokens, or PII to the local disk unencrypted."
   - *Check:* The `name` of the configuration is purely descriptive (e.g. "Default Config" or "Production DB") and does not classify as PII, token, or credential. It is safe to store in the unencrypted user settings (`QSettings`) of the OS.
   - *Rule:* "Reduce the lifetime of secrets in memory..."
   - *Check:* We are not modifying how the password (`QInputDialog`) or `m_password` in memory is handled. We are simply appending text to the input dialog's label.

2. **Implementation Strategy:**
   - **Writing the Name:** In `Config.cpp` (`mitm::config::ConfigManager::LoadEncryptedConfig`), right after successfully parsing `j.value("name", "Default Config")`, we will instantiate a `QSettings` object and set the value for the key `"LastConfigName"`.
   - **Reading the Name:** In `main.cpp`, before invoking `QInputDialog::getText`, we will instantiate a `QSettings` object and read the `"LastConfigName"` key (with a fallback like "Unbekannt"). We will then dynamically format the prompt text (e.g., `"Enter password to decrypt configuration (%1):"`).

3. **Dependencies:**
   - Requires `#include <QSettings>` in `Config.cpp`.

This design strictly adheres to the SpecDD boundaries.

## 6. Tasks (Work Packages)
1. **Task 1: Save Last Config Name**
   - **Repository/Module:** `admin-frontend/mitm_fe_cpp`
   - **File:** `src/Config.cpp`
   - **Action:** Add `#include <QSettings>`. In `mitm::config::ConfigManager::LoadEncryptedConfig`, immediately after setting `m_config.name` from the decrypted JSON, create a `QSettings` instance and write `m_config.name` to the key `"LastConfigName"`.

2. **Task 2: Display Last Config Name in Password Dialog**
   - **Repository/Module:** `admin-frontend/mitm_fe_cpp`
   - **File:** `src/main.cpp`
   - **Action:** In the main function, right before `QInputDialog::getText` is called, read `"LastConfigName"` from `QSettings`. Modify the prompt text string to include the name (e.g. `"Enter password to decrypt configuration (%1):"`). Handle the fallback case where the name might be empty or missing.
