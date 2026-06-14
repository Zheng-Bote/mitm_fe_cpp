/**
 * SPDX-FileComment: Config
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file Config.h
 * @brief Config
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#pragma once

#include <string>
#include <vector>
#include <stdexcept>
#include <QString>

namespace mitm::config {

struct AdminUser {
    std::string username;
    std::string token;
};

struct ProxyConfig {
    std::string proxy_host;
    int proxy_port{8080};
    std::string proxy_username;
    std::string proxy_password;
    bool proxy_active{false};
};

struct ConfigData {
    std::string scheduler_host;
    int scheduler_port{8080};
    std::string log_level{"info"};
    std::vector<AdminUser> admin_users;
    ProxyConfig proxy;
};

class ConfigManager {
public:
    static ConfigManager& GetInstance() {
        static ConfigManager instance;
        return instance;
    }

    void LoadEncryptedConfig(const std::string& filepath, const std::string& password);
    void LoadUserConfig();
    void SaveUserConfig(const ProxyConfig& proxyCfg);
    
    const ConfigData& GetConfig() const { return m_config; }
    
    bool IsAdmin(const std::string& username) const;
    QString GetAuthHeader() const;
    QString GetHostUrl() const;
    
    void SetCurrentUserRoles(const std::vector<std::string>& roles) { m_userRoles = roles; }
    bool HasRole(const std::string& role) const {
        return std::find(m_userRoles.begin(), m_userRoles.end(), role) != m_userRoles.end();
    }
    
    std::optional<std::string> GetProxyString() const;

private:
    ConfigManager() = default;
    ~ConfigManager() = default;

    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    ConfigData m_config;
    std::string m_password;
    std::string m_globalConfigPath;
    std::vector<std::string> m_userRoles;
};

} // namespace mitm::config
