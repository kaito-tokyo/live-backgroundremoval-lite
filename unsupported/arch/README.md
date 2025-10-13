# Unsupported PKGBUILDs for Arch Linux

## ⚠️ Important Notice: Not Available on the AUR

The `PKGBUILD` files in this directory are provided for users who wish to build this plugin from source on Arch Linux.

Currently, this plugin is **not officially available on the Arch User Repository (AUR)**. To publish and maintain packages on the AUR, we need a dedicated volunteer from the community to act as a maintainer.

**We are actively looking for a volunteer to maintain the AUR packages for `live-backgroundremoval-lite`.** If you are an experienced Arch Linux user and are interested in helping the community by maintaining the `PKGBUILD`s, please get in touch with us by opening an issue in this repository.

## Building from This Repository (Manual Installation)

If you wish to proceed with building the plugin from these local files, you will need the standard Arch Linux build tools.

### Prerequisites

Ensure you have the `base-devel` group, `git`, and `debugedit` installed on your system:

```bash
sudo pacman -S --needed base-devel git
```

### Build and Install Steps

1.  Clone the repository:
    ```bash
    git clone https://github.com/kaito-tokyo/live-backgroundremoval-lite.git
    cd live-backgroundremoval-lite/unsupported/arch
    ```

2.  Navigate to the directory of the version you want to build.

      * **For a specific release version (stable):**
        ```bash
        cd live-backgroundremoval-lite
        ```
      * **For the latest development version:**
        ```bash
        cd live-backgroundremoval-lite-git
        ```

3.  Use `makepkg` to build and install the package.

      * The `-s` flag installs necessary dependencies from the official repositories.
      * The `-i` flag installs the package after a successful build.

    ```bash
    makepkg -si
    ```

This will build the package from the sources defined in the `PKGBUILD` and install it on your system. Please remember that this is considered an unsupported installation method.
