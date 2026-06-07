/**
 * SPDX-FileComment: rz_config
 * SPDX-FileType: SOURCE
 * SPDX-FileContributor: ZHENG Robert
 * SPDX-FileCopyrightText: 2026 ZHENG Robert
 * SPDX-License-Identifier: Apache-2.0
 *
 * @file rz_config.hpp.in
 * @brief rz_config
 * @version 0.2.0
 * @date 2026-06-07
 *
 * @author ZHENG Robert (robert@hase-zheng.net)
 * @copyright Copyright (c) 2026 ZHENG Robert
 * @LICENSE Apache-2.0
 */

#pragma once

#include <string_view>
#include <cstdint>

namespace rz {
namespace config {
constexpr std::string_view PROJECT_NAME = "mitm_fe_cpp";
constexpr std::string_view PROG_LONGNAME = "Admin frontend for the MitM project";
constexpr std::string_view PROJECT_DESCRIPTION = "Admin frontend for the MitM project";

constexpr std::string_view EXECUTABLE_NAME = "mitm_fe_cpp";

constexpr std::string_view VERSION = "0.2.0";
constexpr std::int32_t PROJECT_VERSION_MAJOR { 0 };
constexpr std::int32_t PROJECT_VERSION_MINOR { 2 };
constexpr std::int32_t PROJECT_VERSION_PATCH { 0 };

constexpr std::string_view PROJECT_HOMEPAGE_URL = "https://github.com/Zheng-Bote/mitm_fe_cpp";
constexpr std::string_view AUTHOR = "ZHENG Bote";

constexpr std::string_view CREATED_YEAR = "2026";
constexpr std::string_view COPYRIGHT = "ZHENG Robert";
constexpr std::string_view LICENSE = "Apache-2.0";

constexpr std::string_view ORGANIZATION = "ZHENG Robert";
constexpr std::string_view PROJECT_DOMAIN = "net.hase-zheng";

constexpr std::string_view CMAKE_CXX_STANDARD = "c++23";
constexpr std::string_view CMAKE_CXX_COMPILER =
    "GNU 15.2.0";
constexpr std::string_view QT_VERSION_BUILD = "6.9.2";
} // namespace config
} // namespace rz
