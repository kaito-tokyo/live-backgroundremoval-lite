class LocalizedAnchorElement extends HTMLElement {
  private _handleSlotChange;
  private _slot;
  private _hrefMap: Record<string, string>;

  constructor() {
    super();
    this.attachShadow({ mode: "open" });
    const slot = document.createElement("slot");
    if (!this.shadowRoot) {
      throw new Error("Shadow root is not available.");
    }
    this.shadowRoot.appendChild(slot);

    this._handleSlotChange = this.handleSlotChange.bind(this);
    this._slot = slot;
    this._hrefMap = {};
  }

  matchLocale(supported: readonly string[], requested: readonly string[], fallback: string) {
    for (const req of requested) {
      try {
        const reqLocale = new Intl.Locale(req);

        if (supported.includes(reqLocale.baseName)) {
          return reqLocale.baseName;
        }

        const found = supported.find(sup => {
          const supLocale = new Intl.Locale(sup);
          return supLocale.language === reqLocale.language;
        });

        if (found) {
          return found;
        }
      } catch (e) {
        continue;
      }
    }

    return fallback;
  }

  handleSlotChange(e: Event) {
    if (!this.dataset.hrefmap) {
      throw new Error("LocalizedAnchor requires a hrefmap dataset attribute.");
    }
    this._hrefMap = JSON.parse(this.dataset.hrefmap);

    const slot = this._slot;
    const hrefMap = this._hrefMap;
    const nodes = slot.assignedElements();
    if (nodes.length === 0 || !(nodes[0] instanceof HTMLAnchorElement)) {
      throw new Error("LocalizedAnchor requires an <a> element as its child.");
    }
    const a = nodes[0];
    const matchedLocale = this.matchLocale(Object.keys(hrefMap), navigator.languages, "en");
    a.href = hrefMap[matchedLocale];
  }

  connectedCallback() {
    const slot = this._slot;
    slot.addEventListener("slotchange", this._handleSlotChange);
  }

  disconnectedCallback() {
    const slot = this._slot;
    slot.removeEventListener("slotchange", this._handleSlotChange);
  }
}

customElements.define("localized-anchor", LocalizedAnchorElement);
