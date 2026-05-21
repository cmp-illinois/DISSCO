#!/usr/bin/env bash
# Build a DISSCO AppImage from an already-installed AppDir tree.
#
# Inputs (env vars, set by the CMake `appimage` target):
#   APPDIR         Path to the staged AppDir (already populated by
#                  `cmake --install ... --prefix $APPDIR/usr`).
#   OUTPUT_DIR     Directory to write the final .AppImage into.
#   SOURCE_DIR     Repo root (for the .desktop, icon, etc.).
#   DISSCO_VERSION e.g. 2.2.0.
#
# The script downloads linuxdeploy + linuxdeploy-plugin-qt on demand into
# ${OUTPUT_DIR}/.linuxdeploy/ so that fresh CI runners and dev machines work
# without prior setup.

set -euo pipefail

: "${APPDIR:?APPDIR not set}"
: "${OUTPUT_DIR:?OUTPUT_DIR not set}"
: "${SOURCE_DIR:?SOURCE_DIR not set}"
: "${DISSCO_VERSION:?DISSCO_VERSION not set}"

arch="$(uname -m)"
tools_dir="${OUTPUT_DIR}/.linuxdeploy"
mkdir -p "${tools_dir}"

linuxdeploy="${tools_dir}/linuxdeploy-${arch}.AppImage"
linuxdeploy_qt="${tools_dir}/linuxdeploy-plugin-qt-${arch}.AppImage"

fetch() {
    local url="$1" dest="$2"
    if [[ ! -x "${dest}" ]]; then
        echo "Downloading ${url}"
        curl -fSL -o "${dest}" "${url}"
        chmod +x "${dest}"
    fi
}

# Pinned to "continuous" — linuxdeploy's recommended stable channel. Bump
# deliberately if the release toolchain regresses.
fetch "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-${arch}.AppImage" \
      "${linuxdeploy}"
fetch "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-${arch}.AppImage" \
      "${linuxdeploy_qt}"

# Stage the .desktop and icon into the AppDir. linuxdeploy picks them up
# from usr/share/applications and usr/share/icons.
install -Dm644 "${SOURCE_DIR}/packaging/linux/LASSIE.desktop" \
    "${APPDIR}/usr/share/applications/LASSIE.desktop"
install -Dm644 "${SOURCE_DIR}/packaging/linux/LASSIE.png" \
    "${APPDIR}/usr/share/icons/hicolor/256x256/apps/LASSIE.png"

# On GitHub Actions' ubuntu-* runners, FUSE isn't available, so AppImages
# must be extracted instead of executed directly.
export APPIMAGE_EXTRACT_AND_RUN=1

# linuxdeploy-plugin-qt needs to know which Qt to bundle. If qmake is in
# PATH (e.g. via aqtinstall in CI or `brew link qt@6`), it uses it
# automatically; otherwise the caller can export QMAKE.
output_name="DISSCO-${DISSCO_VERSION}-$(uname -s)-${arch}.AppImage"

cd "${OUTPUT_DIR}"
OUTPUT="${output_name}" \
"${linuxdeploy}" \
    --appdir "${APPDIR}" \
    --plugin qt \
    --desktop-file "${APPDIR}/usr/share/applications/LASSIE.desktop" \
    --icon-file "${APPDIR}/usr/share/icons/hicolor/256x256/apps/LASSIE.png" \
    --output appimage

echo "Built ${OUTPUT_DIR}/${output_name}"
