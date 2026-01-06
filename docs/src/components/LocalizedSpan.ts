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
    console.log(textMap);
    slot.addEventListener("slotchange", (e) => {
      const nodes = slot.assignedElements();
      const span = nodes[0] as HTMLSpanElement;
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
