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

### 1. Install vcpkg

We use vcpkg to manage dependencies. If you already have vcpkg installed, you can skip this step.

```bash
( cd ~ && git clone https://github.com/microsoft/vcpkg.git )
~/vcpkg/bootstrap-vcpkg.sh
```

After installation, you need to set the `VCPKG_ROOT` environment variable to the path where you cloned vcpkg. For example:

```bash
export VCPKG_ROOT=~/vcpkg
```

*Note: We recommend cloning vcpkg to a common location, for example, your home directory (`~/vcpkg`). You may also want to add the `export` command to your shell's startup file (e.g., `.bashrc`, `.zshrc`) to set the environment variable automatically.*

### 2. Install Dependencies

Next, install the necessary packages for development.

**For Ubuntu / Debian-based distributions:**

```bash
sudo add-apt-repository ppa:obsproject/obs-studio
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
*(Note: This package list is untested. We welcome feedback and corrections!)*

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

### 3. Clone the Repository

Clone the source code to your local machine.

```bash
git clone https://github.com/kaito-tokyo/obs-backgroundremoval-lite.git
cd obs-backgroundremoval-lite
```

### 4. Build and Install

We use CMake presets for building. The preset name contains `ubuntu`, but it should work on other modern Linux distributions as well.

From the root directory of the repository, run the following commands in order:

```bash
# 1. Configure the build
cmake --preset ubuntu-x86_64

# 2. Build the project
cmake --build --preset ubuntu-x86_64

# 3. Install the plugin
rm -rf ~/.config/obs-studio/plugins/obs-backgroundremoval-lite
mkdir -p ~/.config/obs-studio/plugins/obs-backgroundremoval-lite/bin/64bit
cp build_x86_64/obs-backgroundremoval-lite.so ~/.config/obs-studio/plugins/obs-backgroundremoval-lite/bin/64bit
cp -r data ~/.config/obs-studio/plugins/obs-backgroundremoval-lite/data
```

### 5. Verify the Installation

Once the installation is complete, launch OBS Studio. The plugin should now be available.
If you encounter any issues, please feel free to report them by opening an issue.

## Building from Source on macOS

Here's how to build the project from source on macOS.

### 1. Install Dependencies

First, install Xcode Command Line Tools and Homebrew, then install the necessary packages. A developer build of OBS Studio should be installed separately.

```bash
# Install Xcode Command Line Tools
xcode-select --install

# Install Homebrew (if you don't have it)
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install packages
brew install \
  cmake \
  git \
  jq \
  ninja \
  pkg-config \
  qt6
```

### 2. Install vcpkg

We use vcpkg to manage dependencies. If you already have vcpkg installed, you can skip this step.

```bash
( cd ~ && git clone https://github.com/microsoft/vcpkg.git )
~/vcpkg/bootstrap-vcpkg.sh
```

After installation, you need to set the `VCPKG_ROOT` environment variable to the path where you cloned vcpkg. For example:

```bash
export VCPKG_ROOT=~/vcpkg
```

*Note: We recommend cloning vcpkg to a common location, for example, your home directory (`~/vcpkg`). You may also want to add the `export` command to your shell's startup file (e.g., `.zshrc`) to set the environment variable automatically.*

### 3. Clone the Repository

Clone the source code to your local machine.

```bash
git clone https://github.com/kaito-tokyo/obs-backgroundremoval-lite.git
cd obs-backgroundremoval-lite
```

### 4. Build and Manually Install

We use the `macos` CMake preset, which automatically creates a universal binary for both Apple Silicon and Intel Macs.

**1. Build the project**
From the root directory of the repository, run the following commands in order:

```bash
# Configure the build
cmake --preset macos

# Build the project
cmake --build --preset macos
```

**2. Manually install the plugin**
After the build is complete, you need to manually copy the plugin to the OBS Studio plugins directory. The built plugin (e.g., `obs-backgroundremoval-lite.plugin`) will be located in the build directory.

Copy the plugin to: `~/Library/Application Support/obs-studio/plugins/`

### 5. Verify the Installation

Once you have copied the plugin, launch OBS Studio. The plugin should now be available.
If you encounter any issues, please feel free to report them by opening an issue.