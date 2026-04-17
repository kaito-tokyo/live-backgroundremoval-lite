---
# SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

# file: .github/instructions/github-actions.instructions.md
# author: Kaito Udagawa <umireon@kaito.tokyo>
# version: 1.0.0
# date: 2026-04-17

applyTo: ".github/workflows/*.{yml,yaml}"
---

# Development Guidelines for GitHub Actions Workflow

## Guides for workflow syntax and style

- **Name identifies unique step**: Every step that has the same name must have the same step definition across all workflows.
- **Prefer single quotes for environment variables**: Prefer to quote the value of environment variables on run steps using single quotes. This style has good consistency when we handle backslashes in Windows paths. You can switch to use double quotes or write values without quoting only if single quotes result in a messy syntax.
- **Enforce error-prone bash**: Always use `bash --noprofile --norc -euo pipefail -O nullglob {0}` for shell on run steps.
- **Multi-line syntax for run**: Always use multi-line syntax for the content of run to keep the style consistent.
- **Empty String**: `environment: ''` is used to disable environment for a job. This is undocumented behavior of GitHub Actions, but we confirmed that it works as intended and we decide to depend on this behavior.

## Preinstalled softwares on GitHub-hosted runners

- **The command `jq`** is preinstalled on all the runner including `windows-2022`, `macos-15`, `ubuntu-slim`, and `ubuntu-24.04`.

## About the package `obs-studio` on Ubuntu

- **PPA**: We use the PPA provided by the OBS project (`ppa:obsproject/obs-studio`) to install OBS Studio and its development headers on Ubuntu.
- **Development Headers**: The package `obs-studio` includes both the OBS Studio application and its development headers.

