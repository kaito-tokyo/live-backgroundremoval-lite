# Live Background Removal Lite

**A high-performance, crash-resistant OBS plugin for real-time background removal.**

This plugin allows you to remove the background from your video source without a physical green screen. It is a **complete rewrite** of the popular `obs-backgroundremoval` tool, engineered from scratch to prioritize **stability**, **compatibility**, and **smart resource management**.

[**⬇️ Download Latest Release**](https://kaito-tokyo.github.io/live-backgroundremoval-lite/)

---

## 🧠 Engineering Philosophy: The "Smart Hybrid" Approach

Most AI plugins rely heavily on the GPU, often causing frame drops in games or compatibility issues with drivers. **Live Background Removal Lite** challenges this status quo with a unique architecture designed to maximize your computer's total potential.

### 1. ⚡ Optimized CPU Inference (Breaking the Myth)
There is a common misconception that "AI must run on the GPU." We aim to prove that **highly optimized CPU inference** is not only viable but superior for multitasking.
* By offloading the AI inference to the CPU using a tuned implementation, we leave your **GPU resources completely free for gaming and rendering**, ensuring your stream remains buttery smooth.

### 2. 🎨 Native GPU Post-Processing
We are not "anti-GPU." Instead, we use it where it matters most: **Image Processing**.
* The plugin leverages OBS's native GPU capabilities for scaling, masking, and compositing.
* Because we use standard OBS GPU calls rather than custom AI CUDA/Tensor kernels, there are **zero compatibility issues** or driver conflicts.

### 3. 🛡️ Zero-Crash Stability
The core logic utilizes **Modern C++ practices** (RAII, smart pointers) to ensure robust memory management. We have rigorously engineered the plugin to prevent segmentation faults, ensuring it **never crashes your OBS Studio**, even during long streams.

---

## 🛠️ Under the Hood: The Hybrid Pipeline

To achieve maximum performance, **Live Background Removal Lite** employs a sophisticated **CPU/GPU Hybrid Pipeline**. Every step is mathematically optimized to ensure zero waste.

### 1. ⚡ Smart Motion Detection (GPU)
* **Technique:** PSNR-based Change Detection via OBS GPU
* Before running any AI, the plugin uses the GPU to check if the frame has actually changed (technically similar to calculating Peak Signal-to-Noise Ratio).
* **Benefit:** If you aren't moving, the plugin skips the heavy calculations entirely. This drastically reduces power consumption and heat.

### 2. 🧠 Constant-Cost AI Inference (CPU)
* **Technique:** `ncnn` + Google MediaPipe Model (Fixed @ 256x144)
* The core background segmentation runs on the CPU, but with a twist: the input is always resized to a tiny **256x144** resolution before inference.
* **Benefit:** This ensures the CPU load remains **low and constant**, regardless of whether your camera is 720p or 4K. The processing time is deterministic and lightning-fast.

### 3. 🎨 Fast Guided Filter Upscaling (GPU)
* **Technique:** Custom Pixel Shader (Fast Guided Filter Implementation)
* To turn the low-res (256x144) mask back into a crisp, full-resolution image, we utilize the **Fast Guided Filter** [2]—an accelerated variant of the original Guided Filter [1].
* **Benefit:** This algorithm performs computations on a subsampled grid, drastically reducing the number of pixels to process. Combined with a **Separable (Horizontal/Vertical) implementation**, this achieves theoretical optimality in computational complexity, delivering high-quality edge preservation with virtually zero GPU overhead.
    > *We consciously utilize published, non-patented algorithms to ensure complete legal safety and transparency.*

### 4. 🌊 Temporal Smoothing (GPU)
* **Technique:** Minimum Group Delay Averaging
* Finally, a time-average filter is applied to suppress flicker.
* **Benefit:** This isn't just a simple blur; the algorithm is tuned to **minimize group delay**. This means the mask adapts to your movements instantly (no "ghosting" or lag) while effectively filtering out high-frequency noise.

#### 📚 References
<small>
[1] He, Kaiming, Jian Sun, and Xiaoou Tang. “Guided Image Filtering.” IEEE Transactions on Pattern Analysis and Machine Intelligence 35, no. 6 (June 2013): 1397–1409. https://doi.org/10.1109/TPAMI.2012.213.

[2] He, Kaiming, and Jian Sun. "Fast Guided Filter." arXiv preprint arXiv:1505.00996 (2015). https://doi.org/10.48550/arXiv.1505.00996
</small>

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
