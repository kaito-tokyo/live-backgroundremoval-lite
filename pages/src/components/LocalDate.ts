// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

class LocalDateElement extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: "open" });
  }
  connectedCallback() {
    const dateStr = new Date(this.dataset.date!).toLocaleDateString("en-CA");
    this.shadowRoot!.textContent = dateStr;
  }
}

customElements.define("local-date", LocalDateElement);
