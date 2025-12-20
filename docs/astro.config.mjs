// @ts-check
import { defineConfig } from "astro/config";

import svelte from "@astrojs/svelte";

import sitemap from "@astrojs/sitemap";

// https://astro.build/config
export default defineConfig({
  integrations: [svelte(), sitemap()],
  // HTMLの圧縮を無効化
  compressHTML: false,

  // JSとCSSの圧縮を無効化（Viteの設定）
  vite: {
    build: {
      minify: false,
    },
  },
});
