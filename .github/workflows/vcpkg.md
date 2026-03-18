---
# SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

description: Sync vcpkg-related files with kaito-tokyo/vcpkg-obs-kaito-tokyo. COPILOT_GITHUB_TOKEN needs to be configured.

on:
  schedule: daily
  push:
    branches: [main]
  workflow_dispatch:

permissions:
  contents: read

safe-outputs:
  create-pull-request:
    labels: [dependencies]
    fallback-as-issue: false

engine:
  id: copilot
  model: gpt-5.4-mini
---

# Sync vcpkg-related files with kaito-tokyo/vcpkg-obs-kaito-tokyo

## Context

`kaito-tokyo/vcpkg-obs-kaito-tokyo` is the upstream repository for vcpkg binary packages.

## Objective

Keep the following files in sync between this repository and `kaito-tokyo/vcpkg-obs-kaito-tokyo`:

1. `<PROJECT_ROOT>/vcpkg-triplets/*.cmake`
2. `<PROJECT_ROOT>/vcpkg-configuration.json`

# Goal

Create a Pull Request to pull changes from `kaito-tokyo/vcpkg-obs-kaito-tokyo` to this repository when you detect these files in the upstream are newer than those in this repository.
