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

    const char* pinStr = pin && !pin->isEmpty() ? pin->toStdString().c_str() : nullptr;
    int r = fido_dev_make_cred(dev, cred, pinStr);
    if (r != FIDO_OK) {
        spdlog::error("FidoClient: fido_dev_make_cred failed: {} ({})", fido_strerr(r), r);
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

    QByteArray authDataBytes(reinterpret_cast<const char*>(fido_cred_authdata_raw_ptr(cred)), static_cast<int>(fido_cred_authdata_raw_len(cred)));
    result.attestationObjectBase64 = authDataBytes.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);

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

    const char* pinStr = pin && !pin->isEmpty() ? pin->toStdString().c_str() : nullptr;
    int r = fido_dev_get_assert(dev, assert, pinStr);
    if (r != FIDO_OK) {
        spdlog::error("FidoClient: fido_dev_get_assert failed: {} ({})", fido_strerr(r), r);
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
