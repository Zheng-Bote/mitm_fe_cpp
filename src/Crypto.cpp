/**
 * SPDX-FileComment: Crypto
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file Crypto.cpp
 * @brief Crypto
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "Crypto.h"
#include <sodium.h>

namespace mitm::crypto {

std::vector<uint8_t> Decrypt(const std::vector<uint8_t>& encryptedData, const mitm::crypto::SecureString& password) {
    if (sodium_init() < 0) {
        throw std::runtime_error("Failed to initialize libsodium");
    }

    const size_t SALT_LEN = 16;
    const size_t NONCE_LEN = 12; // crypto_aead_aes256gcm_NPUBBYTES
    const size_t MAC_LEN = 16;   // crypto_aead_aes256gcm_ABYTES

    if (encryptedData.size() < SALT_LEN + NONCE_LEN + MAC_LEN) {
        throw std::runtime_error("Encrypted data is too short");
    }

    // Extract salt, nonce, ciphertext
    const uint8_t* salt = encryptedData.data();
    const uint8_t* nonce = encryptedData.data() + SALT_LEN;
    const uint8_t* ciphertext = encryptedData.data() + SALT_LEN + NONCE_LEN;
    size_t ciphertext_len = encryptedData.size() - SALT_LEN - NONCE_LEN;

    // Derive key using Argon2id (matching Go implementation parameters)
    const uint32_t opslimit = 3;
    const size_t memlimit = 64 * 1024 * 1024; // 64 MB
    
    std::vector<uint8_t> key(crypto_aead_aes256gcm_KEYBYTES);
    
    if (crypto_pwhash(key.data(), key.size(),
                      password.c_str(), password.length(),
                      salt,
                      opslimit,
                      memlimit,
                      crypto_pwhash_ALG_ARGON2ID13) != 0) {
        throw std::runtime_error("Key derivation failed (out of memory?)");
    }

    // Check if hardware AES is available
    if (crypto_aead_aes256gcm_is_available() == 0) {
        throw std::runtime_error("Hardware AES-GCM is not available on this platform");
    }

    // Decrypt
    std::vector<uint8_t> plaintext(ciphertext_len - MAC_LEN);
    unsigned long long plaintext_len_actual = 0;

    if (crypto_aead_aes256gcm_decrypt(plaintext.data(), &plaintext_len_actual,
                                      nullptr,
                                      ciphertext, ciphertext_len,
                                      nullptr, 0,
                                      nonce, key.data()) != 0) {
        throw std::runtime_error("Decryption failed. Incorrect password or corrupted data.");
    }

    plaintext.resize(plaintext_len_actual);
    return plaintext;
}

std::vector<uint8_t> Encrypt(const std::vector<uint8_t>& plaintext, const mitm::crypto::SecureString& password) {
    if (sodium_init() < 0) {
        throw std::runtime_error("Failed to initialize libsodium");
    }

    const size_t SALT_LEN = 16;
    const size_t NONCE_LEN = 12; // crypto_aead_aes256gcm_NPUBBYTES
    const size_t MAC_LEN = 16;   // crypto_aead_aes256gcm_ABYTES

    std::vector<uint8_t> salt(SALT_LEN);
    randombytes_buf(salt.data(), salt.size());

    std::vector<uint8_t> nonce(NONCE_LEN);
    randombytes_buf(nonce.data(), nonce.size());

    // Derive key using Argon2id (matching Go implementation parameters)
    const uint32_t opslimit = 3;
    const size_t memlimit = 64 * 1024 * 1024; // 64 MB
    
    std::vector<uint8_t> key(crypto_aead_aes256gcm_KEYBYTES);
    
    if (crypto_pwhash(key.data(), key.size(),
                      password.c_str(), password.length(),
                      salt.data(),
                      opslimit,
                      memlimit,
                      crypto_pwhash_ALG_ARGON2ID13) != 0) {
        throw std::runtime_error("Key derivation failed (out of memory?)");
    }

    // Check if hardware AES is available
    if (crypto_aead_aes256gcm_is_available() == 0) {
        throw std::runtime_error("Hardware AES-GCM is not available on this platform");
    }

    std::vector<uint8_t> ciphertext(plaintext.size() + MAC_LEN);
    unsigned long long ciphertext_len_actual = 0;

    if (crypto_aead_aes256gcm_encrypt(ciphertext.data(), &ciphertext_len_actual,
                                      plaintext.data(), plaintext.size(),
                                      nullptr, 0,
                                      nullptr,
                                      nonce.data(), key.data()) != 0) {
        throw std::runtime_error("Encryption failed.");
    }

    ciphertext.resize(ciphertext_len_actual);

    // Format: salt + nonce + ciphertext
    std::vector<uint8_t> result;
    result.reserve(SALT_LEN + NONCE_LEN + ciphertext.size());
    result.insert(result.end(), salt.begin(), salt.end());
    result.insert(result.end(), nonce.begin(), nonce.end());
    result.insert(result.end(), ciphertext.begin(), ciphertext.end());

    return result;
}

} // namespace mitm::crypto
