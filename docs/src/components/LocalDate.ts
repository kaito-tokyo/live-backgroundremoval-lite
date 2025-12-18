customElements.define(
  "local-date",
  class extends HTMLElement {
    constructor() {
      super();
      this.attachShadow({ mode: "open" });
    }
    connectedCallback() {
      const dateStr = new Date(this.dataset.date!).toLocaleDateString("en-CA");
      this.shadowRoot!.textContent = dateStr;
    }
  },
);
