# MitM Admin Frontend (C++23 & Qt6)

This directory contains the initial project structure for the C++23 / Qt6 Admin Frontend for the MitM Data Aggregator.

## Requirements
*   **Compiler**: C++23 compatible (GCC 13+, Clang 16+, MSVC 19.38+)
*   **Build System**: CMake 3.21+
*   **Package Manager**: Conan 2.x
*   **Framework**: Qt6 (installed system-wide or via Qt Online Installer)

## Building the Project

We use Conan to fetch standard C++ libraries (`spdlog`, `nlohmann_json`) and CMake to build the Qt6 application.

### 1. Install Conan Dependencies
```bash
cd admin-frontend/mitm_fe_cpp
conan install . --build=missing
```

### 2. Configure with CMake
Assuming you use the Conan `cmake_layout` default (which generates files in `build/Release`):

```bash
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=Release/generators/conan_toolchain.cmake -DCMAKE_BUILD_TYPE=Release
```

*Note: If Qt6 is not in your system path, you may need to add `-DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/gcc_64`.*

### 3. Build
```bash
cmake --build . --config Release
```

### 4. Run
```bash
./bin/mitm_fe_cpp
```
