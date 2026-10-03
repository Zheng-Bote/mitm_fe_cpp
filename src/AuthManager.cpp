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
#include <QMessageBox>
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
    
    QNetworkRequest req(QUrl(host + "/api/user/v1/session"));
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    req.setRawHeader("Accept", "application/json");
    req.setTransferTimeout(10000);

    // Construct the payload. Depending on the backend spec, we send the OS user or we just POST.
    QJsonObject payload;
    payload["os_user"] = osUser;
    
    // We send the admin token (if configured) as part of the payload or basic auth just to authenticate the session request
    // Since we removed GetAuthHeader(), we can just send the token from config if needed.
    // Wait, the API spec says "POST /api/user/v1/session (Creates a new session for the OS user.)"
    // Let's assume we pass the configured admin token here to prove identity.
    QString configuredToken = "";
    auto& config = mitm::config::ConfigManager::GetInstance().GetConfig();
    for (const auto& admin : config.admin_users) {
        if (admin.username == osUser.toStdString() && !admin.token.empty()) {
            configuredToken = QString::fromStdString(admin.token);
            break;
        }
    }
    payload["token"] = configuredToken;

    QNetworkReply* reply = manager.post(req, QJsonDocument(payload).toJson());
    QEventLoop loop;
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    bool success = false;
    if (reply->error() == QNetworkReply::NoError) {
        // Parse session token from response
        auto doc = QJsonDocument::fromJson(reply->readAll());
        if (doc.isObject()) {
            QString sessionToken = doc.object().value("session_token").toString();
            if (!sessionToken.isEmpty()) {
                std::string rawStr = sessionToken.toStdString();
                mitm::crypto::SecureString secureToken(rawStr.begin(), rawStr.end());
                mitm::config::ConfigManager::GetInstance().SetSessionToken(secureToken);
                success = true;
            }
        } else {
            // Fallback if backend returns plain token or sets a Cookie. 
            // If it sets a Cookie, we should extract it from headers or let QNetworkCookieJar handle it.
            // Assuming the token is in the JSON payload as per typical REST APIs:
        }
    } else {
        spdlog::error("Failed to establish session: {} - {}", reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt(), reply->errorString().toStdString());
    }
    reply->deleteLater();
    return success;
}

bool AuthManager::fetchUserRoles() {
    QNetworkAccessManager manager;
    QString host = mitm::config::ConfigManager::GetInstance().GetHostUrl();
    
    QNetworkRequest req(QUrl(host + "/api/user/v1/roles"));
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

} // namespace mitm::auth
