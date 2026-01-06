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

  resolveLocale(languages: readonly string[]): string {
    for (const rawLang of languages) {
      const lang = rawLang.toLowerCase();

      if (lang.startsWith('zh')) {
        if (
          lang === 'zh-tw' ||
          lang === 'zh-hk' ||
          lang === 'zh-mo' ||
          lang.includes('hant')
        ) {
          return 'zh-tw';
        }
        return 'zh-cn';
      }

      if (lang.startsWith('ja')) return 'ja-jp';
      if (lang.startsWith('en')) return 'en';
      if (lang.startsWith('ko')) return 'ko-kr';

      if (lang.startsWith('de')) return 'de-de';
      if (lang.startsWith('fr')) return 'fr-fr';
      if (lang.startsWith('es')) return 'es-es';
      if (lang.startsWith('pt')) return 'pt-br';
      if (lang.startsWith('ru')) return 'ru-ru';
    }

    return 'en';
  }

  handleSlotChange(e: Event) {
    this.m = JSON.parse(this.dataset.map!);

    const slot = this.s;
    const map = this.m;
    const nodes = slot.assignedElements();
    const child = nodes[0];
    const matchedLocale = this.resolveLocale(navigator.languages);
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
