#!/usr/bin/env bash

# SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

# file: scripts/generate_qt_ts.sh
# description: Generates Qt translation sources (.ts) from src.
# author: Kaito Udagawa <umireon@kaito.tokyo>
# version: 1.0.0
# date: 2026-04-18

set -euo pipefail
shopt -s nullglob

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

LOCALES=(
  de_DE
  es_ES
  fr_FR
  ja_JP
  ko_KR
  pt_BR
  ru_RU
  zh_CN
  zh_TW
)

for locale in "${LOCALES[@]}"; do
  lupdate "${ROOT_DIR}/src" -ts "${ROOT_DIR}/translations/${locale}.ts" -target-language "${locale}"
done
