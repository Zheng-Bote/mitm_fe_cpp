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
#include <spdlog/spdlog.h>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QNetworkInterface>
#include <QAbstractSocket>
#include <QMessageBox>
#include <QTimer>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <winrt/Windows.Security.Credentials.UI.h>
#include <winrt/Windows.Foundation.h>
#include <UserConsentVerifierInterop.h>
#pragma comment(lib, "windowsapp.lib")
#endif

namespace mitm::auth {

bool AuthManager::performLogin(bool isInitialStartup) {
    if (!runWindowsHello()) {
        if (!isInitialStartup) {
            QMessageBox::critical(nullptr, "Re-Authentication Failed", "Windows Hello authentication failed or was cancelled.");
        }
        return false;
    }

    QString osUser = QProcessEnvironment::systemEnvironment().value("USER", QProcessEnvironment::systemEnvironment().value("USERNAME", "unknown"));
    
    if (!establishSession(osUser)) {
        if (!isInitialStartup) {
            QMessageBox::critical(nullptr, "Session Error", "Could not establish a session with the backend.");
        }
        return false;
    }

    if (!fetchUserRoles()) {
        if (!isInitialStartup) {
            QMessageBox::warning(nullptr, "Role Retrieval", "Session established, but failed to retrieve user roles.");
        }
        return false;
    }

    startSessionRenewal();
    emit authSuccess();
    return true;
}

bool AuthManager::runWindowsHello() {
#ifdef _WIN32
    spdlog::info("Requesting Windows Hello authentication via WinRT...");
    bool authSuccess = false;
    bool authError = false;
    std::string errorMsg;

    std::thread([&]() {
        try {
            winrt::init_apartment(winrt::apartment_type::multi_threaded);
            
            auto availability = winrt::Windows::Security::Credentials::UI::UserConsentVerifier::CheckAvailabilityAsync().get();
            if (availability == winrt::Windows::Security::Credentials::UI::UserConsentVerifierAvailability::Available) {
                
                auto factory = winrt::get_activation_factory<winrt::Windows::Security::Credentials::UI::UserConsentVerifier>();
                auto interop = factory.as<IUserConsentVerifierInterop>();
                
                winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Security::Credentials::UI::UserConsentVerificationResult> asyncOp{ nullptr };
                winrt::hstring message = L"Please authenticate (Windows Hello) to access the MitM Admin Frontend.";
                
                HWND hwnd = GetActiveWindow();
                if (!hwnd) hwnd = GetForegroundWindow();
                if (!hwnd) hwnd = GetDesktopWindow();

                winrt::check_hresult(interop->RequestVerificationForWindowAsync(
                    hwnd, 
                    (HSTRING)winrt::get_abi(message), 
                    winrt::guid_of<winrt::Windows::Foundation::IAsyncOperation<winrt::Windows::Security::Credentials::UI::UserConsentVerificationResult>>(), 
                    winrt::put_abi(asyncOp)
                ));

                auto result = asyncOp.get();

                if (result == winrt::Windows::Security::Credentials::UI::UserConsentVerificationResult::Verified) {
                    authSuccess = true;
                }
            } else {
                authSuccess = true; 
                spdlog::warn("Windows Hello is not available. Bypassing...");
            }
        } catch (const winrt::hresult_error& e) {
            authError = true;
            errorMsg = winrt::to_string(e.message());
        } catch (const std::exception& e) {
            authError = true;
            errorMsg = e.what();
        }
    }).join();

    if (authError) {
        spdlog::error("Windows Hello Error: {}", errorMsg);
        return false;
    }

    return authSuccess;
#else
    return true; // Non-Windows platforms bypass this for now
#endif
}

bool AuthManager::establishSession(const QString& osUser) {
    QNetworkAccessManager manager;
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    
    QNetworkRequest req(QUrl(host + "/api/v1/auth/session"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json");
    req.setTransferTimeout(10000);

    QJsonObject payload;
    payload["os_user"] = osUser;
    
    // Resolve local IP address (best effort)
    QString localIp = "127.0.0.1";
    for (const auto& address : QNetworkInterface::allAddresses()) {
        if (!address.isLoopback() && (address.protocol() == QAbstractSocket::IPv4Protocol || address.protocol() == QAbstractSocket::IPv6Protocol)) {
            localIp = address.toString();
            break; // take first non-loopback
        }
    }
    payload["client_ip"] = localIp;

    QNetworkReply* reply = manager.post(req, QJsonDocument(payload).toJson());
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool success = false;
    if (reply->error() == QNetworkReply::NoError) {
        auto doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject()) {
            QString sessionToken = doc.object().value("session_token").toString();
            if (!sessionToken.isEmpty()) {
                std::string rawStr = sessionToken.toStdString();
                mitm::crypto::SecureString secureToken(rawStr.begin(), rawStr.end());
                mitm::config::ConfigManager::GetInstance().SetSessionToken(secureToken);
                success = true;
            }
        }
    } else {
        int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
        QString errorMsg = reply->errorString();
        
        QByteArray responseBody = reply->readAll();
        auto doc = QJsonDocument::fromJson(responseBody);
        if (doc.isObject() && doc.object().contains("errors")) {
            auto errors = doc.object().value("errors").toArray();
            if (!errors.isEmpty()) {
                QString detail = errors[0].toObject().value("detail").toString();
                if (!detail.isEmpty()) {
                    errorMsg = detail;
                }
            }
        }

        spdlog::error("Failed to establish session: {} - {}", statusCode, errorMsg.toStdString());
        emit authFailed("Login rejected: " + errorMsg);
        
        if (errorMsg.contains("inactive", Qt::CaseInsensitive)) {
            QMessageBox::critical(nullptr, "Login Rejected", "User account is inactive.");
        }
    }
    reply->deleteLater();
    return success;
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
        if (performLogin(false)) {
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
