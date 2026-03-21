#!/bin/bash

# SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

cd "$(dirname "$BASH_SOURCE")/../.."

BUILDSPEC_VERSION=$(jq -r '.version' buildspec.json)
MANIFEST_VERSION=$(jq -r '.version' data/manifest.json)
PAGES_VERSION=$(jq -r '.version' pages/package.json)

echo "Usage: $0 [branch_name]"
echo "Creates a pull request to bump the version on the current branch of working tree."
echo
echo "Version in buildspec.json: $BUILDSPEC_VERSION"
echo "Version in data/manifest.json: $MANIFEST_VERSION"
echo "Version in pages/package.json: $PAGES_VERSION"

if [[ "$MANIFEST_VERSION" != "$BUILDSPEC_VERSION" || "$PAGES_VERSION" != "$BUILDSPEC_VERSION" ]]; then
  echo "ERROR: Version mismatch detected. Please ensure all versions are the same before creating a bump PR."
  exit 1
fi

if [[ -z "${1:-}" ]]; then
  echo "WARN: No branch name provided. Exiting."
  exit 1
fi

HEAD_BRANCH="$1"

GH_PR_CREATE_ARGS=(
  --assignee umireon
  --head "$HEAD_BRANCH"
  --title "[release bump] Bump version to $BUILDSPEC_VERSION"
  --web
)

gh pr create "${GH_PR_CREATE_ARGS[@]}"

echo "Pull request created successfully to bump version to $BUILDSPEC_VERSION."
