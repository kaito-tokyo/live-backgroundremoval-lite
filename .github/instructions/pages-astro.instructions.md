---
# SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

# file: .github/instructions/pages-astro.instructions.yml
# author: Kaito Udagawa <umireon@kaito.tokyo>
# version: 1.0.0
# date: 2026-04-17

applyTo: "pages/**/*.astro"
---

# Astro Pages Guidelines

## Inline scripts and stylesheets

There is a script (`pages/scripts/add-csp-hashes.mjs`) to add hashes of inline scripts and stylesheets to the CSP header. Adding inline scripts and stylesheets without `unsafe-eval` or `unsafe-inline` is allowed for this project.

## About localization

The website of this project is built using Astro. `pages/src/pages/*.astro` files are the global version of the pages, and `pages/src/pages/*/*.astro` files are the localized versions.

## Files to be localized

**`pages/src/pages/en/index.astro`**:

- `pages/src/pages/de-de/index.astro`
- `pages/src/pages/es-es/index.astro`
- `pages/src/pages/fr-fr/index.astro`
- `pages/src/pages/ja-jp/index.astro`
- `pages/src/pages/ko-kr/index.astro`
- `pages/src/pages/pt-br/index.astro`
- `pages/src/pages/ru-ru/index.astro`
- `pages/src/pages/zh-cn/index.astro`
- `pages/src/pages/zh-tw/index.astro`

**`pages/src/pages/en/usage.astro`**:

- `pages/src/pages/de-de/usage.astro`
- `pages/src/pages/es-es/usage.astro`
- `pages/src/pages/fr-fr/usage.astro`
- `pages/src/pages/ja-jp/usage.astro`
- `pages/src/pages/ko-kr/usage.astro`
- `pages/src/pages/pt-br/usage.astro`
- `pages/src/pages/ru-ru/usage.astro`
- `pages/src/pages/zh-cn/usage.astro`
- `pages/src/pages/zh-tw/usage.astro`
