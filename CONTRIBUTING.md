# Contributing

Thank you for your interest in contributing to this project! 🎉

We welcome all kinds of contributions, including bug reports, feature requests, documentation improvements, and pull requests.

## Reporting Bugs or Requesting Features

The best way to report a bug or request a feature is to open an issue on our [GitHub Issues](https://www.google.com/search?q=https://github.com/kaito-tokyo/obs-backgroundremoval-lite/issues).

Before creating a new issue, please check if a similar one already exists. When reporting a bug, please include as much detail as possible, such as your OS version, OBS Studio version, the plugin version, steps to reproduce, and the expected vs. actual behavior.

## Submitting Pull Requests

1.  **Fork** this repository.
2.  Clone it to your local machine (`git clone https://github.com/your-username/your-repository.git`).
3.  Create a new branch (`git checkout -b feature/my-amazing-feature`).
4.  Make your changes and commit them with a clear message (`git commit -m 'Add some amazing feature'`).
5.  Push your branch (`git push origin feature/my-amazing-feature`).
6.  Open a **Pull Request**.

## Building from Source on Linux

Here's how to build the project from source on Linux.

### 1. Install Dependencies

First, install the necessary packages for development.

**For Ubuntu / Debian-based distributions:**

```bash
sudo apt update
sudo apt install \
  build-essential \
  cmake \
  git \
  jq \
  libgles2-mesa-dev \
  libqt6svg6-dev \
  ninja-build \
  obs-studio \
  pkg-config \
  qt6-base-dev \
  qt6-base-private-dev
```

**For Arch Linux:**
*(Note: This package list is untested. We welcome feedback and corrections\!)*

```bash
sudo pacman -S \
  base-devel \
  cmake \
  git \
  jq \
  mesa \
  ninja \
  obs-studio \
  pkg-config \
  qt6-base \
  qt6-svg
```

### 2. Clone the Repository

Clone the source code to your local machine.

```bash
git clone https://github.com/kaito-tokyo/obs-backgroundremoval-lite.git
cd obs-backgroundremoval-lite
```

### 3. Build and Install

We use CMake presets for building. The preset name contains `ubuntu`, but it should work on other modern Linux distributions as well.

From the root directory of the repository, run the following commands in order:

```bash
# 1. Configure the build
cmake --preset ubuntu-x86_64

# 2. Build the project
cmake --build --preset ubuntu-x86_64

# 3. Install the plugin
sudo cmake --install --preset ubuntu-x86_64
```

### 4. Verify the Installation

Once the installation is complete, launch OBS Studio. The plugin should now be available.
If you encounter any issues, please feel free to report them by opening an issue.
