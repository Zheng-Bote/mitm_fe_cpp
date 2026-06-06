cmake_minimum_required(VERSION 3.21)
project(mitm_fe_cpp VERSION 0.1.0 LANGUAGES CXX)

# Require C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt Auto features
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 (assuming it is installed on the system and CMAKE_PREFIX_PATH is set if necessary)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

# Find Conan dependencies
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Source files
set(SOURCES
    src/main.cpp
    src/MainWindow.h
    src/MainWindow.cpp
    src/DashboardWidget.h
    src/DashboardWidget.cpp
    src/SchedulerWidget.h
    src/SchedulerWidget.cpp
    src/JobEditorDialog.h
    src/JobEditorDialog.cpp
    src/RulesWidget.h
    src/RulesWidget.cpp
    src/DlqWidget.h
    src/DlqWidget.cpp
    src/SettingsWidget.h
    src/SettingsWidget.cpp
    src/SystemLogsWidget.h
    src/SystemLogsWidget.cpp
    src/AuditLogsWidget.h
    src/AuditLogsWidget.cpp
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
)

# Set runtime output directory (helps with finding DLLs/SOs if not using layout)
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
