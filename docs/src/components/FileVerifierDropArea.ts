customElements.define(
  "file-verifier-drop-area",
  class extends HTMLElement {
    l: HTMLLabelElement | null;
    c?: AbortController;

    constructor() {
      super();
      this.attachShadow({ mode: "open" });

      this.shadowRoot!.innerHTML = /*html*/ `
        <label>
          <div>Drag &amp; Drop file(s) here to verify</div>
          <div>No data will be sent to the Internet</div>
        </label>

        <style>
          :host {
            display: block;
          }

          label {
            height: 7rem;
            display: flex;
            align-items: center;
            justify-content: center;
            flex-direction: column;
            border: 2px dashed #ccc;
            border-radius: 20px;
            transition: transform .05s;

            * {
              pointer-events: none;
            }
          }

          .active {
            border-color: #2196F3;
            transform: scale(1.02);
          }
        </style>
      `;

      this.l = this.shadowRoot!.children.item(0) as HTMLLabelElement;
    }

    connectedCallback() {
      const label = this.l!;
      this.c = new AbortController();
      const { signal } = this.c;
      label.addEventListener(
        "dragover",
        (e) => {
          e.preventDefault();
          e.stopPropagation();
          label.classList.add("active");
        },
        { signal },
      );
      label.addEventListener(
        "dragleave",
        (e) => {
          e.preventDefault();
          e.stopPropagation();
          label.classList.remove("active");
        },
        { signal },
      );
      label.addEventListener(
        "drop",
        async (e: DragEvent) => {
          e.preventDefault();
          e.stopPropagation();
          label.classList.remove("active");
          if (!e.dataTransfer) return;
          for (const file of e.dataTransfer.files) {
            const buffer = await file.arrayBuffer();
            const sha256Buffer = await crypto.subtle.digest("SHA-256", buffer);
            this.dispatchEvent(
              new CustomEvent("file-verifier-file-dropped", {
                detail: {
                  name: file.name,
                  size: file.size,
                  sha256Buffer: sha256Buffer,
                },
                bubbles: true,
                composed: true,
              }),
            );
          }
        },
        { signal },
      );
    }

    disconnectedCallback() {
      this.c?.abort();
    }
  },
);
