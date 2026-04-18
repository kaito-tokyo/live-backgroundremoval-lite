# syntax=docker/dockerfile:1
# check=skip=FromPlatformFlagConstDisallowed

# SPDX-FileCopyrightText: 2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

FROM --platform=linux/amd64 ubuntu:noble-20260217

RUN ["install", "-d", "/etc/apt/keyrings"]

COPY <<'EOF' /etc/apt/sources.list
deb http://archive.ubuntu.com/ubuntu noble main restricted universe multiverse
EOF

COPY obsproject.asc /etc/apt/keyrings/obsproject.asc

COPY <<'EOF' /etc/apt/sources.list.d/obsproject.list
deb [signed-by=/etc/apt/keyrings/obsproject.asc] https://ppa.launchpadcontent.net/obsproject/obs-studio/ubuntu noble main
EOF

RUN <<'EOT' bash -euo pipefail -O nullglob
dependencies_dev=(
  build-essential
  ca-certificates
  cmake
  curl
  file
  git
  jq
  libgles2-mesa-dev
  libsimde-dev
  ninja-build
  obs-studio
  pkg-config
  qt6-base-dev
  qt6-base-private-dev
  qt6-svg-dev
  sudo
  tar
  tini
  unzip
  zip
)
apt-get update
DEBIAN_FRONTEND=noninteractive apt-get install -y --no-install-recommends "${dependencies_dev[@]}"
rm -rf /var/lib/apt/lists/*
EOT

ENV VCPKG_ROOT=/usr/local/vcpkg VCPKG_DOWNLOADS=/usr/local/vcpkg-downloads

RUN <<'EOT' bash -euo pipefail -O nullglob
install -d "${VCPKG_DOWNLOADS}"
git clone --filter=blob:none --no-checkout https://github.com/microsoft/vcpkg.git "${VCPKG_ROOT}"
cd "${VCPKG_ROOT}"
git checkout 2025.08.27
"${VCPKG_ROOT}/bootstrap-vcpkg.sh" -disableMetrics
groupadd -g 999 vcpkg
chgrp -R vcpkg "${VCPKG_ROOT}" "${VCPKG_DOWNLOADS}"
chmod -R ug=rwX,o=rX "${VCPKG_ROOT}" "${VCPKG_DOWNLOADS}"
EOT

RUN <<'EOT' bash -euo pipefail -O nullglob
if id ubuntu >/dev/null 2>&1; then
  userdel ubuntu
  rm -rf /home/ubuntu
fi
groupadd -g 1000 vscode
useradd -u 1000 -g vscode -m vscode -G vcpkg -s /bin/bash
EOT

COPY --chmod=400 <<'EOF' /etc/sudoers.d/vscode
vscode ALL=(ALL) NOPASSWD: ALL
EOF

CMD ["/bin/bash"]

USER vscode
