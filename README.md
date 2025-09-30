# Live Background Removal Lite

An OBS plugin that allows you to remove the background from your video source in real-time, without the need for a green screen.

[**⬇️ Download Latest Release**](https://live-backgroundremoval-lite.kaito.tokyo/)

---

## 🌟 Features

* **No Green Screen Required:** Removes the background from your webcam feed without needing a physical green screen.
* **Simple & Easy to Use:** Just add it as a filter to your source, and you're ready to go. No complex configuration is needed.
* **Lightweight Performance:** Designed to be light on system resources, ensuring smooth streaming and recording.

## 💻 Installation

1.  Go to the [Releases page](https://live-backgroundremoval-lite.kaito.tokyo/) and download the appropriate file for your operating system.

2.  Follow the instructions for your OS:
    * **For Windows:**
        * Download the `.zip` file and extract its contents into your OBS Studio installation directory (e.g., `C:\Program Files\obs-studio`).
    * **For macOS:**
        * Download and run the `.pkg` installer.
    * **For Linux:**
        * **Ubuntu:** Download and install the provided `.deb` package.
        * **Arch Linux:** Please refer to the instructions in [`unsupported/arch/`](./unsupported/arch#readme) for installation.
        * **Flatpak:** Please refer to the instructions in [`unsupported/flatpak/`](./unsupported/flatpak#readme) for building.
        * **Other Distributions:** Users of other distributions (like Debian, etc.) will need to build the plugin from source. Please note that converting the `.deb` package is known not to work.

3.  Restart OBS Studio after the installation is complete.

## 🚀 Usage

1.  In OBS Studio, right-click on the video source (e.g., your webcam) you want to apply the effect to.
2.  Select **"Filters"** from the context menu.
3.  Click the **"+"** button at the bottom left of the Filters window and select **"Background Removal Lite"** from the list.
4.  Adjust the settings as needed.

## 🙏 Acknowledgements

This project is built upon and incorporates several open-source components. We are grateful to the developers and contributors of these projects.

<details>
<summary>List of Open Source Components</summary>

* **OBS Studio**
    * License: [GPL-2.0](https://github.com/obsproject/obs-studio/blob/master/COPYING)
* **MediaPipe Selfie Segmentation**
    * License: [Apache-2.0](https://opensource.org/licenses/Apache-2.0)
    * Source: [huggingface.co/onnx-community/mediapipe_selfie_segmentation](https://huggingface.co/onnx-community/mediapipe_selfie_segmentation)
* **ncnn**
    * License: [BSD-3-Clause](https://github.com/Tencent/ncnn/blob/master/LICENSE.txt)
* **GoogleTest**
    * License: [BSD-3-Clause](https://github.com/google/googletest/blob/main/LICENSE)
* **wolfSSL**
    * License: [GPL-2.0](https://github.com/wolfSSL/wolfssl/blob/v5.8.0-stable/COPYING)
* **cURL**
    * License: [curl](https://curl.se/docs/copyright.html)
* **cpr**
    * License: [MIT](https://github.com/libcpr/cpr/blob/master/LICENSE)
* **semver**
    * License: [MIT](https://github.com/Neargye/semver/blob/master/LICENSE)

</details>

## 📜 License

This plugin itself is licensed under the [GPL-2.0](LICENSE).