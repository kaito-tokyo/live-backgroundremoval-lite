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

# Update JSON file using jq
FLATPAK_JSON_PATH="${REPO_ROOT}/unsupported/flatpak/com.obsproject.Studio.Plugin.LiveBackgroundRemovalLite.json"
jq --arg ver "$NEW_VERSION" --arg hash "$COMMIT_HASH" '
    (.modules[] | select(.name == "live-backgroundremoval-lite").sources[] | select(.type == "git")) 
    |= (.tag = $ver | .commit = $hash)
' "$FLATPAK_JSON_PATH" > "$FLATPAK_JSON_PATH.tmp"
if [ $? -ne 0 ]; then
    echo "❌ Error: jq failed to update the JSON manifest."
    rm -f "$FLATPAK_JSON_PATH.tmp"
    exit 1
fi
if [ ! -s "$FLATPAK_JSON_PATH.tmp" ]; then
    echo "❌ Error: jq produced an empty output file. Manifest not updated."
    rm -f "$FLATPAK_JSON_PATH.tmp"
    exit 1
fi
mv "$FLATPAK_JSON_PATH.tmp" "$FLATPAK_JSON_PATH"
# Update metainfo.xml file

# Add release only if not already present
if ! xmlstarlet sel -t -c "/component/releases/release[@version='${NEW_VERSION}']" "${FLATPAK_METAINFO_PATH}" | grep -q .; then
    xmlstarlet ed -L \
        -i "/component/releases/release[1]" -t elem -n "release" \
        -i "/component/releases/release[1]" -t attr -n "version" -v "${NEW_VERSION}" \
        -i "/component/releases/release[1]" -t attr -n "date" -v "${CURRENT_DATE}" \
        "${FLATPAK_METAINFO_PATH}"
else
    echo "  ℹ️ metainfo.xml: release ${NEW_VERSION} already exists, not adding duplicate."
fi

echo "  ✅ Flatpak manifests updated."
echo "--------------------------------------------------"

echo "🎉 All package manifests have been updated successfully!"
echo "Please review the changes with 'git status' and create a pull request."