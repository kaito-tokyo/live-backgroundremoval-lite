customElements.define(
  "local-date",
  class extends HTMLElement {
    connectedCallback() {
      this.textContent = new Date(this.dataset.date!).toLocaleDateString(
        "en-CA",
      );
    }
  },
);
