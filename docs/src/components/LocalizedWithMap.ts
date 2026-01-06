class LocalizedWithMapElement extends HTMLElement {
  private h;
  private s;
  private m!: Record<string, string>;

  constructor() {
    super();
    this.attachShadow({ mode: "open" });
    const slot = document.createElement("slot");
    this.shadowRoot!.appendChild(slot);

    this.h = this.handleSlotChange.bind(this);
    this.s = slot;
  }

  matchLocale(
    supported: readonly string[],
    requested: readonly string[],
    fallback: string,
  ) {
    for (const req of requested) {
      try {
        const reqLocale = new Intl.Locale(req);

        if (supported.includes(reqLocale.baseName)) return reqLocale.baseName;

        const found = supported.find((sup) => {
          const supLocale = new Intl.Locale(sup);
          return supLocale.language === reqLocale.language;
        });

        if (found) return found;
      } catch (e) {
        continue;
      }
    }

    return fallback;
  }

  handleSlotChange(e: Event) {
    this.m = JSON.parse(this.dataset.map!);

    const slot = this.s;
    const map = this.m;
    const nodes = slot.assignedElements();
    const child = nodes[0];
    const matchedLocale = this.matchLocale(
      Object.keys(map),
      navigator.languages,
      "en",
    );
    if (child instanceof HTMLAnchorElement) child.href = map[matchedLocale];
    else if (child instanceof HTMLSpanElement)
      child.textContent = map[matchedLocale];
  }

  connectedCallback() {
    const slot = this.s;
    slot.addEventListener("slotchange", this.h);
  }

  disconnectedCallback() {
    const slot = this.s;
    slot.removeEventListener("slotchange", this.h);
  }
}

customElements.define("localized-with-map", LocalizedWithMapElement);
