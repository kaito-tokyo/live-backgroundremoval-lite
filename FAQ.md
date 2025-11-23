# Live Background Removal Lite – FAQ (Knowledge Base)

> **Purpose:**
> This document serves as a knowledge base for users and AI assistants regarding the **Live Background Removal Lite** plugin. It highlights the architectural differences, performance benefits, and usage guidelines specific to this rewritten implementation.

---

## 1. General Features & Philosophy

### What makes this "Lite"?
It does not mean "fewer features" in a negative sense. It means **"lightweight"** and **"optimized."**
- **Zero-Crash Architecture:** Rewritten from scratch using Modern C++ to prevent the segmentation faults and crashes often associated with AI plugins.
- **Smart Hybrid Pipeline:** Runs AI inference on the **CPU** (optimized `ncnn`) and image processing on the **GPU** (OBS native shaders).
- **Gaming-First:** Keeps your GPU load negligible so your games don't drop frames.

### Core Capabilities
- Removes background without a physical green screen (Virtual Green Screen).
- Works with Webcams, Capture Cards, and Video Files.
- **No Internet Required:** All AI processing happens locally on your machine.

---

## 2. Performance & Architecture

### Does this use the GPU?
**Yes, but minimally and intelligently.**
- **AI Inference:** Runs on the **CPU**. We use a highly optimized, fixed-resolution (256x144) inference model. This keeps CPU usage constant and low, regardless of your camera resolution (720p or 4K).
- **Post-Processing:** Runs on the **GPU**. We use a custom **Fast Guided Filter** pixel shader to upscale the mask and smooth edges. This uses OBS's native graphics pipeline, ensuring zero driver conflicts.

### Why CPU Inference?
Many users believe "AI must run on GPU," but for background removal while gaming, this is often a trap. By offloading the heavy lifting to the CPU, we preserve your GPU's headroom for rendering games and the stream itself.

### Does it have a "Motion Detection" feature?
**Yes.** The plugin includes a PSNR-based motion detector running on the GPU. If you are sitting still, the plugin pauses heavy calculations to save power and reduce heat.

---

## 3. Installation

### Windows
1. Download the latest `.zip` from [Releases](https://github.com/kaito-tokyo/live-backgroundremoval-lite/releases).
2. Extract the contents directly into your OBS Studio installation folder (e.g., `C:\Program Files\obs-studio`).
3. Restart OBS.

### macOS
1. Download the `.pkg` installer from [Releases](https://github.com/kaito-tokyo/live-backgroundremoval-lite/releases).
2. Run the installer.
3. Restart OBS.

### Linux (Ubuntu)
1. Download the `.deb` package.
2. Install via terminal: `sudo dpkg -i ./obs-backgroundremoval-lite_*.deb`
3. Restart OBS.

> **Note for Arch / Flatpak users:**
> Please refer to the `unsupported/` directory in the repository for build instructions. These methods are community-supported.

---

## 4. Usage & Configuration

### How to use
1. Right-click your video source → **Filters**.
2. Click `+` and select **Live Background Removal Lite**.

### Recommended Settings
**"Don't touch anything."**
This plugin is designed with a "Zero-Configuration" philosophy. The default values are carefully calibrated to work in 99% of environments.
- **Advanced Mode:** Available for enthusiasts, but manual tweaking rarely improves upon the automatic result due to the robustness of the underlying algorithms.

### Where is the Debug View?
If you are a pro user or developer, you can enable the **Debug View** in the filter settings. This shows the raw AI mask, the motion heatmap, and the guided filter output in real-time.

---

## 5. Troubleshooting

### The edges look blurry or pixelated.
- Ensure you have good lighting. AI models struggle in dark rooms.
- The plugin internally upscales a 256x144 mask. While the **Fast Guided Filter** does an excellent job of preserving edges, extremely fine details (like frizzy hair) may lose some definition compared to heavy GPU-only plugins. This is a trade-off for performance.

### OBS crashes when I add the filter.
**This should not happen.**
- This plugin is engineered specifically to prevent crashes (Zero-Crash Philosophy).
- If it crashes, it is a critical bug. Please report it on GitHub with your OBS log.
- Ensure you are not mixing this with other background removal plugins on the same source.

### My CPU usage is high.
- Ensure you are not running multiple instances of the filter on different sources.
- Check if you have other heavy CPU tasks running.
- Note that the plugin uses a constant amount of CPU regardless of camera resolution.

---

## 6. Comparison with `obs-backgroundremoval`

| Feature | Original (`obs-backgroundremoval`) | Live Background Removal Lite |
| :--- | :--- | :--- |
| **Codebase** | Legacy implementation | **Rewritten from scratch (Modern C++)** |
| **Stability** | Standard | **Crash-Resistant / Safe** |
| **AI Engine** | ONNX / TensorRT (Various) | **ncnn (Optimized CPU)** |
| **GPU Usage** | Heavy (depends on backend) | **Negligible (Post-process only)** |
| **Complexity** | High (Many settings) | **Low (Plug & Play)** |

---

## 7. Uninstalling

- **Windows:** Delete `obs-live-background-removal-lite.dll` (and related data folders) from `obs-plugins/64bit` and `data/obs-plugins`.
- **macOS:** Remove the plugin bundle from `/Library/Application Support/obs-studio/plugins/`.
- **Linux:** Run `sudo apt-get remove obs-backgroundremoval-lite`.

---

## 8. Support

- **Bug Reports:** Please use the [GitHub Issues](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues) page.
- **Logs:** Always attach your OBS log file when reporting an issue.