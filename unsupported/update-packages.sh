#!/bin/bash

# Exit immediately if a command exits with a non-zero status.
set -euo pipefail

# --- Pre-flight Checks ---

# Check if xmlstarlet is installed.
if ! command -v xmlstarlet &> /dev/null; then
    echo "⚠️ Error: 'xmlstarlet' is not installed."
    echo "Please install it using your system's package manager (e.g., brew install xmlstarlet)."
    exit 1
fi

# --- Variables ---

REPO_ROOT="$(git rev-parse --show-toplevel)"
ARCH_PKGBUILD_PATH="${REPO_ROOT}/unsupported/arch/live-backgroundremoval-lite/PKGBUILD"
FLATPAK_YAML_PATH="${REPO_ROOT}/unsupported/flatpak/com.obsproject.Studio.Plugin.LiveBackgroundRemovalLite.yaml"
FLATPAK_METAINFO_PATH="${REPO_ROOT}/unsupported/flatpak/com.obsproject.Studio.Plugin.LiveBackgroundRemovalLite.metainfo.xml"
REPO_URL="https://github.com/kaito-tokyo/live-backgroundremoval-lite"
PKG_NAME="live-backgroundremoval-lite"

# --- Main Script ---

# Check for new version argument.
if [ -z "${1-}" ]; then
  echo "Usage: $0 <new-version>"
  echo "Example: $0 1.8.6"
  exit 1
fi

NEW_VERSION="${1#v}" # Remove 'v' prefix if it exists
CURRENT_DATE=$(date +%Y-%m-%d)

echo "🚀 Starting package update for version ${NEW_VERSION}..."
echo "--------------------------------------------------"

# 1. Update Arch Linux PKGBUILD
echo "📦 Updating Arch Linux PKGBUILD..."
DOWNLOAD_URL="${REPO_URL}/archive/refs/tags/${NEW_VERSION}.tar.gz"
echo "  Downloading source to calculate checksum..."
SHA256_SUM=$(curl -sL "${DOWNLOAD_URL}" | sha256sum | awk '{print $1}')

if [ -z "${SHA256_SUM}" ]; then
    echo "❌ Error: Failed to calculate SHA256 checksum. Please check the URL."
    exit 1
fi

sed -i '' -e "s/^pkgver=.*/pkgver=${NEW_VERSION}/" "${ARCH_PKGBUILD_PATH}"
sed -i '' -e "s/^sha256sums=('.*')/sha256sums=('${SHA256_SUM}')/" "${ARCH_PKGBUILD_PATH}"
echo "  ✅ PKGBUILD updated."
echo "--------------------------------------------------"

# 2. Update Flatpak Manifests
echo "📦 Updating Flatpak manifests..."
echo "  Fetching git commit hash for tag '${NEW_VERSION}'..."
COMMIT_HASH=$(git rev-list -n 1 "${NEW_VERSION}")

if [ -z "${COMMIT_HASH}" ]; then
    echo "❌ Error: Git tag '${NEW_VERSION}' not found."
    exit 1
fi

# Update YAML file (macOS/BSD sed compatible)
sed -i '' "/name: ${PKG_NAME}/,/- type: git/{
s/tag: .*/tag: ${NEW_VERSION}/
s/commit: .*/commit: '${COMMIT_HASH}'/
}" "${FLATPAK_YAML_PATH}"

# Update metainfo.xml file
xmlstarlet ed -L \
    -i "/component/releases/release[1]" -t elem -n "release" \
    -i "/component/releases/release[1]" -t attr -n "version" -v "${NEW_VERSION}" \
    -i "/component/releases/release[1]" -t attr -n "date" -v "${CURRENT_DATE}" \
    "${FLATPAK_METAINFO_PATH}"

echo "  ✅ Flatpak manifests updated."
echo "--------------------------------------------------"

echo "🎉 All package manifests have been updated successfully!"
echo "Please review the changes with 'git status' and create a pull request."