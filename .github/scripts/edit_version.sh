#!/bin/bash

# SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

cd "$(dirname "$BASH_SOURCE")/../.."

INDENT_WIDTH=2

echo "Usage: $0 [new_version]"
echo "Bumps the version on the working tree."
echo "Just shows current versions if no new version is provided."
echo
echo "Version in buildspec.json: $(jq -r '.version' buildspec.json)"
echo "Version in data/manifest.json: $(jq -r '.version' data/manifest.json)"
echo "Version in pages/package.json: $(jq -r '.version' pages/package.json)"

if [[ -z "${1:-}" ]]; then
  echo "INFO: No new version provided. Exiting."
  exit 0
fi

NEW_VERSION="$1"

echo "Indent width for JSON files: $INDENT_WIDTH spaces"

jq --indent "$INDENT_WIDTH" ".version = \"$NEW_VERSION\"" buildspec.json > buildspec.json.new
cp buildspec.json.new buildspec.json
rm buildspec.json.new
echo "buildspec.json updated. New version in buildspec.json: $(jq -r '.version' buildspec.json)"

jq --indent "$INDENT_WIDTH" ".version = \"$NEW_VERSION\"" data/manifest.json > data/manifest.json.new
cp data/manifest.json.new data/manifest.json
rm data/manifest.json.new
echo "data/manifest.json updated. New version in data/manifest.json: $(jq -r '.version' data/manifest.json)"

jq --indent "$INDENT_WIDTH" ".version = \"$NEW_VERSION\"" pages/package.json > pages/package.json.new
cp pages/package.json.new pages/package.json
rm pages/package.json.new
echo "pages/package.json updated. New version in pages/package.json: $(jq -r '.version' pages/package.json)"

(cd pages && npm install --package-lock-only --ignore-scripts --no-audit --no-fund)

echo "Version bump completed. All files updated to version $NEW_VERSION."
