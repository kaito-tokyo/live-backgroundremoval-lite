# Unsupported Build Instructions for Fedora

## ⚠️ Important Notice: Manual Build Required

This plugin is **not officially available as a pre-built package for Fedora**. Users will need to build and install the plugin from source using the instructions provided below.

## Building from This Repository (Manual Installation)

If you wish to proceed with building the plugin from these local files, you will need the standard Fedora build tools.

### Prerequisites

Install the required build dependencies:

```bash
sudo dnf install @development-tools ninja-build rpm-build obs-studio-devel qt6-qtbase-private-devel ncnn-devel fmt-devel libcurl-devel
```

### Build and Install Steps

1.  Clone the repository:

    ```bash
    git clone https://github.com/kaito-tokyo/live-backgroundremoval-lite.git
    cd live-backgroundremoval-lite
    ```

2.  Configure the project:

    ```bash
    cmake \
     -B build -S . \
     -DCMAKE_BUILD_TYPE=RelWithDebInfo \
     -DCMAKE_INSTALL_PREFIX=/usr \
     -DUSE_PKGCONFIG=ON \
     -DBUILD_TESTING=OFF \
     -GNinja
    ```

3.  Build the project:

    ```bash
    cmake --build build
    ```

4.  Package the plugin:

    ```bash
    cpack --config build/CPackConfig.cmake -G RPM
    ```

5.  Install the plugin:

    ```bash
    sudo dnf install release/*.rpm
    ```

6.  Restart OBS Studio to load the plugin.

## Verifying the Installation

After restarting OBS Studio:

1. Right-click on a video source (e.g., your webcam)
2. Select **"Effect Filters"** from the context menu
3. Click the **"+"** button and look for **"Live Background Removal Lite"**

If the plugin appears in the list, the installation was successful!

## Uninstalling

To remove the plugin:

```bash
sudo dnf remove live-backgroundremoval-lite
```
