#!/bin/bash

# SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

cd "$(dirname "${BASH_SOURCE[0]}")/.."

if [[ -d .deps_vendor ]]; then
  echo 'The .deps_vendor directory already exists. Exiting...' >&2
  exit 1
fi

echo '## Checking if required tools can be called without errors'

if [[ -z "${VCPKG_ROOT:-}" ]]; then
  echo 'ERROR: VCPKG_ROOT is not set' >&2
  exit 1
fi

cmake --version
ninja --version
pkg-config --version
"${VCPKG_ROOT}/vcpkg" --version
xcodebuild -version

echo '## Installing dependencies'

export VCPKG_BINARY_SOURCES
if [[ -z "${VCPKG_BINARY_SOURCES:-}" ]]; then
  VCPKG_BINARY_SOURCES='clear;http,https://vcpkg-obs.kaito.tokyo/{name}/{version}/{sha}'
fi

"${VCPKG_ROOT}/vcpkg" install --triplet=arm64-osx-obs --x-install-root=.deps_vendor/vcpkg_installed_arm64
"${VCPKG_ROOT}/vcpkg" install --triplet=x64-osx-obs --x-install-root=.deps_vendor/vcpkg_installed_x64

./scripts/lipo_vcpkg_macos.sh \
  .deps_vendor/vcpkg_installed_universal/universal-osx-obs \
  .deps_vendor/vcpkg_installed_arm64/arm64-osx-obs \
  .deps_vendor/vcpkg_installed_x64/x64-osx-obs

cmake -P scripts/download_deps.cmake

./scripts/build_libobs_macos.sh
