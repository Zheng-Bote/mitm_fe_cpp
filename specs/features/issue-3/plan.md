# Feature: Ops Data Decryptor Frontend (Issue 3)

## Plan
1. **admin-frontend/mitm_fe_cpp/src/Crypto.h/cpp**: Add `EnvelopeDecrypt(const std::vector<uint8_t>& kek, const std::vector<uint8_t>& wrappedKey, const std::vector<uint8_t>& payloadNonce, const std::vector<uint8_t>& payload)` using OpenSSL/libsodium (AES-256-GCM without Argon2id, matching the Go implementation).
2. **admin-frontend/mitm_fe_cpp/src/ApiClient.h/cpp**: Add method `fetchStorageKeys(const QString& username, const QString& password, std::vector<std::string>& outKeys)` to call `/admin/storage-keys`.
3. **admin-frontend/mitm_fe_cpp/src/DataDecryptorWidget.h/cpp**: Create the widget UI (MASTER_KEY password field, encrypted JSON text area, decrypted JSON text area, Decrypt button). On click, parse JSON (it should have `nonce` and `ciphertext` fields base64 encoded), fetch keys, iterate to decrypt.
4. **admin-frontend/mitm_fe_cpp/src/SettingsWidget.cpp**: Add `DataDecryptorWidget` as a new tab.

## Tasks
- [ ] Task 1: Add `EnvelopeDecrypt` to `Crypto.cpp`.
- [ ] Task 2: Add `fetchStorageKeys` to `ApiClient.cpp`.
- [ ] Task 3: Create `DataDecryptorWidget`.
- [ ] Task 4: Register widget in `SettingsWidget`.
