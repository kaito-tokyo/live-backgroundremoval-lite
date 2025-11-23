# Live Background Removal Lite

**A high-performance, crash-resistant OBS plugin for real-time background removal.**

This plugin allows you to remove the background from your video source without a physical green screen. It is a **complete rewrite** of the popular `obs-backgroundremoval` tool, engineered from scratch to prioritize **stability**, **memory safety**, and **low resource usage**.

[**⬇️ Download Latest Release**](https://kaito-tokyo.github.io/live-backgroundremoval-lite/)

---

## ⚡ Why "Lite"? (Engineering Philosophy)

While inspired by existing tools, **Live Background Removal Lite** was developed with a specific goal: **To eliminate the crashes and instability often associated with AI plugins.**

* **🛡️ Zero-Crash Stability:**
    The core logic has been rewritten using **Modern C++ practices** (RAII, smart pointers) to ensure robust memory management. We have rigorously engineered the plugin to prevent segmentation faults and memory leaks, ensuring it **never crashes your OBS Studio**, even during long streams.

* **🚀 Optimized Pipeline:**
    By stripping away legacy overhead and optimizing the data flow between the CPU/GPU and OBS, this plugin achieves lower latency and higher FPS compared to other implementations using the same AI models.

* **💻 Lightweight & Native:**
    No external "virtual camera" software is required. It runs natively within OBS, keeping your system load minimal—perfect for laptops or setups without high-end RTX GPUs.

---

## 📸 Demo

<div align="center">

<img src="docs/public/demo.gif" alt="Background Removal Lite Demo" width="480" style="border:1px solid #ccc; border-radius:8px; box-shadow:0 2px 8px #0002;">

<p style="margin-top:8px; color:#666; font-size:90%;">
  <em>
    The plugin seamlessly removes a cluttered background, revealing the source behind it.
  </em>
</p>

</div>

---

## 🌟 Features

* **Rock-Solid Stability:** Built with a focus on exception handling and thread safety. If an error occurs in the inference engine, the plugin handles it gracefully without taking down the entire application.
* **No Green Screen Required:** High-quality background removal using AI, without the need for a physical chroma key setup.
* **Simple & Easy to Use:** Just add it as a filter to your source. No complex configuration or external dependencies needed.
* **Cross-Platform Support:** Works reliably across Windows, macOS, and Linux.

## 💻 Installation

1.  Go to the [Releases page](https://github.com/kaito-tokyo/live-backgroundremoval-lite/releases) and download the appropriate file for your operating system.

2.  Follow the instructions for your OS:
    * **For Windows:**
        * Download the `.zip` file and extract its contents into your OBS Studio installation directory (e.g., `C:\Program Files\obs-studio`).
    * **For macOS:**
        * Download and run the `.pkg` installer.
    * **For Linux:**
        * **Ubuntu:** Download and install the provided `.deb` package.
        * **Arch Linux:** Please refer to the instructions in [`unsupported/arch/`](./unsupported/arch#readme).
        * **Flatpak:** Please refer to the instructions in [`unsupported/flatpak/`](./unsupported/flatpak#readme).
        * **Other Distributions:** Users of other distributions will need to build from source. Note: Converting the `.deb` package is known not to work.

3.  Restart OBS Studio after the installation is complete.

## 🚀 Usage

1.  In OBS Studio, right-click on the video source (e.g., your webcam) you want to apply the effect to.
2.  Select **"Filters"** from the context menu.
3.  Click the **"+"** button at the bottom left of the Filters window and select **"Live Background Removal Lite"** from the list.
4.  Adjust the settings as needed.

## 🙏 Acknowledgements

This project is built upon and incorporates several open-source components. We are grateful to the developers and contributors of these projects.

<details>
<summary>List of Open Source Components</summary>

* **backward-cpp** (License: [MIT](https://github.com/bombela/backward-cpp/blob/master/LICENSE))
* **cURL** (License: [curl](https://curl.se/docs/copyright.html))
* **fmt** (License: [MIT](https://github.com/fmtlib/fmt/blob/master/LICENSE.rst))
* **GoogleTest** (License: [BSD-3-Clause](https://github.com/google/googletest/blob/main/LICENSE))
* **MediaPipe Selfie Segmentation** (License: [Apache-2.0](https://opensource.org/licenses/Apache-2.0))
    * Source: [huggingface.co/onnx-community/mediapipe_selfie_segmentation](https://huggingface.co/onnx-community/mediapipe_selfie_segmentation)
* **ncnn** (License: [BSD-3-Clause](https://github.com/Tencent/ncnn/blob/master/LICENSE.txt))
* **OBS Studio** (License: [GPL-2.0](https://github.com/obsproject/obs-studio/blob/master/COPYING))
* **OpenCV** (License: [Apache-2.0](https://github.com/opencv/opencv/blob/master/LICENSE))
    * Source: [github.com/opencv/opencv](https://github.com/opencv/opencv)
* **wolfSSL** (License: [GPL-3.0](https://github.com/wolfSSL/wolfssl/blob/master/COPYING))

</details>

## 🐞 Bug Reports

If you find a bug or issue, please report it on the [GitHub Issues page](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues).

## 📜 License

This plugin itself is licensed under the [GPL-3.0-or-later](LICENSE).