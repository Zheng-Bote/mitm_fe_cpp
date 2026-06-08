cmake_minimum_required(VERSION 3.21)
project(mitm_fe_cpp
VERSION 0.3.0
DESCRIPTION "Admin frontend for the MitM project"
HOMEPAGE_URL "https://github.com/Zheng-Bote/mitm_fe_cpp"
LANGUAGES CXX)

# Require C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Enable Qt Auto features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# --- Dependencies via FetchContent ---
include(FetchContent)

FetchContent_Declare(
  gh_update_checker
  GIT_REPOSITORY https://github.com/Zheng-Bote/gh-update-checker.git
  GIT_TAG        v1.2.0
)
FetchContent_MakeAvailable(gh_update_checker)
if(NOT TARGET gh_update_checker::gh_update_checker)
    add_library(gh_update_checker::gh_update_checker ALIAS gh_update_checker)
endif()

# Find Qt6 (assuming it is installed on the system and CMAKE_PREFIX_PATH is set if necessary)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# Find Conan dependencies
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(libsodium CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(OpenSSL CONFIG REQUIRED)

# --- Configure (für rz_config.hpp) ---
set(EXECUTABLE_NAME "${PROJECT_NAME}")
set(PROG_LONGNAME "${PROJECT_DESCRIPTION}")
set(PROG_CREATED "2026")
set(PROG_COPYRIGHT "ZHENG Robert")
set(PROG_LICENSE "Apache-2.0")
set(PROG_AUTHOR "ZHENG Bote")
set(PROG_ORGANIZATION_NAME "ZHENG Robert")
set(PROG_ORGANIZATION_DOMAIN "net.hase-zheng")
set(CMAKE_QTV "${Qt6_VERSION}")
# create rz_config.hpp
add_subdirectory(configure)

# Source files
set(SOURCES
    src/main.cpp
    src/Crypto.h
    src/Crypto.cpp
    src/Config.h
    src/Config.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/DashboardWidget.h
    src/DashboardWidget.cpp
    src/SchedulerWidget.h
    src/SchedulerWidget.cpp
    src/JobEditorDialog.h
    src/JobEditorDialog.cpp
    src/TransformationWidget.h
    src/TransformationWidget.cpp
    src/DlqWidget.h
    src/DlqWidget.cpp
    src/SettingsWidget.h
    src/SettingsWidget.cpp
    src/SystemLogsWidget.h
    src/SystemLogsWidget.cpp
    src/AuditLogsWidget.h
    src/AuditLogsWidget.cpp
    src/SourcesWidget.h
    src/SourcesWidget.cpp
)

# Create executable
add_executable(${PROJECT_NAME} WIN32 MACOSX_BUNDLE ${SOURCES})

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    nlohmann_json::nlohmann_json
    spdlog::spdlog
    libsodium::libsodium
    httplib::httplib
    openssl::openssl
    gh_update_checker::gh_update_checker
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${PROJECT_SOURCE_DIR}/include
)

# Set runtime output directory (helps with finding DLLs/SOs if not using layout)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/${PROJECT_NAME}_libs.txt" 
     CONTENT "$<TARGET_PROPERTY:${PROJECT_NAME},LINK_LIBRARIES>")
