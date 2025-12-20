// @ts-check
import { defineConfig } from "astro/config";

import lit from "@astrojs/lit";
import sitemap from "@astrojs/sitemap";
import svelte from "@astrojs/svelte";

// https://astro.build/config
export default defineConfig({
  integrations: [svelte(), sitemap(), lit()],
  build: {
    assets: "assets",
  },
});
