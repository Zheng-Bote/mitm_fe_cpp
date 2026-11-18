#!/usr/bin/env bash
# Builds mitm_fe_cpp (Release) and packages it as a Linux AppImage.
#
# Prerequisites: Qt 6 (system packages or Qt installer), cmake, ninja or make,
#                conan, libfido2 (libfido2-dev), curl, file
#   sudo apt install cmake ninja-build libfido2-dev curl file && pipx install conan
#   conan profile detect   # only once
#
# Usage: ./build_linux_appimage.sh            # build + package
#        ./build_linux_appimage.sh --no-build # package an existing build only
#
# Qt is located in this order:
#   QMAKE=/path/to/qmake        explicit qmake
#   QT_PREFIX=/path/to/gcc_64   Qt installation (uses $QT_PREFIX/bin/qmake)
#   Qt6_DIR from build/Release/CMakeCache.txt
#   qmake6 / qmake in PATH
#
# Output: build/appimage/mitm_fe_cpp-<version>-<arch>.AppImage
#
# The AppImage stores config.enc, configs/ and data/ next to the .AppImage file.
# It runs on distributions with the same or a newer glibc than the build host.
set -euo pipefail

cd "$(dirname "$0")"

APP_NAME="mitm_fe_cpp"
DISPLAY_NAME="MitM Admin Frontend"
LOGO="img/logo_256x256.png"
BUILD_DIR="build/Release"
OUT_DIR="build/appimage"
TOOLS_DIR="$OUT_DIR/tools"
APPDIR="$OUT_DIR/AppDir"

VERSION="$(sed -n '/^project(/,/)/s/^[[:space:]]*VERSION[[:space:]]\{1,\}\([0-9.]*\).*/\1/p' CMakeLists.txt)"
ARCH="$(uname -m)"

die() { echo "Error: $*" >&2; exit 1; }

# --- Build ---
if [[ "${1:-}" != "--no-build" ]]; then
    conan install . --build=missing -s build_type=Release
    if [[ -n "${QT_PREFIX:-}" ]]; then
        cmake --preset conan-release -DCMAKE_PREFIX_PATH="$QT_PREFIX"
    else
        cmake --preset conan-release
    fi
    cmake --build --preset conan-release -j "$(nproc)"
fi

BIN="$BUILD_DIR/bin/$APP_NAME"
[[ -x "$BIN" ]] || die "$BIN not found (build first or omit --no-build)"

# --- Locate qmake (linuxdeploy-plugin-qt reads the Qt paths from it) ---
if [[ -z "${QMAKE:-}" ]]; then
    if [[ -n "${QT_PREFIX:-}" ]]; then
        QMAKE="$QT_PREFIX/bin/qmake"
    else
        qt6_dir="$(sed -n 's/^Qt6_DIR:PATH=//p' "$BUILD_DIR/CMakeCache.txt" 2>/dev/null || true)"
        if [[ -n "$qt6_dir" && -x "$qt6_dir/../../../bin/qmake" ]]; then
            QMAKE="$(cd "$qt6_dir/../../../bin" && pwd)/qmake"
        else
            QMAKE="$(command -v qmake6 || command -v qmake || true)"
        fi
    fi
fi
[[ -n "$QMAKE" && -x "$QMAKE" ]] || die "qmake not found; set QMAKE or QT_PREFIX"
export QMAKE
QT_LIBS="$("$QMAKE" -query QT_INSTALL_LIBS)"
echo "Using Qt $("$QMAKE" -query QT_VERSION) from $QT_LIBS"

# --- Tools (downloaded once; extract-and-run avoids the FUSE requirement) ---
export APPIMAGE_EXTRACT_AND_RUN=1
mkdir -p "$TOOLS_DIR"
fetch_tool() {
    local name="$1" url="$2"
    if [[ ! -x "$TOOLS_DIR/$name" ]]; then
        echo "Downloading $name"
        curl -fL --retry 3 -o "$TOOLS_DIR/$name.part" "$url"
        chmod +x "$TOOLS_DIR/$name.part"
        mv "$TOOLS_DIR/$name.part" "$TOOLS_DIR/$name"
    fi
}
fetch_tool "linuxdeploy-$ARCH.AppImage" \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-$ARCH.AppImage"
fetch_tool "linuxdeploy-plugin-qt-$ARCH.AppImage" \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-$ARCH.AppImage"
# linuxdeploy finds its plugins in PATH
export PATH="$(cd "$TOOLS_DIR" && pwd):$PATH"

# --- Desktop entry and icon ---
rm -rf "$APPDIR"
mkdir -p "$APPDIR"
DESKTOP="$OUT_DIR/$APP_NAME.desktop"
cat > "$DESKTOP" <<EOF
[Desktop Entry]
Type=Application
Name=$DISPLAY_NAME
Comment=Admin frontend for the MitM project
Exec=$APP_NAME
Icon=$APP_NAME
Categories=Development;Utility;
Terminal=false
X-AppImage-Version=$VERSION
EOF
ICON="$OUT_DIR/$APP_NAME.png"
cp "$LOGO" "$ICON"

# --- Bundle the binary, its libraries and the Qt plugins; create the AppImage ---
# Qt libraries (incl. ICU) are resolved from the Qt installation first.
export LD_LIBRARY_PATH="$QT_LIBS${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
# Wayland support in addition to xcb, if the Qt installation ships the plugin
# (libqwayland.so since Qt 6.8, libqwayland-generic.so before).
QT_PLATFORMS="$("$QMAKE" -query QT_INSTALL_PLUGINS)/platforms"
for wayland in libqwayland.so libqwayland-generic.so; do
    if [[ -f "$QT_PLATFORMS/$wayland" ]]; then
        export EXTRA_PLATFORM_PLUGINS="$wayland"
        break
    fi
done
APPIMAGE_FILE="$APP_NAME-$VERSION-$ARCH.AppImage"
export LDAI_OUTPUT="$APPIMAGE_FILE"   # name used by the appimage output plugin
rm -f "$OUT_DIR/$APPIMAGE_FILE"

(
    cd "$OUT_DIR"
    "./tools/linuxdeploy-$ARCH.AppImage" \
        --appdir AppDir \
        --executable "../../$BIN" \
        --desktop-file "$APP_NAME.desktop" \
        --icon-file "$APP_NAME.png" \
        --plugin qt \
        --output appimage
)

rm -rf "$APPDIR" "$DESKTOP" "$ICON"
[[ -f "$OUT_DIR/$APPIMAGE_FILE" ]] || die "AppImage was not created"
echo "AppImage created: $OUT_DIR/$APPIMAGE_FILE"
