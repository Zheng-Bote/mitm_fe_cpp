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
#include <openssl/crypto.h>
#include <memory>
#include "Config.h"

#ifdef _WIN32
#include <windows.h>
#include <webauthn.h>
#endif

namespace mitm::auth {

namespace {

// libfido2 waits forever on HID reads by default; bound every transaction.
constexpr int kDeviceInfoTimeoutMs = 5000;
constexpr int kCeremonyTimeoutMs = 60000;

bool isWinHelloPath(const char* path) {
    return path && strncmp(path, "windows://hello", 15) == 0;
}

bool isProcessElevated() {
#ifdef _WIN32
    HANDLE token = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token)) {
        return false;
    }
    TOKEN_ELEVATION elevation{};
    DWORD size = 0;
    bool elevated = GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size)
                    && elevation.TokenIsElevated;
    CloseHandle(token);
    return elevated;
#else
    return true;
#endif
}

// Windows (1903+) blocks raw FIDO HID access for non-elevated processes: the handle opens,
// but reads never complete. Such processes must use webauthn.dll via "windows://hello".
const char* selectDevicePath(const fido_dev_info_t* devlist, size_t ndevs, AuthenticatorKind kind) {
    const char* winHello = nullptr;
    const char* hid = nullptr;
    for (size_t i = 0; i < ndevs; ++i) {
        const char* p = fido_dev_info_path(fido_dev_info_ptr(devlist, i));
        if (isWinHelloPath(p)) {
            if (!winHello) winHello = p;
        } else if (p && !hid) {
            hid = p;
        }
    }

    if (kind == AuthenticatorKind::Platform) {
        return winHello;
    }
    if (winHello && !isProcessElevated()) {
        return winHello;
    }
    return hid ? hid : winHello;
}

#ifdef _WIN32
// libfido2's winhello backend never sets dwAuthenticatorAttachment, so Windows picks the
// platform authenticator (Windows Hello) even when the user chose a security key. Call
// webauthn.dll directly with the requested attachment and hand the attestation object back
// to libfido2, so the caller reads the result exactly as after fido_dev_make_cred().
int winHelloMakeCredential(fido_cred_t* cred, const QString& rpId, const QString& rpName,
                           const QByteArray& userId, const QString& username,
                           const QString& displayName, const QByteArray& clientDataJson,
                           AuthenticatorKind kind, QString& errorMessage) {
    const std::wstring wRpId = rpId.toStdWString();
    const std::wstring wRpName = rpName.toStdWString();
    const std::wstring wUserName = username.toStdWString();
    const std::wstring wDisplayName = displayName.toStdWString();

    WEBAUTHN_RP_ENTITY_INFORMATION rp{};
    rp.dwVersion = WEBAUTHN_RP_ENTITY_INFORMATION_CURRENT_VERSION;
    rp.pwszId = wRpId.c_str();
    rp.pwszName = wRpName.c_str();

    WEBAUTHN_USER_ENTITY_INFORMATION user{};
    user.dwVersion = WEBAUTHN_USER_ENTITY_INFORMATION_CURRENT_VERSION;
    user.cbId = static_cast<DWORD>(userId.size());
    user.pbId = reinterpret_cast<PBYTE>(const_cast<char*>(userId.constData()));
    user.pwszName = wUserName.c_str();
    user.pwszDisplayName = wDisplayName.c_str();

    WEBAUTHN_COSE_CREDENTIAL_PARAMETER coseParam{};
    coseParam.dwVersion = WEBAUTHN_COSE_CREDENTIAL_PARAMETER_CURRENT_VERSION;
    coseParam.pwszCredentialType = WEBAUTHN_CREDENTIAL_TYPE_PUBLIC_KEY;
    coseParam.lAlg = WEBAUTHN_COSE_ALGORITHM_ECDSA_P256_WITH_SHA256;
    WEBAUTHN_COSE_CREDENTIAL_PARAMETERS coseParams{1, &coseParam};

    WEBAUTHN_CLIENT_DATA clientData{};
    clientData.dwVersion = WEBAUTHN_CLIENT_DATA_CURRENT_VERSION;
    clientData.cbClientDataJSON = static_cast<DWORD>(clientDataJson.size());
    clientData.pbClientDataJSON = reinterpret_cast<PBYTE>(const_cast<char*>(clientDataJson.constData()));
    clientData.pwszHashAlgId = WEBAUTHN_HASH_ALGORITHM_SHA_256;

    // Version 1 carries every field we need and is supported by all webauthn.dll releases.
    WEBAUTHN_AUTHENTICATOR_MAKE_CREDENTIAL_OPTIONS options{};
    options.dwVersion = WEBAUTHN_AUTHENTICATOR_MAKE_CREDENTIAL_OPTIONS_VERSION_1;
    options.dwTimeoutMilliseconds = kCeremonyTimeoutMs;
    options.dwAuthenticatorAttachment =
        kind == AuthenticatorKind::Platform    ? WEBAUTHN_AUTHENTICATOR_ATTACHMENT_PLATFORM
        : kind == AuthenticatorKind::SecurityKey ? WEBAUTHN_AUTHENTICATOR_ATTACHMENT_CROSS_PLATFORM
                                                 : WEBAUTHN_AUTHENTICATOR_ATTACHMENT_ANY;
    // The backend registers passkeys, which require user verification.
    options.dwUserVerificationRequirement = WEBAUTHN_USER_VERIFICATION_REQUIREMENT_REQUIRED;
    // Direct attestation carries the YubiKey certificate (serial number extraction).
    options.dwAttestationConveyancePreference = WEBAUTHN_ATTESTATION_CONVEYANCE_PREFERENCE_DIRECT;

    HWND hwnd = GetForegroundWindow();
    if (!hwnd) hwnd = GetTopWindow(nullptr);

    PWEBAUTHN_CREDENTIAL_ATTESTATION attestation = nullptr;
    HRESULT hr = WebAuthNAuthenticatorMakeCredential(hwnd, &rp, &user, &coseParams, &clientData,
                                                     &options, &attestation);
    if (FAILED(hr)) {
        errorMessage = hr == NTE_USER_CANCELLED
            ? QString("The operation was cancelled or timed out.")
            : QString("Windows WebAuthn error: %1").arg(QString::fromWCharArray(WebAuthNGetErrorName(hr)));
        spdlog::error("FidoClient: WebAuthNAuthenticatorMakeCredential failed: 0x{:08x}", static_cast<unsigned long>(hr));
        return FIDO_ERR_INTERNAL;
    }

    int r = fido_cred_set_attobj(cred, attestation->pbAttestationObject, attestation->cbAttestationObject);
    WebAuthNFreeCredentialAttestation(attestation);
    if (r != FIDO_OK) {
        errorMessage = "Could not decode the attestation returned by Windows.";
    }
    return r;
}
#endif

} // namespace

FidoClient& FidoClient::instance() {
    static FidoClient inst;
    return inst;
}

FidoClient::FidoClient() {
    fido_init(FIDO_DEBUG);
    fido_set_log_handler([](const char* msg) {
        if (msg) {
            spdlog::debug("[libfido2] {}", msg);
        }
    });
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

FidoDeviceInfo FidoClient::getDeviceInfo(AuthenticatorKind kind) {
    FidoDeviceInfo info;
    size_t ndevs = 0;
    fido_dev_info_t* devlist = fido_dev_info_new(64);
    if (!devlist) return info;

    if (fido_dev_info_manifest(devlist, 64, &ndevs) != FIDO_OK || ndevs == 0) {
        fido_dev_info_free(&devlist, 64);
        return info;
    }

    const char* path = selectDevicePath(devlist, ndevs, kind);
    if (!path) {
        fido_dev_info_free(&devlist, 64);
        return info;
    }
    info.hasDevice = true;

    if (isWinHelloPath(path)) {
        // webauthn.dll handles PIN / biometrics itself; there is nothing to query.
        info.isWinHello = true;
        fido_dev_info_free(&devlist, 64);
        return info;
    }

    fido_dev_t* dev = fido_dev_new();
    if (dev) {
        fido_dev_set_timeout(dev, kDeviceInfoTimeoutMs);
    }
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
        int retries = -1;
        int r_pin = fido_dev_get_retry_count(dev, &retries);
        if (r_pin == FIDO_ERR_PIN_BLOCKED || (r_pin == FIDO_OK && retries == 0)) {
            info.pinBlocked = true;
        }

        int uv_retries = -1;
        int r_uv = fido_dev_get_uv_retry_count(dev, &uv_retries);
        if (r_uv == FIDO_ERR_UV_BLOCKED || (r_uv == FIDO_OK && uv_retries == 0)) {
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
    const std::optional<QString>& pin,
    AuthenticatorKind kind,
    const QString& origin
) {
    lastErrorCode_ = 0;
    lastErrorMessage_.clear();

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

    const char* path = selectDevicePath(devlist, ndevs, kind);
    if (!path) {
        lastErrorMessage_ = "The requested authenticator (Windows Hello / platform) is not available on this system.";
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    fido_dev_t* dev = fido_dev_new();
    if (dev) {
        fido_dev_set_timeout(dev, kCeremonyTimeoutMs);
    }
    if (!dev || fido_dev_open(dev, path) != FIDO_OK) {
        spdlog::error("FidoClient: Failed to open FIDO2 device at {}", path);
        lastErrorMessage_ = "Could not open the FIDO2 authenticator.";
        if (dev) fido_dev_free(&dev);
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    // Prepare ClientDataJSON
    QByteArray rawChallenge = QByteArray::fromBase64(challengeBase64.toUtf8(), QByteArray::Base64UrlEncoding);
    if (rawChallenge.isEmpty()) {
        rawChallenge = QByteArray::fromBase64(challengeBase64.toUtf8());
    }

    // The backend validates this against its configured WebAuthn origin, which it returns
    // in the ceremony start response; the scheduler host URL is only a fallback.
    QString host = origin.isEmpty() ? mitm::config::ConfigManager::GetInstance().GetHostUrl() : origin;

    QJsonObject clientDataObj;
    clientDataObj["type"] = "webauthn.create";
    clientDataObj["challenge"] = QString::fromUtf8(rawChallenge.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    clientDataObj["origin"] = host.isEmpty() ? "http://localhost:8080" : host;
    clientDataObj["crossOrigin"] = false;

    QByteArray clientDataJson = QJsonDocument(clientDataObj).toJson(QJsonDocument::Compact);

    fido_cred_t* cred = fido_cred_new();
    if (!cred) {
        fido_dev_close(dev);
        fido_dev_free(&dev);
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    fido_cred_set_type(cred, COSE_ES256);
    fido_cred_set_clientdata(cred, reinterpret_cast<const unsigned char*>(clientDataJson.constData()), clientDataJson.size());
    fido_cred_set_rp(cred, rpId.toStdString().c_str(), rpName.toStdString().c_str());

    QByteArray userId = QCryptographicHash::hash(username.toUtf8(), QCryptographicHash::Sha256);
    fido_cred_set_user(cred, 
        reinterpret_cast<const unsigned char*>(userId.constData()), userId.size(),
        username.toStdString().c_str(),
        displayName.toStdString().c_str(),
        nullptr
    );
    fido_cred_set_rk(cred, FIDO_OPT_OMIT);

    int r = FIDO_ERR_INTERNAL;
    QString winHelloError;
    if (fido_dev_is_winhello(dev)) {
#ifdef _WIN32
        r = winHelloMakeCredential(cred, rpId, rpName, userId, username, displayName,
                                   clientDataJson, kind, winHelloError);
#endif
    } else {
        std::string pinUtf8;
        if (pin && !pin->isEmpty()) {
            pinUtf8 = pin->toStdString();
        }
        r = fido_dev_make_cred(dev, cred, pinUtf8.empty() ? nullptr : pinUtf8.c_str());
        OPENSSL_cleanse(pinUtf8.data(), pinUtf8.size());
    }
    if (r != FIDO_OK) {
        lastErrorCode_ = r;
        lastErrorMessage_ = winHelloError.isEmpty() ? friendlyErrorMessage(r) : winHelloError;
        spdlog::error("FidoClient: fido_dev_make_cred failed: {} ({}) - {}", fido_strerr(r), r, lastErrorMessage_.toStdString());
        fido_cred_free(&cred);
        fido_dev_close(dev);
        fido_dev_free(&dev);
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    FidoRegisterResult result;
    // webauthn.dll also serves USB security keys, so the device path does not tell us the attachment.
    if (kind == AuthenticatorKind::Platform) {
        result.attachmentType = "platform";
    }
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
    const std::optional<QString>& pin,
    AuthenticatorKind kind,
    const QString& origin
) {
    lastErrorCode_ = 0;
    lastErrorMessage_.clear();

    size_t ndevs = 0;
    fido_dev_info_t* devlist = fido_dev_info_new(64);
    if (!devlist) {
        return std::nullopt;
    }

    if (fido_dev_info_manifest(devlist, 64, &ndevs) != FIDO_OK || ndevs == 0) {
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    const char* path = selectDevicePath(devlist, ndevs, kind);
    if (!path) {
        lastErrorMessage_ = "The requested authenticator (Windows Hello / platform) is not available on this system.";
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    fido_dev_t* dev = fido_dev_new();
    if (dev) {
        fido_dev_set_timeout(dev, kCeremonyTimeoutMs);
    }
    if (!dev || fido_dev_open(dev, path) != FIDO_OK) {
        spdlog::error("FidoClient: Failed to open FIDO2 device at {}", path);
        lastErrorMessage_ = "Could not open the FIDO2 authenticator.";
        if (dev) fido_dev_free(&dev);
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    QByteArray rawChallenge = QByteArray::fromBase64(challengeBase64.toUtf8(), QByteArray::Base64UrlEncoding);
    if (rawChallenge.isEmpty()) {
        rawChallenge = QByteArray::fromBase64(challengeBase64.toUtf8());
    }

    // The backend validates this against its configured WebAuthn origin, which it returns
    // in the ceremony start response; the scheduler host URL is only a fallback.
    QString host = origin.isEmpty() ? mitm::config::ConfigManager::GetInstance().GetHostUrl() : origin;

    QJsonObject clientDataObj;
    clientDataObj["type"] = "webauthn.get";
    clientDataObj["challenge"] = QString::fromUtf8(rawChallenge.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals));
    clientDataObj["origin"] = host.isEmpty() ? "http://localhost:8080" : host;
    clientDataObj["crossOrigin"] = false;

    QByteArray clientDataJson = QJsonDocument(clientDataObj).toJson(QJsonDocument::Compact);

    fido_assert_t* assert = fido_assert_new();
    if (!assert) {
        fido_dev_close(dev);
        fido_dev_free(&dev);
        fido_dev_info_free(&devlist, 64);
        return std::nullopt;
    }

    fido_assert_set_rp(assert, rpId.toStdString().c_str());
    fido_assert_set_clientdata(assert, reinterpret_cast<const unsigned char*>(clientDataJson.constData()), clientDataJson.size());

    for (const auto& credIdB64 : allowedCredentialIdsBase64) {
        QByteArray credId = QByteArray::fromBase64(credIdB64.toUtf8(), QByteArray::Base64UrlEncoding);
        if (credId.isEmpty()) {
            credId = QByteArray::fromBase64(credIdB64.toUtf8());
        }
        if (!credId.isEmpty()) {
            fido_assert_allow_cred(assert, reinterpret_cast<const unsigned char*>(credId.constData()), credId.size());
        }
    }

    std::string pinUtf8;
    if (fido_dev_is_winhello(dev)) {
        // Passkey login requires user verification (see registerCredential).
        fido_assert_set_uv(assert, FIDO_OPT_TRUE);
    } else if (pin && !pin->isEmpty()) {
        pinUtf8 = pin->toStdString();
    }
    int r = fido_dev_get_assert(dev, assert, pinUtf8.empty() ? nullptr : pinUtf8.c_str());
    OPENSSL_cleanse(pinUtf8.data(), pinUtf8.size());
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
