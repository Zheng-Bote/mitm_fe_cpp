/**
 * SPDX-FileComment: FIDO2 / WebAuthn Client using libfido2
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: Antigravity
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file FidoClient.h
 * @brief Cross-platform FIDO2 client wrapper using libfido2 and OpenSSL.
 */
#pragma once

#include <QString>
#include <QByteArray>
#include <QJsonObject>
#include <optional>
#include <string>
#include <vector>

namespace mitm::auth {

struct FidoRegisterResult {
    QString credentialIdBase64;
    QString clientDataJsonBase64;
    QString attestationObjectBase64;
    QString publicKeyBase64;
    std::optional<QString> yubikeySerial;
    QString attachmentType{"cross-platform"};
};

struct FidoAssertResult {
    QString credentialIdBase64;
    QString clientDataJsonBase64;
    QString authenticatorDataBase64;
    QString signatureBase64;
    std::optional<QString> userHandleBase64;
};

// Authenticator requested by the user. On Windows, non-elevated processes cannot talk to
// FIDO HID devices directly, so everything except elevated SecurityKey goes through
// webauthn.dll ("windows://hello"), which offers security keys and platform authenticators.
enum class AuthenticatorKind {
    Any,
    SecurityKey,
    Platform
};

struct FidoDeviceInfo {
    bool hasDevice{false};
    bool isWinHello{false};
    bool pinSet{false};
    bool pinBlocked{false};
    bool uvSet{false};
    bool uvBlocked{false};
};

class FidoClient {
public:
    static FidoClient& instance();

    // Check if at least one FIDO2 token/authenticator is physically attached
    bool hasDeviceAttached();

    // Inspect first attached device properties (PIN set, PIN blocked, etc.)
    FidoDeviceInfo getDeviceInfo(AuthenticatorKind kind = AuthenticatorKind::Any);

    // Perform FIDO2 Registration (Make Credential)
    std::optional<FidoRegisterResult> registerCredential(
        const QString& rpId,
        const QString& rpName,
        const QString& challengeBase64,
        const QString& username,
        const QString& displayName,
        const std::optional<QString>& pin = std::nullopt,
        AuthenticatorKind kind = AuthenticatorKind::Any,
        const QString& origin = QString()  // clientDataJSON.origin; empty = scheduler host URL
    );

    // Perform FIDO2 Authentication (Get Assertion)
    std::optional<FidoAssertResult> assertCredential(
        const QString& rpId,
        const QString& challengeBase64,
        const std::vector<QString>& allowedCredentialIdsBase64,
        const std::optional<QString>& pin = std::nullopt,
        AuthenticatorKind kind = AuthenticatorKind::Any,
        const QString& origin = QString()  // clientDataJSON.origin; empty = scheduler host URL
    );

    // Extract YubiKey Serial Number from X.509 cert extension (OID 1.3.6.1.4.1.41482.3.9)
    static std::optional<QString> extractYubikeySerial(const unsigned char* certDer, size_t certLen);

    // Error Reporting
    int lastErrorCode() const { return lastErrorCode_; }
    QString lastErrorMessage() const { return lastErrorMessage_; }
    static QString friendlyErrorMessage(int fidoErrCode);

private:
    FidoClient();
    ~FidoClient() = default;

    int lastErrorCode_{0};
    QString lastErrorMessage_{};
};

} // namespace mitm::auth
