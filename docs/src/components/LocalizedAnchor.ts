class LocalizedAnchorElement extends HTMLElement {
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
    const hrefMap = JSON.parse(host.dataset.hrefmap || "{}");
    const nodes = slot.assignedElements();
    if (nodes.length === 0 || !(nodes[0] instanceof HTMLAnchorElement)) {
      throw new Error("LocalizedAnchor requires an <a> element as its child.");
    }
    const a = nodes[0];
    for (const lang of navigator.languages) {
      const lowerLang = lang.toLowerCase();
      if (lowerLang in hrefMap) {
        a.href = hrefMap[lowerLang];
        break;
      }
      const looseMatch = Object.keys(hrefMap).find(e => e.slice(0, 2) === lowerLang);
      if (looseMatch) {
        a.href = hrefMap[looseMatch];
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
      throw new Error("LocalizedAnchor requires a <slot> element.");
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
      throw new Error("LocalizedAnchor requires a <slot> element.");
    }
    slot.removeEventListener("slotchange", this.handleSlotChange);
  }
}

customElements.define("localized-anchor", LocalizedAnchorElement);
