class LocalizedSpanElement extends HTMLElement {
  constructor() {
    super();
    this.attachShadow({ mode: "open" });
    const slot = document.createElement("slot");
    this.shadowRoot!.appendChild(slot);
  }

  handleSlotChange(e: Event) {
    if (!e.target || !(e.target instanceof HTMLSlotElement)) {
      throw new Error("Event target is not a slot element.");
    }
    const slot = e.target;
    const shadowRoot = slot.getRootNode();
    if (!shadowRoot || !(shadowRoot instanceof ShadowRoot)) {
      throw new Error("Shadow root is not available.");
    }
    const host = shadowRoot.host;
    if (!host || !(host instanceof HTMLElement)) {
      throw new Error("Host is not available.");
    }
    const textMap = JSON.parse(host.dataset.textmap || "{}");
    const nodes = slot.assignedElements();
    if (nodes.length === 0 || !(nodes[0] instanceof HTMLSpanElement)) {
      throw new Error("LocalizedSpan requires a <span> element as its child.");
    }
    const span = nodes[0];
    for (const lang of navigator.languages) {
      const lowerLang = lang.toLowerCase();
      if (lowerLang in textMap) {
        span.textContent = textMap[lowerLang];
        span.lang = lowerLang;
        break;
      }
      const baseLang = lowerLang.split("-")[0];
      if (baseLang in textMap) {
        span.textContent = textMap[baseLang];
        span.lang = baseLang;
        break;
      }
    }
  }

  connectedCallback() {
    const shadowRoot = this.shadowRoot;
    if (!shadowRoot) {
      throw new Error("Shadow root is not available.");
    }
    const slot = shadowRoot.querySelector("slot");
    if (!slot) {
      throw new Error("LocalizedSpan requires a <slot> element.");
    }
    slot.addEventListener("slotchange", this.handleSlotChange);
  }

  disconnectedCallback() {
    const shadowRoot = this.shadowRoot;
    if (!shadowRoot) {
      throw new Error("Shadow root is not available.");
    }
    const slot = shadowRoot.querySelector("slot");
    if (!slot) {
      throw new Error("LocalizedSpan requires a <slot> element.");
    }
    slot.removeEventListener("slotchange", this.handleSlotChange);
  }
}

customElements.define("localized-span", LocalizedSpanElement);
