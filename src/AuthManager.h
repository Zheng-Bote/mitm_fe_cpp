/**
 * SPDX-FileComment: AuthManager
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

namespace mitm::auth {

class AuthManager : public QObject {
    Q_OBJECT
public:
    static AuthManager& instance() {
        static AuthManager inst;
        return inst;
    }

    // (Re-)authenticates via the WebAuthn passkey ceremony in LoginDialog.
    // Returns true if the user signed in successfully.
    bool performLogin();

    // Finishes a login whose session token was already issued by the WebAuthn ceremony:
    // fetches roles and starts session renewal.
    bool completePasskeyLogin();

signals:
    void authSuccess();
    void authFailed(const QString& errorMsg);

private slots:
    void onRenewSession();

private:
    void startSessionRenewal();
    void stopSessionRenewal();
    explicit AuthManager(QObject* parent = nullptr) : QObject(parent) {}
    ~AuthManager() override = default;

    bool fetchUserRoles();
    QTimer* renewalTimer{nullptr};
};

} // namespace mitm::auth
