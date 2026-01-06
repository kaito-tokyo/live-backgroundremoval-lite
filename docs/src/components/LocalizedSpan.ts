class LocalizedSpanElement extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: "open" });
    const slot = document.createElement("slot");
    this.shadowRoot!.appendChild(slot);
  }
  connectedCallback() {
    const shadowRoot = this.shadowRoot!;
    const slot = shadowRoot.querySelector("slot") as HTMLSlotElement;
    const textMap = JSON.parse(this.dataset.textmap || "{}");
    slot.addEventListener("slotchange", () => {
      const nodes = slot.assignedElements();
      if (nodes.length === 0) {
        return;
      }
      const firstNode = nodes[0];
      if (!(firstNode instanceof HTMLSpanElement)) {
        return;
      }
      const span = firstNode;
      for (const lang of navigator.languages) {
        const lowerLang = lang.toLowerCase();
        if (lowerLang in textMap) {
          span.textContent = textMap[lowerLang];
          break;
        }
      }
    });
  }
}

customElements.define("localized-span", LocalizedSpanElement);
