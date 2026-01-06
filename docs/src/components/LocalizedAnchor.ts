class LocalizedAnchorElement extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: "open" });
    const slot = document.createElement("slot");
    this.shadowRoot!.appendChild(slot);
  }
  connectedCallback() {
    const shadowRoot = this.shadowRoot!;
    const slot = shadowRoot.querySelector("slot") as HTMLSlotElement;
    const hrefMap = JSON.parse(this.dataset.hrefmap || "{}");
    slot.addEventListener("slotchange", (e) => {
      const nodes = slot.assignedElements();
      const a = nodes[0] as HTMLAnchorElement;
      for (const lang of navigator.languages) {
        const lowerLang = lang.toLowerCase();
        if (lowerLang in hrefMap) {
          a.href = hrefMap[lowerLang];
          break;
        }
      }
    });
  }
}

customElements.define("localized-anchor", LocalizedAnchorElement);
