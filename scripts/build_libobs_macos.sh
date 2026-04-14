#!/bin/bash

# SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

OSX_DEPLOYMENT_TARGET="${OSX_DEPLOYMENT_TARGET:-12.0}"

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

if [[ ! -f "${ROOT_DIR}/.deps/.deps_versions" ]]; then
  echo "Dependencies not found. Please run \`cmake -P scripts/download_deps.cmake\` first."
  exit 1
fi

. "${ROOT_DIR}/.deps/.deps_versions"

LIBOBS_SRC_DIR="${ROOT_DIR}/.deps/obs-studio-${OBS_VERSION}"
LIBOBS_BUILD_DIR="${LIBOBS_SRC_DIR}/build_universal"

# ------------------------------------------------------------------------------
# 1. Setup Phase
# ------------------------------------------------------------------------------
echo "::group::Initialize and Check Dependencies"

CMAKE_OSX_ARCHITECTURES="arm64;x86_64"
CMAKE_OSX_DEPLOYMENT_TARGET="12.0"

if [[ ! -f "${ROOT_DIR}/.deps/.deps_versions" ]]; then
	echo "Dependencies not found. Please run \`cmake -P scripts/download_deps.cmake\` first."
	exit 1
fi

. "${ROOT_DIR}/.deps/.deps_versions"

PREBUILT_DIR="${ROOT_DIR}/.deps/obs-deps-${PREBUILT_VERSION}-universal"
QT6_DIR="${ROOT_DIR}/.deps/obs-deps-qt6-${QT6_VERSION}-universal"

configure_universal() {
  cmake -S "${LIBOBS_SRC_DIR}" \
    -B "${LIBOBS_BUILD_DIR}" \
    -G "Xcode" \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" \
    -DCMAKE_OSX_DEPLOYMENT_TARGET="${OSX_DEPLOYMENT_TARGET}" \
    -DOBS_CMAKE_VERSION=3.0.0 \
    -DENABLE_PLUGINS=OFF \
    -DENABLE_FRONTEND=OFF \
    -DOBS_VERSION_OVERRIDE="${OBS_VERSION}" \
    -DCMAKE_PREFIX_PATH="${PREBUILT_DIR};${QT6_DIR}" \
    -DCMAKE_INSTALL_PREFIX="${ROOT_DIR}/.deps"
}

build_universal() {
  cmake --build "${LIBOBS_BUILD_DIR}" --target obs-frontend-api --config "${1}" --parallel
}

install_universal() {
  cmake --install "${LIBOBS_BUILD_DIR}" --component Development --config "${1}" --prefix "${ROOT_DIR}/.deps"
}

fixup_universal() {
  find "${ROOT_DIR}/.deps" -type f \( -name "*.prl" -o -name "*.cmake" -o -name "*.conf" -o -name "*.pri" \) -print0 | while IFS= read -r -d '' file; do
    sed -i '' \
      -e 's/-framework AGL//g' \
      -e 's/-framework;AGL//g' \
      -e 's/set(__opengl_agl_fw_path "${WrapOpenGL_AGL}")//g' \
      "${file}"
  done
}

if [[ "$#" -eq 0 ]]; then
  configure_universal
  build_universal Debug
  build_universal Release
  install_universal Debug
  install_universal Release
  fixup_universal
else
  "$@"
fi
