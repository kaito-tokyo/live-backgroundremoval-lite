# Building and Installing on Fedora

## ⚠️ Important Notice: Manual Build Required

This plugin is not available as a pre-built package for Fedora. Users will need to build and install the plugin from source.

## Building from Source

### Prerequisites

Install the required build dependencies:

```bash
# Install build tools and system libraries
sudo dnf install -y \
  cmake \
  ninja-build \
  gcc-c++ \
  git \
  pkg-config

# Install OBS Studio and development libraries
sudo dnf install -y \
  obs-studio \
  obs-studio-devel \
  qt6-qtbase-devel \
  qt6-qtbase-private-devel \
  qt6-qtsvg-devel \
  mesa-libGLES-devel \
  simde-devel
```

### Install vcpkg

This project uses vcpkg to manage C++ dependencies. Install vcpkg globally:

```bash
# Clone vcpkg to your home directory or preferred location
git clone https://github.com/microsoft/vcpkg.git ~/vcpkg
cd ~/vcpkg
./bootstrap-vcpkg.sh

# Set the VCPKG_ROOT environment variable
export VCPKG_ROOT=~/vcpkg
echo 'export VCPKG_ROOT=~/vcpkg' >> ~/.bashrc
```

### Build and Install Steps

1. Clone the repository:
   ```bash
   git clone --recursive https://github.com/kaito-tokyo/live-backgroundremoval-lite.git
   cd live-backgroundremoval-lite
   ```

2. Install C++ dependencies from vcpkg:
   ```bash
   "$VCPKG_ROOT/vcpkg" install --triplet x64-linux-obs
   ```

3. Configure the project:
   ```bash
   cmake --preset ubuntu-x86_64
   ```

4. Build the project:
   ```bash
   cmake --build --preset ubuntu-x86_64
   ```

5. Run tests (optional):
   ```bash
   ctest --preset ubuntu-x86_64 --output-on-failure
   ```

6. Install the plugin:
   ```bash
   sudo cmake --install build_x86_64 --config RelWithDebInfo
   ```

   Alternatively, for a user-local installation:
   ```bash
   cmake --install build_x86_64 --config RelWithDebInfo --prefix ~/.config/obs-studio/plugins
   ```

7. Restart OBS Studio to load the plugin.

## Verifying the Installation

After restarting OBS Studio:

1. Right-click on a video source (e.g., your webcam)
2. Select **"Filters"** from the context menu
3. Click the **"+"** button and look for **"Live Background Removal Lite"**

If the plugin appears in the list, the installation was successful!

## Troubleshooting

### OBS Studio Not Found

If CMake cannot find OBS Studio, ensure `obs-studio-devel` is installed:

```bash
sudo dnf install obs-studio-devel
```

### vcpkg Dependencies Fail to Build

Ensure you have all build dependencies installed. You may need additional development packages:

```bash
sudo dnf install -y libcurl-devel wolfssl-devel
```

### Plugin Not Appearing in OBS

- Verify the plugin was installed to the correct location
- Check OBS Studio logs for any loading errors
- Ensure you restarted OBS Studio after installation

## Uninstalling

To remove the plugin:

**System-wide installation:**
```bash
sudo rm -rf /usr/lib64/obs-plugins/live-backgroundremoval-lite.so
sudo rm -rf /usr/share/obs/obs-plugins/live-backgroundremoval-lite/
```

**User-local installation:**
```bash
rm -rf ~/.config/obs-studio/plugins/live-backgroundremoval-lite.so
rm -rf ~/.config/obs-studio/plugins/live-backgroundremoval-lite/
```

## Community Contributions

We welcome contributions from the Fedora community to improve these build instructions or to help package this plugin for official Fedora repositories. If you're interested in maintaining an RPM package, please open an issue in the repository.
