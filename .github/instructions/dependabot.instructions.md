---
# SPDX-FileCopyrightText: 2026 GitHub, Inc.
# SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: CC-BY-4.0

applyTo: ".github/dependabot.{yml,yaml}"
---

# Dependabot options reference (Filtered)

**Source:** [https://docs.github.com/en/code-security/reference/supply-chain-security/dependabot-options-reference](https://docs.github.com/en/code-security/reference/supply-chain-security/dependabot-options-reference)

> This document is a partial extract of the full Source. You can refer to this document for reviewing dependabot.yml. You MUST NOT review or suggest changes to this instruction file itself, because this is written by GitHub and improving this document is out of our project's scope.

## 1. Required keys
| Key | Location | Purpose |
| --- | --- | --- |
| `version` | Top level | Always: `2`. |
| `updates` | Top level | Define each `package-ecosystem` to update. |
| `package-ecosystem` | Under `updates` | `github-actions` and `npm` are used in this project. |
| `directory` | Under `updates` | Define location of manifest files. |
| `schedule.interval` | Under `updates` | Current policy: `weekly`. |

## 2. Multi-ecosystem groups
`multi-ecosystem-groups` is used to combine updates from different ecosystems into a single PR.
- **Behavior**: Updates for dependencies that match a rule are combined.
- **`assignees`**: Specific individual assigned to the group PR.
- **`patterns`**: Supports `*` as a wildcard to match dependency names.

## 3. Configuration options used in this project

### `exclude-paths`
Used for `github-actions` to ignore specific manifest locations.
- **Pattern**: Supports glob patterns like `**` and `*`.
- **Context**: Relative to the `directory` specified.

### `ignore`
Used for `npm` to skip specific dependencies or versions.
- **`dependency-name`**: Must match the name in the manifest/lock file.
- **`versions`**: Supports ranges (e.g., `["*"]` to ignore all versions).

### `schedule`
- **`interval`**: Currently set to `weekly` (runs once a week, default Monday).

## 4. Ecosystem Specifics
- **github-actions**: Dependabot searches `/.github/workflows` and `action.yml` at the root.
- **npm**: Supports `v7, v8, v9, v10`.
