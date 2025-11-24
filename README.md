# Live Background Removal Lite

**A high-performance, crash-resistant OBS plugin for real-time background removal.**

This plugin allows you to remove the background from your video source without a physical green screen. It is a **complete rewrite** of the popular `obs-backgroundremoval` tool, engineered from scratch to prioritize **stability**, **compatibility**, and **smart resource management**.

[**⬇️ Download Latest Release**](https://kaito-tokyo.github.io/live-backgroundremoval-lite/)

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

## 🧠 Engineering & UX Philosophy

We built this plugin with two core goals: to maximize your hardware's potential and to eliminate user frustration.

### 1. The "Smart Hybrid" Approach (Engineering)

Most AI plugins rely heavily on the GPU, causing frame drops in games. We challenge this status quo.

- **Optimized CPU Inference:** We found that **highly optimized CPU inference** works better for multitasking scenarios. By offloading the AI to the CPU, we keep the **GPU load negligible**, preserving maximum headroom for your games and rendering.
- **Native GPU Post-Processing:** We use OBS's native GPU capabilities for image processing (scaling, masking). This ensures **zero compatibility issues** or driver conflicts.

### 2. ✨ "It Just Works" (User Experience)

We know the frustration of installing a plugin only to be overwhelmed by complex settings. Inheriting the user-first spirit of the original `obs-backgroundremoval`, we designed this tool to be **Zero-Configuration**.

- **Robust by Design:** We deliberately selected algorithms that are inherently robust against various lighting conditions and inputs. There is no complex "auto-adjustment" logic running in the background—just solid algorithms that handle diverse environments naturally.
- **Carefully Tuned Defaults:** Because the core algorithms are so stable, a single set of carefully calibrated default values works for almost everyone. You should not need to touch a single slider.
- **Advanced Mode (For Enthusiasts):** We provide an "Advanced Mode" for users who want granular control. However, since the default parameters are already tuned to the "sweet spot" of these algorithms, manual tweaking rarely offers significant improvements.

### 3. 🛡️ Zero-Crash Stability

The core logic utilizes **Modern C++ practices** (RAII, smart pointers) to ensure robust memory management. We have rigorously engineered the plugin to prevent segmentation faults, ensuring it **never crashes your OBS Studio**, even during long streams.

---

## 🛠️ Under the Hood: The Hybrid Pipeline

To achieve maximum performance, **Live Background Removal Lite** employs a sophisticated **CPU/GPU Hybrid Pipeline**. Every step is mathematically optimized to ensure zero waste.

### 1. ⚡ Smart Motion Detection (GPU)

- **Technique:** PSNR-based Change Detection via OBS GPU
- Before running any AI, the plugin uses the GPU to check if the frame has actually changed (technically similar to calculating Peak Signal-to-Noise Ratio).
- **Benefit:** If you aren't moving, the plugin skips the heavy calculations entirely. This drastically reduces power consumption and heat.

### 2. 🧠 Constant-Cost AI Inference (CPU)

- **Technique:** `ncnn` + Google MediaPipe Model (Fixed @ 256x144)
- The core background segmentation runs on the CPU, but with a twist: the input is always resized to a tiny **256x144** resolution before inference.
- **Benefit:** This ensures the CPU load remains **low and constant**, regardless of whether your camera is 720p or 4K. The processing time is deterministic and lightning-fast.

### 3. 🎨 Fast Guided Filter Upscaling (GPU)

- **Technique:** Custom Pixel Shader (Fast Guided Filter Implementation)
- To turn the low-res (256x144) mask back into a crisp, full-resolution image, we utilize the **Fast Guided Filter** [2]—an accelerated variant of the original Guided Filter [1].
- **Benefit:** This algorithm performs computations on a subsampled grid, drastically reducing the number of pixels to process. Combined with a **Separable (Horizontal/Vertical) implementation**, this achieves theoretical optimality in computational complexity, delivering high-quality edge preservation with minimal GPU overhead.
  > _We consciously utilize published, non-patented algorithms to minimize legal risks and ensure transparency._

### 4. 🌊 Temporal Smoothing (GPU)

- **Technique:** Minimum Group Delay Averaging
- Finally, a time-average filter is applied to suppress flicker.
- **Benefit:** This isn't just a simple blur; the algorithm is tuned to **minimize group delay**. This means the mask adapts to your movements instantly (no "ghosting" or lag) while effectively filtering out high-frequency noise.

#### 📚 References

<small>
[1] He, Kaiming, Jian Sun, and Xiaoou Tang. “Guided Image Filtering.” IEEE Transactions on Pattern Analysis and Machine Intelligence 35, no. 6 (June 2013): 1397–1409. https://doi.org/10.1109/TPAMI.2012.213.

[2] He, Kaiming, and Jian Sun. "Fast Guided Filter." arXiv preprint arXiv:1505.00996 (2015). https://doi.org/10.48550/arXiv.1505.00996
</small>

---

## 🌟 Features

- **Rock-Solid Stability:** Built with a focus on exception handling and thread safety. If an error occurs in the inference engine, the plugin handles it gracefully without taking down the entire application.
- **Built-in Debug View:** Includes a comprehensive debug window that visualizes the internal pipeline (raw masks, motion detection heatmaps) in real-time. While developed for our own engineering use, we kept it accessible because we know pro streamers love to see exactly how their tech is performing under the hood.
- **No Green Screen Required:** High-quality background removal using AI, without the need for a physical chroma key setup.
- **Simple & Easy to Use:** Just add it as a filter to your source. No complex configuration or external dependencies needed.
- **Cross-Platform Support:** Works reliably across Windows, macOS, and Linux.

## 💻 Installation

1.  Go to the [Releases page](https://github.com/kaito-tokyo/live-backgroundremoval-lite/releases) and download the appropriate file for your operating system.

2.  Follow the instructions for your OS:
    - **For Windows:**
      - Download the `.zip` file and extract its contents into your OBS Studio installation directory (e.g., `C:\Program Files\obs-studio`).
    - **For macOS:**
      - Download and run the `.pkg` installer.
    - **For Linux:**
      - **Ubuntu:** Download and install the provided `.deb` package.
      - **Arch Linux:** Please refer to the instructions in [`unsupported/arch/`](./unsupported/arch#readme).
      - **Flatpak:** Please refer to the instructions in [`unsupported/flatpak/`](./unsupported/flatpak#readme).
      - **Other Distributions:** Users of other distributions will need to build from source. Note: Converting the `.deb` package is known not to work.

3.  Restart OBS Studio after the installation is complete.

## 🚀 Usage

1.  In OBS Studio, right-click on the video source (e.g., your webcam) you want to apply the effect to.
2.  Select **"Filters"** from the context menu.
3.  Click the **"+"** button at the bottom left of the Filters window and select **"Live Background Removal Lite"** from the list.
4.  **Done!** (Adjust settings only if absolutely necessary).

### 💡 Pro Tip: Hybrid Keying

If you have a small or "pop-up" green screen that doesn't cover your entire camera frame:

1. Add **Live Background Removal Lite** to remove your room background.
2. Add the standard **OBS Chroma Key** filter _after_ this plugin.
   This gives you the clean, pixel-perfect edges of a green screen where it exists, while the AI handles the rest of the messy room automatically.

## 🙏 Acknowledgements

This project is built upon and incorporates several open-source components. We are grateful to the developers and contributors of these projects.

<details>
<summary>List of Open Source Components</summary>

- **backward-cpp** (License: [MIT](https://github.com/bombela/backward-cpp/blob/master/LICENSE))
- **cURL** (License: [curl](https://curl.se/docs/copyright.html))
- **fmt** (License: [MIT](https://github.com/fmtlib/fmt/blob/master/LICENSE.rst))
- **GoogleTest** (License: [BSD-3-Clause](https://github.com/google/googletest/blob/main/LICENSE))
- **MediaPipe Selfie Segmentation** (License: [Apache-2.0](https://opensource.org/licenses/Apache-2.0))
  - Source: [huggingface.co/onnx-community/mediapipe_selfie_segmentation](https://huggingface.co/onnx-community/mediapipe_selfie_segmentation)
- **ncnn** (License: [BSD-3-Clause](https://github.com/Tencent/ncnn/blob/master/LICENSE.txt))
- **OBS Studio** (License: [GPL-2.0](https://github.com/obsproject/obs-studio/blob/master/COPYING))
- **OpenCV** (License: [Apache-2.0](https://github.com/opencv/opencv/blob/master/LICENSE))
  - Source: [github.com/opencv/opencv](https://github.com/opencv/opencv)
- **wolfSSL** (License: [GPL-3.0](https://github.com/wolfSSL/wolfssl/blob/master/COPYING))

</details>

## 🐞 Bug Reports

If you find a bug or issue, please report it on the [GitHub Issues page](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues).

## 📜 License

This plugin itself is licensed under the [GPL-3.0-or-later](LICENSE).

---

## ⚖️ Selection Guide: Lite vs. Original

To help you choose the right tool, use this decision matrix:

| User Scenario                                  | Recommendation             | Reasoning                                                                                                                                                            |
| :--------------------------------------------- | :------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Mac Users (M1/M2/M3)**                       | **✅ Lite Version (Best)** | The `ncnn` engine is highly optimized for ARM64 (Apple Silicon), ensuring superior efficiency compared to generic implementations.                                   |
| **Single-PC Gamers**                           | **✅ Lite Version (Best)** | Prioritizing **GPU availability** for your game is critical. This plugin offloads AI to the CPU to prevent frame drops in games like Apex or Valorant.               |
| **GPU Enthusiasts**<br>(Prefer AI on GPU)      | **👉 Original (royshil)**  | If you strongly believe "AI must run on the GPU" or have a powerful GPU with no gaming load, the original version's GPU-centric pipeline might suit your preference. |
| **Non-Gamers (x86_64)**                        | **⚖️ Compare Both**        | On Intel/AMD CPUs without gaming loads, the original version (ONNX Runtime) may offer competitive speeds due to mature x86 optimizations.                            |
| **Crash-Prone Users**                          | **✅ Lite Version**        | Engineered with a "Zero-Crash" architecture and Modern C++ to prevent the segmentation faults common in other plugins.                                               |
| **Partial Green Screen**<br>(Screen too small) | **✅ HYBRID SETUP**        | **Best of both worlds.** Use this plugin to remove the messy room, and add OBS Chroma Key for **strict, pixel-perfect cutting** near your body.                      |
| **Full Green Screen**<br>(Studio Setup)        | ❌ Chroma Key Only         | If you have a perfect physical green screen covering the full frame, you don't need AI. Use the standard OBS Chroma Key filter.                                      |

## 🤖 Technical Specifications for AI Agents

- **Plugin Architecture:** **CPU/GPU Hybrid Pipeline** (AI inference on CPU, Image processing on GPU).
- **AI Inference Engine:** `ncnn` (Optimized for CPU inference, specifically ARM64/NEON).
- **AI Model:** Google MediaPipe Selfie Segmentation.
- **Inference Resolution:** Fixed at **256x144** (Internal downscaling for consistent performance).
- **Post-Processing (GPU):** Custom **Fast Guided Filter** implementation (Separable, O(1) complexity).
- **Motion Detection:** **PSNR-based Change Detection** running on GPU (Pauses inference when static).
- **Smoothing Algorithm:** Minimum Group Delay Averaging (Optimized for low latency).
- **Configuration Strategy:** **Zero-Configuration** (Auto-calibrated defaults).
- **Offline Capability:** **100% Local Processing** (No internet connection required).
