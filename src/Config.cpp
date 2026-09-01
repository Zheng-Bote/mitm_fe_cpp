#include <QRegularExpression>
/**
 * SPDX-FileComment: Config
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file Config.cpp
 * @brief Config
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#include "Config.h"
#include "Crypto.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <QProcessEnvironment>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>

namespace mitm::config {

void ConfigManager::LoadEncryptedConfig(const std::string& filepath, const mitm::crypto::SecureString& password) {
    m_password = password;
    m_globalConfigPath = filepath;

    std::ifstream file(filepath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open config file: " + filepath);
    }

    // Read file contents
    std::vector<uint8_t> encryptedData((std::istreambuf_iterator<char>(file)),
                                        std::istreambuf_iterator<char>());
    file.close();

    // Decrypt
    std::vector<uint8_t> plaintext = crypto::Decrypt(encryptedData, password);
    std::string jsonStr(plaintext.begin(), plaintext.end());

    // Parse JSON
    auto j = nlohmann::json::parse(jsonStr);

    m_config.scheduler_host = j.value("scheduler_host", "localhost");
    m_config.scheduler_port = j.value("scheduler_port", 8080);
    m_config.scheduler_use_https = j.value("scheduler_use_https", false);
    m_config.log_level = j.value("log_level", "info");
    m_config.name = j.value("name", "Default Config");

    if (j.contains("networking") && j["networking"].contains("proxy")) {
        auto p = j["networking"]["proxy"];
        m_config.proxy.proxy_host = p.value("proxy_host", "");
        m_config.proxy.proxy_port = p.value("proxy_port", 8080);
    }
    
    // Support either "admins" or "admin_users" as array
    auto adminsKey = j.contains("admins") ? "admins" : (j.contains("admin_users") ? "admin_users" : "");
    if (!std::string(adminsKey).empty() && j[adminsKey].is_array()) {
        for (const auto& user : j[adminsKey]) {
            AdminUser au;
            if (user.is_object()) {
                au.username = user.value("Username", user.value("username", ""));
                au.token = user.value("AuthToken", user.value("token", ""));
                if (!au.username.empty()) m_config.admin_users.push_back(au);
            } else if (user.is_string()) {
                au.username = user.get<std::string>();
                m_config.admin_users.push_back(au);
            }
        }
    }

    LoadUserConfig();
}

bool ConfigManager::IsAdmin(const std::string& username) const {
    for (const auto& admin : m_config.admin_users) {
        if (admin.username == username) return true;
    }
    return false;
}

QString ConfigManager::GetAuthHeader() const {
    auto env = QProcessEnvironment::systemEnvironment();
    QString osUser = env.value("USER", env.value("USERNAME", "unknown"));
    QString token = "";
    
    for (const auto& admin : m_config.admin_users) {
        if (admin.username == osUser.toStdString() && !admin.token.empty()) {
            token = QString::fromStdString(admin.token);
            break;
        }
    }
    
    QString credentials = osUser + ":" + token;
    return "Basic " + credentials.toLocal8Bit().toBase64();
}

QString ConfigManager::GetHostUrl() const {
    QString protocol = m_config.scheduler_use_https ? "https://" : "http://";
    QString host = QString::fromStdString(m_config.scheduler_host);
    QString portPart = "";
    
    if ((m_config.scheduler_use_https && m_config.scheduler_port != 443) ||
        (!m_config.scheduler_use_https && m_config.scheduler_port != 80)) {
        portPart = ":" + QString::number(m_config.scheduler_port);
    }
    
    return protocol + host + portPart;
}

void ConfigManager::LoadUserConfig() {
    auto env = QProcessEnvironment::systemEnvironment();
    QString osUser = env.value("USER", env.value("USERNAME", "unknown"));
    osUser.remove(QRegularExpression("[^a-zA-Z0-9_-]"));
    if (osUser.isEmpty()) osUser = "unknown";
    
    // Path should be in <Programm-Ordner>/configs/
    QString dir = QCoreApplication::applicationDirPath() + "/configs/";
    QString userConfigPath = dir + osUser + "_config.enc";

    std::ifstream file(userConfigPath.toStdString(), std::ios::binary);
    if (!file.is_open()) {
        return; // No user config yet, use global defaults
    }

    try {
        std::vector<uint8_t> encryptedData((std::istreambuf_iterator<char>(file)),
                                            std::istreambuf_iterator<char>());
        file.close();

        std::vector<uint8_t> plaintext = crypto::Decrypt(encryptedData, m_password);
        std::string jsonStr(plaintext.begin(), plaintext.end());
        auto j = nlohmann::json::parse(jsonStr);

        if (j.contains("networking") && j["networking"].contains("proxy")) {
            auto p = j["networking"]["proxy"];
            if (p.contains("proxy_host")) {
                m_config.proxy.proxy_host = p.value("proxy_host", m_config.proxy.proxy_host);
            }
            m_config.proxy.proxy_port = p.value("proxy_port", m_config.proxy.proxy_port);
            m_config.proxy.proxy_username = p.value("proxy_username", "");
            m_config.proxy.proxy_password = p.value("proxy_password", "");
            m_config.proxy.proxy_active = p.value("proxy_active", false);
        }
    } catch (...) {
        // Fallback to global config if reading/decrypting fails
    }
}

void ConfigManager::SaveUserConfig(const ProxyConfig& proxyCfg) {
    m_config.proxy = proxyCfg;

    auto env = QProcessEnvironment::systemEnvironment();
    QString osUser = env.value("USER", env.value("USERNAME", "unknown"));
    osUser.remove(QRegularExpression("[^a-zA-Z0-9_-]"));
    if (osUser.isEmpty()) osUser = "unknown";
    
    QString dir = QCoreApplication::applicationDirPath() + "/configs/";
    QDir().mkpath(dir); // Ensure the configs directory exists
    QString userConfigPath = dir + osUser + "_config.enc";

    nlohmann::json j;
    j["networking"]["proxy"]["proxy_host"] = proxyCfg.proxy_host;
    j["networking"]["proxy"]["proxy_port"] = proxyCfg.proxy_port;
    if (!proxyCfg.proxy_username.empty()) j["networking"]["proxy"]["proxy_username"] = proxyCfg.proxy_username;
    if (!proxyCfg.proxy_password.empty()) j["networking"]["proxy"]["proxy_password"] = proxyCfg.proxy_password;
    j["networking"]["proxy"]["proxy_active"] = proxyCfg.proxy_active;

    std::string jsonStr = j.dump(4);
    std::vector<uint8_t> plaintext(jsonStr.begin(), jsonStr.end());
    std::vector<uint8_t> encryptedData = crypto::Encrypt(plaintext, m_password);

    std::ofstream file(userConfigPath.toStdString(), std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(encryptedData.data()), encryptedData.size());
    }
}

QNetworkProxy ConfigManager::GetProxy() const {
    QNetworkProxy proxy;
    if (!m_config.proxy.proxy_active || m_config.proxy.proxy_host.empty()) {
        proxy.setType(QNetworkProxy::NoProxy);
        return proxy;
    }

    std::string host = m_config.proxy.proxy_host;
    // Strip https:// or http://
    if (host.starts_with("https://")) host = host.substr(8);
    else if (host.starts_with("http://")) host = host.substr(7);

    proxy.setType(QNetworkProxy::HttpProxy);
    proxy.setHostName(QString::fromStdString(host));
    proxy.setPort(m_config.proxy.proxy_port);

    if (!m_config.proxy.proxy_username.empty() || !m_config.proxy.proxy_password.empty()) {
        proxy.setUser(QString::fromStdString(m_config.proxy.proxy_username));
        proxy.setPassword(QString::fromStdString(m_config.proxy.proxy_password.c_str()));
    }
    
    return proxy;
}

} // namespace mitm::config
