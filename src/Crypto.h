/**
 * SPDX-FileComment: Crypto
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file Crypto.h
 * @brief Crypto
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>
#include "SecureString.h"

namespace mitm::crypto {

/**
 * @brief Decrypts a payload using a wrapped DEK and a KEK (Envelope Decryption).
 * @param kek The Key Encryption Key (MASTER_KEY), usually 32 bytes.
 * @param wrappedKey The wrapped Data Encryption Key (DEK).
 * @param payloadNonce The nonce used to encrypt the payload.
 * @param payload The encrypted payload (ciphertext).
 * @return Decrypted plaintext payload.
 * @throws std::runtime_error on decryption failure.
 */
std::vector<uint8_t> EnvelopeDecrypt(const std::vector<uint8_t>& kek, const std::vector<uint8_t>& wrappedKey, const std::vector<uint8_t>& payloadNonce, const std::vector<uint8_t>& payload);

/**
 * @brief Decrypts data using AES-256-GCM and Argon2id derived key.
 * @param encryptedData The encrypted data (salt + nonce + ciphertext).
 * @param password The password for decryption.
 * @return Decrypted plaintext data.
 * @throws std::runtime_error on decryption failure.
 */
std::vector<uint8_t> Decrypt(const std::vector<uint8_t>& encryptedData, const mitm::crypto::SecureString& password);

/**
 * @brief Encrypts data using AES-256-GCM and Argon2id derived key.
 * @param plaintext The plaintext data.
 * @param password The password for encryption.
 * @return Encrypted data (salt + nonce + ciphertext).
 * @throws std::runtime_error on encryption failure.
 */
std::vector<uint8_t> Encrypt(const std::vector<uint8_t>& plaintext, const mitm::crypto::SecureString& password);

} // namespace mitm::crypto
