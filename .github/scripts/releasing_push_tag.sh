#!/bin/bash

# SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

set -euo pipefail

NEW_VERSION="$(jq -r '.version' buildspec.json)"

read -p "Push '$NEW_VERSION'? [Enter to proceed] " -n 1 -r

git tag -s "$NEW_VERSION"
git push origin "$NEW_VERSION"
