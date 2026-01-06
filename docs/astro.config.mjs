// @ts-check
import { defineConfig } from "astro/config";

import sitemap from "@astrojs/sitemap";

// https://astro.build/config
export default defineConfig({
  i18n: {
    locales: [
      "de-de",
      "en",
      "es-es",
      "fr-fr",
      "ja-jp",
      "ko-kr",
      "pt-br",
      "ru-ru",
      "zh-cn",
      "zh-tw",
    ],
    defaultLocale: "en",
  },
  integrations: [sitemap()],
  build: {
    assets: "assets",
  },
});
