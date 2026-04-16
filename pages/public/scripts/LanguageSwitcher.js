// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

// Use `<script src=".../LanguageSwitcher.js"></script>` to include this. This script is not a module.

class LanguageSwitcherElement extends HTMLElement {
  /**
   * @param {MouseEvent} e
   */
  static onClick(e) {
    const self = e.currentTarget?.closest("language-switcher");

    if (self instanceof LanguageSwitcherElement) {
      self.querySelector("div")?.showPopover();
    } else {
      /** @type {HTMLLinkElement | null} */
      const link = document.head.querySelector("link[rel='alternate'][hreflang='x-default']");
      if (!link) throw new Error("DefaultAlternateMissingError");
      location.href = link.href;
    }
  }

  connectedCallback() {
    if (this.querySelector("div[popover]")) {
      // Already initialized
      return;
    }

    const langTexts = {
      "en": "English",
      "de-de": "Deutsch",
      "es-es": "Español",
      "fr-fr": "Français",
      "ko-kr": "한국어",
      "ja-jp": "日本語",
      "pt-br": "Português (Brasil)",
      "ru-ru": "Русский",
      "zh-cn": "中文（简体）",
      "zh-tw": "中文（繁體）"
    };

    const h2 = document.createElement("h2");
    h2.textContent = "🌐 Language";

    const div = document.createElement("div");
    div.popover = "auto";
    div.appendChild(h2);

    const ul = document.createElement("ul");
    div.appendChild(ul);
    document.head.querySelectorAll("link").forEach((link) => {
      const { rel, href, hreflang } = link;
      if (rel !== "alternate" || !hreflang || hreflang === "x-default") return;

      const a = document.createElement("a");
      a.textContent = langTexts[hreflang] || hreflang;
      a.href = href;

      const li = document.createElement("li");
      li.appendChild(a);

      ul.appendChild(li);
    });

    this.appendChild(div);
  }
}

if (!customElements.get("language-switcher")) {
  customElements.define("language-switcher", LanguageSwitcherElement);
}
