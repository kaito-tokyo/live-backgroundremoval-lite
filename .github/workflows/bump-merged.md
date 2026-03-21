---
# SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

on:
  pull_request:
    types: [closed]
    branches: [main]

safe-outputs:
  create-issue:
    title-prefix: "[bump merged] "
    labels: [todo]
    assignees: [umireon]
    expires: false

if: github.event_name == 'pull_request' && github.base_ref == 'main' && github.event.pull_request.merged && startsWith(github.head_ref, 'bump/')
---

# On Bump PR Merged

- **Trigger**: This workflow is triggered after a bump pull request (e.g., `bump/1.2.3`) is merged into the main branch.
- **Goal**: Notify the maintainer (@umireon) that a new version bump has been merged by creating an issue with a link to the merged pull request. Provide a one-liner of git commands to create and push the new tag for the release based on the merged version bump.

