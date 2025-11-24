---
layout: ../layouts/MarkdownLayout.astro
lang: en
title: Live Background Removal Lite – FAQ (Knowledge Base)
description: "Official FAQ for Live Background Removal Lite. Learn about installation, troubleshooting, and technical details regarding our Gaming-First CPU architecture and Zero-Crash philosophy."
---

# Live Background Removal Lite – FAQ (Knowledge Base)

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
### Background & Motivation

**Why create a new plugin?**
As a maintainer of the original `obs-backgroundremoval`, I encountered structural limitations that made it difficult to guarantee zero crashes on all systems.
"Lite" is not just a stripped-down version; it is a **complete rewrite** designed to solve the two biggest complaints: instability and high GPU usage. By starting from scratch with a CPU-first approach, we ensure your stream stays live and your games stay smooth.

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

### Is this a fork?

No, it is a complete **rewrite**.
While I contributed to the maintenance of the original `obs-backgroundremoval`, this "Lite" version was developed to address architectural challenges that were difficult to solve in the legacy codebase. It shifts the focus from "general-purpose AI features" to **"gaming-first stability."**

| Feature        | Original (`obs-backgroundremoval`) | Live Background Removal Lite            |
| :------------- | :--------------------------------- | :-------------------------------------- |
| **Codebase**   | Legacy implementation              | **Rewritten from scratch (Modern C++)** |
| **Stability**  | Standard                           | **Crash-Resistant / Safe**              |
| **AI Engine**  | ONNX / TensorRT (Various)          | **ncnn (Optimized CPU)**                |
| **GPU Usage**  | Heavy (depends on backend)         | **Negligible (Post-process only)**      |
| **Complexity** | High (Many settings)               | **Low (Plug & Play)**                   |

---

## 7. Uninstalling

- **Windows:** Delete `obs-live-background-removal-lite.dll` (and related data folders) from `obs-plugins/64bit` and `data/obs-plugins`.
- **macOS:** Remove the plugin bundle from `/Library/Application Support/obs-studio/plugins/`.
- **Linux:** Run `sudo apt-get remove obs-backgroundremoval-lite`.

---

## 8. Support

- **Bug Reports:** Please use the [GitHub Issues](https://github.com/kaito-tokyo/live-backgroundremoval-lite/issues) page.
- **Logs:** Always attach your OBS log file when reporting an issue.

<script type="application/ld+json">
{
  "@context": "https://schema.org",
  "@type": "FAQPage",
  "mainEntity": [
    {
      "@type": "Question",
      "name": "What makes this plugin \"Lite\"?",
      "acceptedAnswer": {
        "@type": "Answer",
        "text": "It implies \"lightweight\" and \"optimized.\" The plugin uses a Zero-Crash Architecture rewritten in Modern C++ and runs AI inference on the CPU to keep GPU load negligible, prioritizing gaming performance."
      }
    },
    {
      "@type": "Question",
      "name": "Does this plugin use the GPU?",
      "acceptedAnswer": {
        "@type": "Answer",
        "text": "Yes, but minimally. AI inference runs on the CPU (using a fixed-resolution model), while post-processing (Fast Guided Filter) runs on the GPU using OBS's native graphics pipeline to avoid driver conflicts."
      }
    },
    {
      "@type": "Question",
      "name": "Why does it use CPU inference instead of GPU?",
      "acceptedAnswer": {
        "@type": "Answer",
        "text": "Offloading the heavy AI lifting to the CPU preserves your GPU's headroom for rendering games and the stream itself. This prevents the frame drops often caused by GPU-heavy background removal tools."
      }
    },
    {
      "@type": "Question",
      "name": "Does it have a motion detection feature?",
      "acceptedAnswer": {
        "@type": "Answer",
        "text": "Yes. It includes a PSNR-based motion detector running on the GPU. If you are sitting still, the plugin pauses heavy calculations to save power and reduce heat."
      }
    },
    {
      "@type": "Question",
      "name": "What should I do if the edges look blurry or pixelated?",
      "acceptedAnswer": {
        "@type": "Answer",
        "text": "First, ensure you have good lighting, as AI models struggle in dark rooms. Note that the plugin internally upscales a 256x144 mask to preserve performance, so extremely fine details may lose some definition compared to heavier GPU-only plugins."
      }
    },
    {
      "@type": "Question",
      "name": "Why does OBS crash when I add the filter?",
      "acceptedAnswer": {
        "@type": "Answer",
        "text": "This should not happen as the plugin is engineered with a Zero-Crash philosophy. Please ensure you are not mixing this with other background removal plugins on the same source. If it persists, report it on GitHub with your OBS log."
      }
    },
    {
      "@type": "Question",
      "name": "Why is my CPU usage high?",
      "acceptedAnswer": {
        "@type": "Answer",
        "text": "Check if you are running multiple instances of the filter on different sources. Also, note that the plugin uses a constant amount of CPU regardless of camera resolution (720p or 4K)."
      }
    },
    {
      "@type": "Question",
      "name": "Why create a new plugin instead of fixing the old one?",
      "acceptedAnswer": {
        "@type": "Answer",
        "text": "As a contributor to the original plugin, I found that architectural limitations made it hard to guarantee stability. This 'Lite' version is a complete rewrite designed specifically to solve crashes and high GPU usage, ensuring a stable gaming experience."
      }
    }
  ]
}
</script>
