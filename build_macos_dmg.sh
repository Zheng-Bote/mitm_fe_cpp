#!/usr/bin/env bash
# Builds mitm_fe_cpp (Release) and packages it as a macOS DMG.
#
# Prerequisites (Homebrew): qt cmake ninja conan libfido2
#   brew install qt cmake ninja conan libfido2 && conan profile detect
#
# Usage: ./build_macos_dmg.sh            # build + package
#        ./build_macos_dmg.sh --no-build # package an existing build only
#
# Output: build/dmg/mitm_fe_cpp-<version>-macos-<arch>.dmg
set -euo pipefail

cd "$(dirname "$0")"

APP_NAME="mitm_fe_cpp"
BUNDLE_ID="net.hase-zheng.mitm_fe_cpp"
VOLUME_NAME="MitM Admin Frontend"
LOGO="img/logo_256x256.png"
QT_PREFIX="$(brew --prefix)"
BUILD_DIR="build/Release"
OUT_DIR="build/dmg"

VERSION="$(sed -n '/^project(/,/)/s/^[[:space:]]*VERSION[[:space:]]\{1,\}\([0-9.]*\).*/\1/p' CMakeLists.txt)"
ARCH="$(uname -m)"

# --- Build ---
if [[ "${1:-}" != "--no-build" ]]; then
    conan install . --build=missing -s build_type=Release \
        -c tools.cmake.cmaketoolchain:generator=Ninja
    cmake --preset conan-release -DCMAKE_PREFIX_PATH="$QT_PREFIX"
    cmake --build --preset conan-release
fi

SRC_APP="$BUILD_DIR/bin/$APP_NAME.app"
[[ -d "$SRC_APP" ]] || { echo "Error: $SRC_APP not found" >&2; exit 1; }

# --- Stage a copy of the bundle ---
STAGE="$OUT_DIR/stage"
rm -rf "$STAGE"
mkdir -p "$STAGE"
cp -R "$SRC_APP" "$STAGE/"
APP="$STAGE/$APP_NAME.app"

# --- App icon (.icns) from the logo ---
ICONSET="$OUT_DIR/$APP_NAME.iconset"
rm -rf "$ICONSET"
mkdir -p "$ICONSET"
for size in 16 32 128 256; do
    sips -z "$size" "$size" "$LOGO" --out "$ICONSET/icon_${size}x${size}.png" >/dev/null
    double=$((size * 2))
    if (( double <= 256 )); then
        sips -z "$double" "$double" "$LOGO" --out "$ICONSET/icon_${size}x${size}@2x.png" >/dev/null
    fi
done
mkdir -p "$APP/Contents/Resources"
iconutil -c icns "$ICONSET" -o "$APP/Contents/Resources/$APP_NAME.icns"
rm -rf "$ICONSET"

# --- Info.plist metadata ---
PLIST="$APP/Contents/Info.plist"
set_plist() { /usr/libexec/PlistBuddy -c "Set :$1 $2" "$PLIST" 2>/dev/null \
              || /usr/libexec/PlistBuddy -c "Add :$1 string $2" "$PLIST"; }
set_plist CFBundleIconFile "$APP_NAME.icns"
set_plist CFBundleName "$APP_NAME"
set_plist CFBundleIdentifier "$BUNDLE_ID"
set_plist CFBundleShortVersionString "$VERSION"
set_plist CFBundleVersion "$VERSION"
set_plist NSHumanReadableCopyright "ZHENG Robert, Apache-2.0"

# --- Bundle Qt frameworks and Homebrew dylibs (libfido2, ...) ---
"$QT_PREFIX/bin/macdeployqt" "$APP" -verbose=1
# Homebrew ships these plugins in qtbase, but their QtSvg/QtPdf frameworks live
# in other kegs that macdeployqt cannot resolve; the app uses neither SVG nor PDF.
rm -f "$APP/Contents/PlugIns/iconengines/libqsvgicon.dylib" \
      "$APP/Contents/PlugIns/imageformats/libqpdf.dylib"

# --- Ad-hoc signature (required on Apple Silicon after modifying the bundle) ---
codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict "$APP"

# --- Create DMG ---
ln -s /Applications "$STAGE/Applications"
DMG="$OUT_DIR/$APP_NAME-$VERSION-macos-$ARCH.dmg"
rm -f "$DMG"
hdiutil create -volname "$VOLUME_NAME" -srcfolder "$STAGE" \
    -ov -format UDZO "$DMG"
rm -rf "$STAGE"

echo "DMG created: $DMG"
