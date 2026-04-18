# syntax=docker/dockerfile:1

# SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
#
# SPDX-License-Identifier: Apache-2.0

FROM fedora:latest

RUN <<'EOT' bash -euo pipefail -O nullglob
dependencies_dev=(
  fmt-devel
  git
  libcurl-devel
  ncnn-devel
  ninja-build
  obs-studio-devel
  qt6-linguist
  qt6-qtbase-private-devel
  rpm-build
)
dnf install -y "${dependencies_dev[@]}" && dnf clean all && rm -rf /var/cache/dnf
EOT

RUN <<'EOT' bash -euo pipefail -O nullglob
dependencies_run=(
  obs-studio
  xorg-x11-server-Xvfb
)
dnf install -y "${dependencies_run[@]}" && dnf clean all && rm -rf /var/cache/dnf
EOT

RUN <<'EOT' bash -euo pipefail -O nullglob
groupadd -g 1000 vscode
useradd -u 1000 -g vscode -m vscode -s /bin/bash
EOT

COPY --chmod=400 <<'EOF' /etc/sudoers.d/vscode
vscode ALL=(ALL) NOPASSWD: ALL
EOF

CMD ["/bin/bash"]

USER vscode
