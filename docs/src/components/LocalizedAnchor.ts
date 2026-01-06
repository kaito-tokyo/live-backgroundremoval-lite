class LocalizedAnchorElement extends HTMLElement {
  private slotChangeHandler_?: (e: Event) => void;

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

    if (!this.slotChangeHandler_) {
      this.slotChangeHandler_ = () => {
        const nodes = slot.assignedElements();
        const a = nodes[0] as HTMLAnchorElement;
        for (const lang of navigator.languages) {
          const lowerLang = lang.toLowerCase();
          if (lowerLang in hrefMap) {
            a.href = hrefMap[lowerLang];
            break;
          }
        }
      };
      slot.addEventListener("slotchange", this.slotChangeHandler_);
    }
  }

  disconnectedCallback() {
    const shadowRoot = this.shadowRoot;
    if (!shadowRoot) {
      return;
    }

    const slot = shadowRoot.querySelector("slot") as HTMLSlotElement | null;
    if (slot && this.slotChangeHandler_) {
      slot.removeEventListener("slotchange", this.slotChangeHandler_);
      this.slotChangeHandler_ = undefined;
    }
  }
}

customElements.define("localized-anchor", LocalizedAnchorElement);
