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

namespace mitm::auth {

class AuthManager : public QObject {
    Q_OBJECT
public:
    static AuthManager& instance() {
        static AuthManager inst;
        return inst;
    }

    // Performs the authentication flow:
    // 1. Windows Hello (if available)
    // 2. POST /api/user/v1/session to get the token
    // 3. GET /api/user/v1/roles to populate ConfigManager
    // Returns true if successful.
    bool performLogin(bool isInitialStartup = false);

signals:
    void authSuccess();
    void authFailed(const QString& errorMsg);

private:
    explicit AuthManager(QObject* parent = nullptr) : QObject(parent) {}
    ~AuthManager() override = default;

    bool runWindowsHello();
    bool establishSession(const QString& osUser);
    bool fetchUserRoles();
};

} // namespace mitm::auth
