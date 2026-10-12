/**
 * SPDX-FileComment: FIDO2 / WebAuthn Client using libfido2
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: Antigravity
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file FidoClient.cpp
 * @brief Implementation of cross-platform FIDO2 client wrapper.
 */
#include "FidoClient.h"
#include <fido.h>
#include <cbor.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>
#include <openssl/sha.h>
#include <spdlog/spdlog.h>
#include <QJsonDocument>
#include <QJsonObject>
#include <QCryptographicHash>
#include <memory>

namespace mitm::auth {

FidoClient& FidoClient::instance() {
    static FidoClient inst;
    return inst;
}

FidoClient::FidoClient() {
    fido_init(0);
}

QString FidoClient::friendlyErrorMessage(int code) {
    switch (code) {
        case FIDO_ERR_PIN_REQUIRED:
            return "A PIN is required by this Security Key. Please enter your FIDO2 PIN.";
        case FIDO_ERR_PIN_INVALID:
            return "Invalid PIN entered. Please check your PIN and try again.";
        case FIDO_ERR_PIN_AUTH_BLOCKED:
            return "Security Key PIN is temporarily blocked after multiple failed attempts. Please unplug and re-insert the key.";
        case FIDO_ERR_PIN_BLOCKED:
            return "Security Key PIN is permanently blocked (too many wrong attempts). You must reset the key using YubiKey Manager.";
        case FIDO_ERR_PIN_AUTH_INVALID:
            return "PIN authentication failed. Please re-enter the correct PIN.";
        case FIDO_ERR_ACTION_TIMEOUT:
            return "Operation timed out. Please touch the key prompt in time.";
        case FIDO_ERR_UP_REQUIRED:
            return "User presence required: Please touch the flashing contact on your Security Key.";
        case FIDO_ERR_UV_BLOCKED:
            return "User verification (biometrics) is blocked. Please use your PIN or reset credentials.";
        case FIDO_ERR_CREDENTIAL_EXCLUDED:
            return "This Security Key is already registered to your account.";
        case FIDO_ERR_NO_CREDENTIALS:
            return "No matching WebAuthn credentials found on this Security Key.";
        default:
            return QString("FIDO2 operation failed (%1): %2").arg(code).arg(fido_strerr(code));
    }
}

bool FidoClient::hasDeviceAttached() {
    size_t ndevs = 0;
    fido_dev_info_t* devlist = fido_dev_info_new(64);
    if (!devlist) {
        return false;
    }

    int r = fido_dev_info_manifest(devlist, 64, &ndevs);
    fido_dev_info_free(&devlist, 64);

    return (r == FIDO_OK && ndevs > 0);
}

FidoDeviceInfo FidoClient::getDeviceInfo() {
    FidoDeviceInfo info;
    size_t ndevs = 0;
    fido_dev_info_t* devlist = fido_dev_info_new(64);
    if (!devlist) return info;

    if (fido_dev_info_manifest(devlist, 64, &ndevs) != FIDO_OK || ndevs == 0) {
        fido_dev_info_free(&devlist, 64);
        return info;
    }

    info.hasDevice = true;
    const char* path = fido_dev_info_path(fido_dev_info_ptr(devlist, 0));
    fido_dev_t* dev = fido_dev_new();
    if (dev && fido_dev_open(dev, path) == FIDO_OK) {
        fido_cbor_info_t* ci = fido_cbor_info_new();
        if (ci) {
            if (fido_dev_get_cbor_info(dev, ci) == FIDO_OK) {
                char* const* opt_keys = fido_cbor_info_options_name_ptr(ci);
                const bool* opt_vals = fido_cbor_info_options_value_ptr(ci);
                size_t opt_len = fido_cbor_info_options_len(ci);
                for (size_t i = 0; i < opt_len; ++i) {
                    if (opt_keys[i] && strcmp(opt_keys[i], "clientPin") == 0) {
                        info.pinSet = opt_vals[i];
                    } else if (opt_keys[i] && strcmp(opt_keys[i], "uv") == 0) {
                        info.uvSet = opt_vals[i];
                    }
                }
            }
            fido_cbor_info_free(&ci);
        }

        // Check PIN retries to detect PIN Blocked state
        int retries = 0;
        int r_pin = fido_dev_get_retry_count(dev, &retries);
        if (r_pin == FIDO_ERR_PIN_BLOCKED || retries == 0) {
            info.pinBlocked = true;
        }

        int uv_retries = 0;
        int r_uv = fido_dev_get_uv_retry_count(dev, &uv_retries);
        if (r_uv == FIDO_ERR_UV_BLOCKED || uv_retries == 0) {
            info.uvBlocked = true;
        }

        fido_dev_close(dev);
        fido_dev_free(&dev);
    } else if (dev) {
        fido_dev_free(&dev);
    }

    fido_dev_info_free(&devlist, 64);
    return info;
}

std::optional<QString> FidoClient::extractYubikeySerial(const unsigned char* certDer, size_t certLen) {
    if (!certDer || certLen == 0) {
        return std::nullopt;
    }

    const unsigned char* p = certDer;
    X509* cert = d2i_X509(nullptr, &p, static_cast<long>(certLen));
    if (!cert) {
        return std::nullopt;
    }

    std::optional<QString> serialStr = std::nullopt;
    ASN1_OBJECT* obj = OBJ_txt2obj("1.3.6.1.4.1.41482.3.9", 1);
    if (obj) {
        int idx = X509_get_ext_by_OBJ(cert, obj, -1);
        if (idx >= 0) {
            X509_EXTENSION* ext = X509_get_ext(cert, idx);
            if (ext) {
                ASN1_OCTET_STRING* data = X509_EXTENSION_get_data(ext);
                if (data && data->data) {
                    const unsigned char* dataPtr = data->data;
                    ASN1_INTEGER* serialInt = d2i_ASN1_INTEGER(nullptr, &dataPtr, data->length);
                    if (serialInt) {
                        long serialLong = ASN1_INTEGER_get(serialInt);
                        serialStr = QString::number(serialLong);
                        ASN1_INTEGER_free(serialInt);
                    }
                }
            }
        }
        ASN1_OBJECT_free(obj);
    }

    X509_free(cert);
    return serialStr;
}

std::optional<FidoRegisterResult> FidoClient::registerCredential(
    const QString& rpId,
    const QString& rpName,
    const QString& challengeBase64,
    const QString& username,
    const QString& displayName,
    const std::optional<QString>& pin
) {
    size_t ndevs = 0;
    fido_dev_info_t* devlist = fido_dev_info_new(64);
    if (!devlist) {
        spdlog::error("FidoClient: Failed to allocate dev info");
        return std::nullopt;
    }

    if (fido_dev_info_manifest(devlist, 64, &ndevs) != FIDO_OK || ndevs == 0) {
        spdlog::warn("FidoClient: No FIDO2 devices found");
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    const char* path = fido_dev_info_path(fido_dev_info_ptr(devlist, 0));
    fido_dev_t* dev = fido_dev_new();
    if (!dev || fido_dev_open(dev, path) != FIDO_OK) {
        spdlog::error("FidoClient: Failed to open FIDO2 device at {}", path ? path : "null");
        if (dev) fido_dev_free(&dev);
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    // Prepare ClientDataJSON
    QByteArray rawChallenge = QByteArray::fromBase64(challengeBase64.toUtf8(), QByteArray::Base64UrlEncoding);
    if (rawChallenge.isEmpty()) {
        rawChallenge = QByteArray::fromBase64(challengeBase64.toUtf8());
    }

    QJsonObject clientDataObj;
    clientDataObj["type"] = "webauthn.create";
    clientDataObj["challenge"] = QString::fromUtf8(rawChallenge.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    clientDataObj["origin"] = "http://localhost:8080";
    clientDataObj["crossOrigin"] = false;

    QByteArray clientDataJson = QJsonDocument(clientDataObj).toJson(QJsonDocument::Compact);
    QByteArray clientDataHash = QCryptographicHash::hash(clientDataJson, QCryptographicHash::Sha256);

    fido_cred_t* cred = fido_cred_new();
    if (!cred) {
        fido_dev_close(dev);
        fido_dev_free(&dev);
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    fido_cred_set_type(cred, COSE_ES256);
    fido_cred_set_clientdata_hash(cred, reinterpret_cast<const unsigned char*>(clientDataHash.constData()), clientDataHash.size());
    fido_cred_set_rp(cred, rpId.toStdString().c_str(), rpName.toStdString().c_str());

    QByteArray userId = QCryptographicHash::hash(username.toUtf8(), QCryptographicHash::Sha256);
    fido_cred_set_user(cred, 
        reinterpret_cast<const unsigned char*>(userId.constData()), userId.size(),
        username.toStdString().c_str(),
        displayName.toStdString().c_str(),
        nullptr
    );
    fido_cred_set_rk(cred, FIDO_OPT_FALSE);

    lastErrorCode_ = 0;
    lastErrorMessage_.clear();

    const char* pinStr = pin && !pin->isEmpty() ? pin->toStdString().c_str() : nullptr;
    int r = fido_dev_make_cred(dev, cred, pinStr);
    if (r != FIDO_OK) {
        lastErrorCode_ = r;
        lastErrorMessage_ = friendlyErrorMessage(r);
        spdlog::error("FidoClient: fido_dev_make_cred failed: {} ({}) - {}", fido_strerr(r), r, lastErrorMessage_.toStdString());
        fido_cred_free(&cred);
        fido_dev_close(dev);
        fido_dev_free(&dev);
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    FidoRegisterResult result;
    result.credentialIdBase64 = QByteArray(reinterpret_cast<const char*>(fido_cred_id_ptr(cred)), static_cast<int>(fido_cred_id_len(cred))).toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    result.clientDataJsonBase64 = clientDataJson.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    
    if (fido_cred_x5c_ptr(cred) && fido_cred_x5c_len(cred) > 0) {
        result.yubikeySerial = extractYubikeySerial(fido_cred_x5c_ptr(cred), fido_cred_x5c_len(cred));
    }

    // Build raw attestation object or public key representation
    QByteArray pubKeyBytes(reinterpret_cast<const char*>(fido_cred_pubkey_ptr(cred)), static_cast<int>(fido_cred_pubkey_len(cred)));
    result.publicKeyBase64 = pubKeyBytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    // Build WebAuthn-compliant CBOR attestationObject: { "fmt": string, "authData": bytes, "attStmt": map }
    cbor_item_t* attMap = cbor_new_definite_map(3);
    if (attMap) {
        const char* fmtStr = fido_cred_fmt(cred);
        if (!fmtStr || strlen(fmtStr) == 0) fmtStr = "none";

        struct cbor_pair fmtPair;
        fmtPair.key = cbor_build_string("fmt");
        fmtPair.value = cbor_build_string(fmtStr);
        (void)cbor_map_add(attMap, fmtPair);
        cbor_decref(&fmtPair.key);
        cbor_decref(&fmtPair.value);

        struct cbor_pair authDataPair;
        authDataPair.key = cbor_build_string("authData");
        authDataPair.value = cbor_build_bytestring(
            fido_cred_authdata_raw_ptr(cred),
            fido_cred_authdata_raw_len(cred)
        );
        (void)cbor_map_add(attMap, authDataPair);
        cbor_decref(&authDataPair.key);
        cbor_decref(&authDataPair.value);

        struct cbor_pair attStmtPair;
        attStmtPair.key = cbor_build_string("attStmt");
        if (fido_cred_attstmt_ptr(cred) && fido_cred_attstmt_len(cred) > 0) {
            struct cbor_load_result loadRes;
            attStmtPair.value = cbor_load(fido_cred_attstmt_ptr(cred), fido_cred_attstmt_len(cred), &loadRes);
            if (!attStmtPair.value) {
                attStmtPair.value = cbor_new_definite_map(0);
            }
        } else {
            attStmtPair.value = cbor_new_definite_map(0);
        }
        (void)cbor_map_add(attMap, attStmtPair);
        cbor_decref(&attStmtPair.key);
        cbor_decref(&attStmtPair.value);

        unsigned char* serializedAttObj = nullptr;
        size_t attObjLen = 0;
        size_t written = cbor_serialize_alloc(attMap, &serializedAttObj, &attObjLen);
        if (serializedAttObj && (attObjLen > 0 || written > 0)) {
            size_t finalLen = attObjLen > 0 ? attObjLen : written;
            result.attestationObjectBase64 = QByteArray(reinterpret_cast<const char*>(serializedAttObj), static_cast<int>(finalLen))
                .toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
            free(serializedAttObj);
        }
        cbor_decref(&attMap);
    }

    if (result.attestationObjectBase64.isEmpty()) {
        QByteArray authDataBytes(reinterpret_cast<const char*>(fido_cred_authdata_raw_ptr(cred)), static_cast<int>(fido_cred_authdata_raw_len(cred)));
        result.attestationObjectBase64 = authDataBytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    }

    fido_cred_free(&cred);
    fido_dev_close(dev);
    fido_dev_free(&dev);
    fido_dev_info_free(&devlist, 64);

    return result;
}

std::optional<FidoAssertResult> FidoClient::assertCredential(
    const QString& rpId,
    const QString& challengeBase64,
    const std::vector<QString>& allowedCredentialIdsBase64,
    const std::optional<QString>& pin
) {
    size_t ndevs = 0;
    fido_dev_info_t* devlist = fido_dev_info_new(64);
    if (!devlist) {
        return std::nullopt;
    }

    if (fido_dev_info_manifest(devlist, 64, &ndevs) != FIDO_OK || ndevs == 0) {
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    const char* path = fido_dev_info_path(fido_dev_info_ptr(devlist, 0));
    fido_dev_t* dev = fido_dev_new();
    if (!dev || fido_dev_open(dev, path) != FIDO_OK) {
        if (dev) fido_dev_free(&dev);
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    QByteArray rawChallenge = QByteArray::fromBase64(challengeBase64.toUtf8(), QByteArray::Base64UrlEncoding);
    if (rawChallenge.isEmpty()) {
        rawChallenge = QByteArray::fromBase64(challengeBase64.toUtf8());
    }

    QJsonObject clientDataObj;
    clientDataObj["type"] = "webauthn.get";
    clientDataObj["challenge"] = QString::fromUtf8(rawChallenge.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    clientDataObj["origin"] = "http://localhost:8080";
    clientDataObj["crossOrigin"] = false;

    QByteArray clientDataJson = QJsonDocument(clientDataObj).toJson(QJsonDocument::Compact);
    QByteArray clientDataHash = QCryptographicHash::hash(clientDataJson, QCryptographicHash::Sha256);

    fido_assert_t* assert = fido_assert_new();
    if (!assert) {
        fido_dev_close(dev);
        fido_dev_free(&dev);
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    fido_assert_set_rp(assert, rpId.toStdString().c_str());
    fido_assert_set_clientdata_hash(assert, reinterpret_cast<const unsigned char*>(clientDataHash.constData()), clientDataHash.size());

    for (const auto& credIdB64 : allowedCredentialIdsBase64) {
        QByteArray credId = QByteArray::fromBase64(credIdB64.toUtf8(), QByteArray::Base64UrlEncoding);
        if (credId.isEmpty()) {
            credId = QByteArray::fromBase64(credIdB64.toUtf8());
        }
        if (!credId.isEmpty()) {
            fido_assert_allow_cred(assert, reinterpret_cast<const unsigned char*>(credId.constData()), credId.size());
        }
    }

    lastErrorCode_ = 0;
    lastErrorMessage_.clear();

    const char* pinStr = pin && !pin->isEmpty() ? pin->toStdString().c_str() : nullptr;
    int r = fido_dev_get_assert(dev, assert, pinStr);
    if (r != FIDO_OK) {
        lastErrorCode_ = r;
        lastErrorMessage_ = friendlyErrorMessage(r);
        spdlog::error("FidoClient: fido_dev_get_assert failed: {} ({}) - {}", fido_strerr(r), r, lastErrorMessage_.toStdString());
        fido_assert_free(&assert);
        fido_dev_close(dev);
        fido_dev_free(&dev);
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    FidoAssertResult result;
    result.credentialIdBase64 = QByteArray(reinterpret_cast<const char*>(fido_assert_id_ptr(assert, 0)), static_cast<int>(fido_assert_id_len(assert, 0))).toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    result.clientDataJsonBase64 = clientDataJson.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    QByteArray authData(reinterpret_cast<const char*>(fido_assert_authdata_raw_ptr(assert, 0)), static_cast<int>(fido_assert_authdata_raw_len(assert, 0)));
    result.authenticatorDataBase64 = authData.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    QByteArray sig(reinterpret_cast<const char*>(fido_assert_sig_ptr(assert, 0)), static_cast<int>(fido_assert_sig_len(assert, 0)));
    result.signatureBase64 = sig.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

    if (fido_assert_user_id_ptr(assert, 0) && fido_assert_user_id_len(assert, 0) > 0) {
        QByteArray uid(reinterpret_cast<const char*>(fido_assert_user_id_ptr(assert, 0)), static_cast<int>(fido_assert_user_id_len(assert, 0)));
        result.userHandleBase64 = uid.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
    }

    fido_assert_free(&assert);
    fido_dev_close(dev);
    fido_dev_free(&dev);
    fido_dev_info_free(&devlist, 64);

    return result;
}

} // namespace mitm::auth
