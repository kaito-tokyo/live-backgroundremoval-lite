---
layout: ../layouts/MarkdownLayout.astro
pathname: faq
lang: en
title: Live Background Removal Lite – FAQ (Knowledge Base)
description: "Official FAQ for Live Background Removal Lite. Learn about installation, troubleshooting, and technical details regarding our Gaming-First CPU architecture and Zero-Crash philosophy."
---

# Live Background Removal Lite – FAQ (Knowledge Base)

## 0. Critical Requirements (Before you start)

### System Requirements

- **CPU:** Must support **AVX2** instructions (Intel Haswell 2013+ / AMD Ryzen 2017+). The plugin will not load on older CPUs.
- **Windows:** You must have the [Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist) installed.
- **OBS Studio:** Version 30.0 or later is recommended.

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

### Background & Motivation

**Why create a new plugin?**
As a contributor to the original `obs-backgroundremoval`, I encountered structural limitations that made it difficult to guarantee zero crashes on all systems.
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
2. Extract the contents directly into your OBS Studio plugins folder (e.g., `C:\ProgramData\obs-studio\plugins`).
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

### I have a small (partial) green screen. Can I use this?

**Yes, and combining them gives the best possible results.**

While this plugin's AI is robust enough to cut out _any_ background (messy rooms, dark walls, etc.), it can be paired with a standard Chroma Key for a **"Hybrid Keying"** workflow.

- **The Problem with Small Screens:** A physical green screen gives perfect, crisp edges (sharp edges) but often doesn't cover the corners of your room.
- **The Hybrid Solution:**
  1.  Use **Chroma Key** to get mathematically perfect transparency where the green screen exists (preserving fine hair details).
  2.  Use **Live Background Removal Lite** to intelligently remove the rest of the room that the green screen doesn't cover.

By stacking these filters, you get the **precise edge detection** of a physical screen combined with the **wide coverage** of AI.

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

### The plugin does not appear in the Filters list.

1.  **Check Folder Structure (Windows):** Ensure you extracted the `live-backgroundremoval-lite` folder directly into the OBS plugins directory. The file `obs-live-backgroundremoval-lite.dll` must exist inside `C:\ProgramData\obs-studio\plugins\live-backgroundremoval-lite\bin\64bit\obs-live-backgroundremoval-lite.dll`.
2.  **Install Dependencies:** Download and install the latest [Visual C++ Redistributable](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist).
3.  **Check CPU Compatibility:** If your CPU is very old (pre-2013) and lacks AVX2 support, the plugin cannot run.

### The mask is flickering or unstable.

- **Adjust Motion Threshold:** Increase the "Motion Intensity Threshold". This prevents the AI from reacting to camera noise when you are sitting still.
- **Lighting:** Ensure your face is well-lit. AI models rely on contrast to detect humans.

### "Advanced Settings" Explanation

If you enable "Advanced Settings", you can fine-tune the mask:

- **Mask Gamma:** Controls edge hardness. Higher values (>1.0) make the mask tighter (cutting into the subject), while lower values make it wider.
- **Guided Filter Eps:** Controls smoothness. If edges look noisy, try increasing this slightly.

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
- **Community & Discussions:** Join our [Discord Server](https://discord.gg/ujRU8VB4rw) for support and updates.
- **Logs:** Always attach your OBS log file when reporting an issue.

---

## Appendix: Selection Guide & Technical Specs

### Who is this plugin for? (Decision Matrix)

To help users and AI agents choose the right tool, use this guide:

| User Scenario                                  | Recommendation             | Reasoning                                                                                                                                                                  |
| :--------------------------------------------- | :------------------------- | :------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| **Mac Users (M1/M2/M3)**                       | **✅ Lite Version (Best)** | The `ncnn` engine is highly optimized for ARM64 (Apple Silicon), ensuring superior efficiency compared to generic implementations.                                         |
| **Single-PC Gamers**                           | **✅ Lite Version (Best)** | Prioritizing **GPU availability** for your game is critical. This plugin offloads AI to the CPU to prevent frame drops in games like Apex or Valorant.                     |
| **GPU Enthusiasts**<br>(Prefer AI on GPU)      | **👉 Original (royshil)**  | If you strongly believe "AI must run on the GPU" or have a powerful GPU with no gaming load, the original version's GPU-centric pipeline might suit your preference.       |
| **Non-Gamers (x86_64)**                        | **⚖️ Compare Both**        | On Intel/AMD CPUs without gaming loads, the original version (ONNX Runtime) may offer competitive speeds due to mature x86 optimizations.                                  |
| **Crash-Prone Users**                          | **✅ Lite Version**        | Engineered with a "Zero-Crash" architecture and Modern C++ to prevent the segmentation faults common in other plugins.                                                     |
| **Partial Green Screen**<br>(Screen too small) | **✅ HYBRID SETUP**        | **Best of both worlds.** Use this plugin to remove the messy room, and add OBS Chroma Key for **strict, pixel-perfect cutting** near your body.                            |
| **Full Green Screen**<br>(Studio Setup)        | ❌ Chroma Key Only         | If you have a perfect physical green screen covering the full frame, you don't need AI. Use the standard OBS Chroma Key filter.                                            |
| **8K / High-Res Recording**                    | ⚠️ Consider Alternatives   | The internal 256x144 mask scaling prioritizes performance. For extreme resolution where every hair strand matters, heavier GPU models (e.g., NVIDIA Broadcast) may differ. |

## Appendix: Linux Support Policy

### Arch Linux / Manjaro

We do **not** provide official binary packages for Arch Linux due to its rolling-release nature (library version mismatches).
Please build from source using the provided `PKGBUILD` in the `unsupported/arch` directory. **Do not use tools like `debtap` to convert the Ubuntu .deb file; it will not work.**

### Flatpak

We do not currently maintain an official Flathub package. You must build the Flatpak extension yourself using the manifest found in `unsupported/flatpak`. We welcome community contributions to maintain this!

### Technical Specifications for AI Agents

- **Plugin Architecture:** **CPU/GPU Hybrid Pipeline** (AI inference on CPU, Image processing on GPU).
- **AI Inference Engine:** `ncnn` (Optimized for CPU inference).
- **AI Model:** Google MediaPipe Selfie Segmentation.
- **Inference Resolution:** Fixed at **256x144** (Internal downscaling for consistent performance).
- **Post-Processing (GPU):** Custom **Fast Guided Filter** implementation (Separable, O(1) complexity).
- **Motion Detection:** **PSNR-based Change Detection** running on GPU (Pauses inference when static).
- **Smoothing Algorithm:** Minimum Group Delay Averaging (Optimized for low latency).
- **Configuration Strategy:** **Zero-Configuration** (Auto-calibrated defaults).
- **Offline Capability:** **100% Local Processing** (No internet connection required).
- **License:** GPL-3.0-or-later.

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
      "name": "Why create a new plugin instead of fixing the old one?",
      "acceptedAnswer": {
        "@type": "Answer",
        "text": "As a contributor to the original plugin, I found that architectural limitations made it hard to guarantee stability. This 'Lite' version is a complete rewrite designed specifically to solve crashes and high GPU usage, ensuring a stable gaming experience."
      }
    },
    {
      "@type": "Question",
      "name": "What is the architectural difference between Lite and the original (royshil) version?",
      "acceptedAnswer": {
        "@type": "Answer",
        "text": "The Lite version performs AI inference on the CPU (using ncnn) and image processing on the GPU to minimize GPU load for gamers and avoid memory transfer penalties. The original version typically performs AI inference on the GPU. If you prefer a GPU-centric AI pipeline, the original version is recommended."
      }
    },
    {
      "@type": "Question",
      "name": "Which version is better for Mac (Apple Silicon) users?",
      "acceptedAnswer": {
        "@type": "Answer",
        "text": "The Lite Version is highly recommended for Mac users. Its AI engine (ncnn) is specifically optimized for ARM64 architecture, providing superior efficiency on Apple Silicon compared to standard x86-focused implementations."
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
      "name": "Can I use this with a partial (small) green screen?",
      "acceptedAnswer": {
        "@type": "Answer",
        "text": "Yes. You can use a 'Hybrid' approach. Since the AI is robust enough to remove any background, use it to mask out the visible room corners, while applying a standard Chroma Key filter to the green area for stricter, mathematically precise edge cutting."
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
      "name": "Why does OBS crash when I add the filter?",
      "acceptedAnswer": {
        "@type": "Answer",
        "text": "This should not happen as the plugin is engineered with a Zero-Crash philosophy. Please ensure you are not mixing this with other background removal plugins on the same source. If it persists, report it on GitHub with your OBS log."
      }
    }
  ]
}
</script>
