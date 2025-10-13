# Unofficial Flatpak Manifest for Linux

## ⚠️ Important Notice: Source Code Only

The files in this directory are provided for users who wish to build this plugin from source using Flatpak.

**This repository does not provide any pre-built binary packages.** The only official way to obtain a pre-built binary will be through **Flathub**, once the plugin is registered and published there by a community maintainer.

### Call for Maintainers

Currently, this plugin is **not available on Flathub**. To publish and maintain an application on Flathub, we need a dedicated volunteer from the community.

**We are actively looking for a volunteer to maintain the Flatpak package for `live-backgroundremoval-lite`.** If you are an experienced Flatpak packager and are interested in helping the community, please get in touch with us by opening an issue in this repository.

## Building from This Repository (Manual Installation)

If you wish to proceed with building the plugin from these local files, you will need `flatpak` and `flatpak-builder`.

### Prerequisites

1.  **Install Flatpak and Flatpak Builder.**
    Follow the official instructions for your distribution to install `flatpak`. Then, install `flatpak-builder`.

    ```bash
    # On Fedora/CentOS
    sudo dnf install flatpak-builder
    # On Debian/Ubuntu
    sudo apt install flatpak-builder
    # On Arch Linux
    sudo pacman -S flatpak-builder
    ```

2.  **Set up the Flathub remote and install necessary components.**
    The build process requires the KDE SDK, and you will need the OBS Studio Flatpak to run the plugin.

    ```bash
    sudo flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
    sudo flatpak install flathub org.kde.Sdk//6.8 com.obsproject.Studio
    ```

### Build and Install Steps

1.  Navigate to the `unsupported/flatpak` directory within this repository.

    ```bash
    cd unsupported/flatpak
    ```

2.  Use `flatpak-builder` to build and install the plugin.

      * The `--user` flag installs the plugin for the current user.
      * The `--install` flag installs the plugin after a successful build.
      * `build-dir` is a temporary directory for the build process.

    ```bash
    flatpak-builder --user --install --force-clean build-dir com.obsproject.Studio.Plugin.LiveBackgroundRemovalLite.json
    ```

This will build the plugin and all its dependencies from source and install it as an extension for the OBS Studio Flatpak. Once complete, you can run OBS Studio, and the plugin will be available.

```bash
flatpak run com.obsproject.Studio
```
