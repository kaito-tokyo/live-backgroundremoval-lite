<!--
SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>

SPDX-License-Identifier: Apache-2.0
-->

# Build Instructions for Fedora

## Prerequisites

Install the required build dependencies:

```bash
sudo dnf install @development-tools cmake fmt-devel git libcurl-devel ncnn-devel ninja-build obs-studio obs-studio-devel qt6-linguist qt6-qtbase-private-devel rpm-build
```

## Build and install steps

**Clone the repository**:

```bash
git clone https://github.com/kaito-tokyo/live-backgroundremoval-lite.git
cd live-backgroundremoval-lite
```

**Configure and build**:

```bash
cmake --preset fedora && cmake --build --preset fedora
```

**Package**:

```bash
(cd build_fedora && cpack -G RPM -C RelWithDebInfo)
```

**Install the plugin**:

```bash
sudo dnf install ./release/*.rpm
```

## Uninstalling the plugin

```bash
sudo dnf remove live-backgroundremoval-lite
```
