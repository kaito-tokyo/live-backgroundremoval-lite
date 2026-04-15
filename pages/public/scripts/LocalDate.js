// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

// Use `<script src=".../LocalDate.js"></script>` to include this. This script is not a module.

class LocalDateElement extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: "open" });
  }

  connectedCallback() {
    try {
      this.shadowRoot.textContent = new Date(this.dataset.date).toLocaleDateString("en-CA");
    } catch (e) {
      this.shadowRoot.textContent = "Invalid Date";
    }
  }
}

if (!customElements.get("local-date")) {
  customElements.define("local-date", LocalDateElement);
}
