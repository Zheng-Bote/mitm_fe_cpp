/**
 * SPDX-FileComment: AuthManager
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 */
#include "AuthManager.h"
#include "Config.h"
#include "ApiClient.h"
#include "LoginDialog.h"
#include <spdlog/spdlog.h>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>


namespace mitm::auth {

bool AuthManager::performLogin() {
    // Sessions are only issued by the WebAuthn passkey ceremony in LoginDialog, which
    // completes the login via completePasskeyLogin() before accepting.
    mitm::ui::LoginDialog dialog;
    return dialog.exec() == QDialog::Accepted;
}

bool AuthManager::completePasskeyLogin() {
    if (!fetchUserRoles()) {
        return false;
    }

    startSessionRenewal();
    emit authSuccess();
    return true;
}

bool AuthManager::fetchUserRoles() {
    QNetworkAccessManager manager;
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    
    QNetworkRequest req(QUrl(host + "/api/v1/auth/me"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json");
    req.setTransferTimeout(10000);
    
    mitm::crypto::SecureString secureToken = mitm::config::ConfigManager::GetInstance().GetSessionToken();
    std::string token(secureToken.begin(), secureToken.end());
    if (!token.empty()) {
        req.setRawHeader("Authorization", ("Bearer " + QString::fromStdString(token)).toUtf8());
    }

    QNetworkReply* reply = manager.get(req);
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool success = false;
    if (reply->error() == QNetworkReply::NoError) {
        auto doc = QJsonDocument::fromJson(reply->readAll());
        std::vector<std::string> userRoles;
        if (doc.isArray()) {
            for (const auto& v : doc.array()) {
                userRoles.push_back(v.toString().toStdString());
            }
            success = true;
        } else if (doc.isObject() && doc.object().contains("roles")) {
            for (const auto& v : doc.object().value("roles").toArray()) {
                userRoles.push_back(v.toString().toStdString());
            }
            success = true;
        }
        mitm::config::ConfigManager::GetInstance().SetCurrentUserRoles(userRoles);
    } else {
        spdlog::error("Failed to fetch user roles: {} - {}", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), reply->errorString().toStdString());
    }
    reply->deleteLater();
    return success;
}

void AuthManager::startSessionRenewal() {
    if (!renewalTimer) {
        renewalTimer = new QTimer(this);
        connect(renewalTimer, &QTimer::timeout, this, &AuthManager::onRenewSession);
    }
    // Ping every 30 minutes (1800000 ms) to keep the 2-hour idle timeout alive
    renewalTimer->start(1800000);
}

void AuthManager::stopSessionRenewal() {
    if (renewalTimer) {
        renewalTimer->stop();
    }
}

void AuthManager::onRenewSession() {
    spdlog::info("Attempting session renewal ping...");
    // A simple GET to /me extends the idle timeout if it succeeds
    if (!fetchUserRoles()) {
        spdlog::warn("Session renewal ping failed. Attempting full re-authentication.");
        if (performLogin()) {
            spdlog::info("Session re-authentication successful.");
        } else {
            spdlog::error("Session re-authentication failed. Session is lost.");
            stopSessionRenewal();
            // Could emit a signal here to log out the user entirely
        }
    } else {
        spdlog::info("Session successfully renewed/kept alive.");
    }
}

} // namespace mitm::auth
