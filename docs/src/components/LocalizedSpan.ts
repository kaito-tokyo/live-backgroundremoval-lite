class LocalizedSpanElement extends HTMLElement {
  private _handleSlotChange;
  private _slot;
  private _textMap: Record<string, string>;

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
    this._textMap = {};
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
    if (!this.dataset.textmap) {
      throw new Error("LocalizedSpan requires a textmap dataset attribute.");
    }
    this._textMap = JSON.parse(this.dataset.textmap);

    const slot = this._slot;
    const textMap = this._textMap;
    const nodes = slot.assignedElements();
    if (nodes.length === 0 || !(nodes[0] instanceof HTMLSpanElement)) {
      throw new Error("LocalizedSpan requires a <span> element as its child.");
    }
    const span = nodes[0];
    const matchedLocale = this.matchLocale(Object.keys(textMap), navigator.languages, "en");
    span.textContent = textMap[matchedLocale];
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

customElements.define("localized-span", LocalizedSpanElement);
