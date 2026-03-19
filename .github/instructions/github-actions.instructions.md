---
# SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

applyTo: "**/*.{yml,yaml}"
---

# GitHub Actions Workflow Guidelines

<PreinstalledSoftwaresOnRunnerImages>
- xcbeautify is preinstalled on macos-15.
- jq is preinstalled on ubuntu-slim, ubuntu-24.04, macos-15, and windows-2022.
</PreinstalledSoftwaresOnRunnerImages>

<ObsStudioOnUbuntu>
- **PPA**: We use the PPA provided by the OBS project (ppa:obsproject/obs-studio) to install OBS Studio and its development headers on Ubuntu.
- **Development Headers**: The package named obs-studio includes both the OBS Studio application and its development headers.
</ObsStudioOnUbuntu>

<EnvironmentOnJobs>
- **Empty String**: `environment: ''` is used to disable environment for a job. This is undocumented behavior of GitHub Actions, but we confirmed that it works as intended and we decide to depend on this behavior.
</EnvironmentOnJobs>
