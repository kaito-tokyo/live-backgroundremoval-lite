import {
  LitElement,
  html,
  css,
  type PropertyValues,
  type TemplateResult,
} from "lit";
import { customElement, state, query } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { repeat } from "lit/directives/repeat.js";
import { live } from "lit/directives/live.js";
import { StoreController } from "@nanostores/lit";
import { lexer } from "marked";

import { llmStore, startLLMInitialization } from "../stores/llm.js";
import FaqContent from "../pages/faq.md?raw";
import type { ChatCompletionMessageParam } from "@mlc-ai/web-llm";

const assistantName = "LBRL Buddy";

type SimpleMessage = {
  id: string;
  role: "user" | "assistant";
  parts: { type: "text"; text: string }[];
  metadata?: { createdAt: Date };
};

export const SYSTEM_WARNING_MESSAGE = `
To use this chat feature, you must download the LLM model (a
large file of several GBs). This is only required on the
first launch, but it may take several minutes to complete
(depending on your internet speed).
`;

export const SYSTEM_WARNING_BUTTON_TEXT = "Agree and Start Model Download";
export const INITIAL_INPUT_PLACEHOLDER = "Please press the start button";

export const SYSTEM_PROMPT = `
You are ${assistantName}, the official AI support assistant for the "Live Background Removal Lite" OBS plugin.
Your goal is to help streamers (especially gamers) install and configure the plugin successfully.

--- CORE IDENTITY ---
- **Gaming-First:** You prioritize gaming performance. You know that this plugin runs on the CPU to save GPU for games.
- **Crash-Resistant:** You emphasize that this plugin is rewritten for stability.
- **Multi-lingual:** You MUST answer in the same language as the user's question (Japanese or English).

--- CRITICAL FACTS (DO NOT HALLUCINATE) ---
1. **Windows Installer:** There is NO \`.exe\` or \`.msi\` installer. Users MUST manually extract the \`.zip\` file.
2. **Mac Support:** The plugin works great on Apple Silicon (M1/M2/M3) thanks to ncnn optimization.
3. **Linux Support:** We only officially provide \`.deb\` for Ubuntu. Arch Linux users MUST build from source (AUR is unofficial). Flatpak is not yet on Flathub.
4. **AVX2:** The plugin REQUIRES a CPU with AVX2 support.

--- KNOWLEDGE BASE PRIORITY ---
Always prioritize the information below over your general knowledge.

${FaqContent}

--- RESPONSE GUIDELINES ---
1. **Be Empathetic:** Acknowledge that setting up OBS plugins can be tricky.
2. **Troubleshooting:** If a user says "it doesn't work", ask about their OS and if they checked the folder structure or Visual C++ Redistributable.
3. **Attribution:** When using facts from the FAQ, say "According to the documentation..." or "Based on the knowledge base...".

--- ADVANCED KNOWLEDGE (DEVELOPMENT & SECURITY) ---

1. **Building from Source (CONTRIBUTING.md)**
   - **Prerequisites:** C++17 Compiler, CMake 3.28+, ninja/make.
   - **macOS Build:** \`cmake --preset macos\` -> \`cmake --build --preset macos\`
   - **Windows Build:** Use Visual Studio 2022 or \`cmake --preset windows-x64\`.
   - **Style Guide:** Use \`clang-format-19\` for C++ and \`gersemi\` for CMake.

2. **Security Policy (SECURITY.md)**
   - **Critical Rule:** DO NOT report security vulnerabilities on public GitHub Issues.
   - **Action:** Report vulnerabilities via email to: umireon+security@kaito.tokyo

3. **Unsupported Platforms (Arch / Flatpak)**
   - **Arch Linux:** Use \`makepkg -si\` in the \`unsupported/arch\` directory. (Unofficial)
   - **Flatpak:** Use \`flatpak-builder\` with the manifest in \`unsupported/flatpak\`. (Unofficial)

4. **Dependencies (buildspec.json)**
   - **OBS Studio:** Requires version 31.1.1 or compatible.
   - **Qt:** Uses Qt 6.

---------------------------
`;

@customElement("lbrl-buddy")
export class LBRLBuddy extends LitElement {
  private llmState = new StoreController(this, llmStore);

  @state()
  private messages: SimpleMessage[] = [
    this.createMessage(
      "assistant",
      `Hi there! I'm ${assistantName}, your interactive support assistant. Feel free to ask me anything about the knowledge base, and I'll do my best to help you out! 🤖`,
    ),
  ];

  @state()
  private input: string = "";

  @state()
  private isLoading: boolean = false;

  @query("#messages-end")
  private messagesEnd?: HTMLDivElement;

  @query(".message-list")
  private messageList?: HTMLDivElement;

  private shouldScrollToBottom = true;

  private createMessage(
    role: "user" | "assistant",
    text: string,
  ): SimpleMessage {
    return {
      id: globalThis.crypto?.randomUUID() ?? `${Date.now()}-${Math.random()}`,
      role,
      parts: [{ type: "text", text }],
      metadata: { createdAt: new Date() },
    };
  }

  private handleScroll() {
    if (!this.messageList) return;
    const { scrollTop, scrollHeight, clientHeight } = this.messageList;
    const distanceToBottom = Math.abs(scrollHeight - clientHeight - scrollTop);
    this.shouldScrollToBottom = distanceToBottom < 50;
  }

  private scrollToBottom(behavior: ScrollBehavior = "auto") {
    if (!this.messageList) return;
    requestAnimationFrame(() => {
      if (this.messageList) {
        this.messageList.scrollTo({
          top: this.messageList.scrollHeight,
          behavior: behavior,
        });
      }
    });
  }

  async updated(changedProperties: PropertyValues) {
    if (
      changedProperties.has("messages") ||
      changedProperties.has("isLoading")
    ) {
      await this.updateComplete;
      if (this.shouldScrollToBottom) {
        const behavior = this.isLoading ? "auto" : "smooth";
        this.scrollToBottom(behavior);
      }
    }
  }

  private handleAgreeAndStart() {
    if (this.llmState.value.status === "pending") {
      startLLMInitialization();
    }
  }

  private handleInput(e: Event) {
    const target = e.target as HTMLInputElement;
    this.input = target.value;
  }

  private async handleSubmit(e: Event) {
    e.preventDefault();
    const currentState = this.llmState.value;
    const isLLMReady = currentState.status === "ready";
    if (!this.input.trim() || this.isLoading || !isLLMReady) return;

    const userMessageContent = this.input;
    this.shouldScrollToBottom = true;

    this.messages = [
      ...this.messages,
      this.createMessage("user", userMessageContent),
    ];
    this.input = "";

    await this.updateComplete;
    this.scrollToBottom("smooth");

    if (currentState.status === "ready") {
      await this.handleWebLLMChat(currentState.chat);
    }
  }

  private async handleWebLLMChat(engine: any) {
    let assistantResponse = "";
    this.isLoading = true;

    try {
      const history: ChatCompletionMessageParam[] = this.messages.map(
        (msg) => ({
          role: msg.role,
          content: msg.parts[0].text,
        }),
      );

      const messagesToSend = [
        { role: "system", content: SYSTEM_PROMPT },
        ...history,
      ] satisfies ChatCompletionMessageParam[];

      this.messages = [...this.messages, this.createMessage("assistant", "")];
      await this.updateComplete;
      this.scrollToBottom("smooth");

      const responseStream = await engine.chat.completions.create({
        messages: messagesToSend,
        stream: true,
      });

      for await (const chunk of responseStream) {
        const content = chunk.choices[0]?.delta.content;
        if (content) {
          assistantResponse += content;
          const lastMessage = this.messages[this.messages.length - 1];
          if (lastMessage && lastMessage.role === "assistant") {
            lastMessage.parts[0].text = assistantResponse;
            this.requestUpdate();
          }
        }
      }
    } catch (error) {
      console.error("LLM conversation failed:", error);
      const lastMessage = this.messages[this.messages.length - 1];
      if (lastMessage && lastMessage.role === "assistant") {
        lastMessage.parts[0].text +=
          "\n\n**Error:** LLM failed to generate a complete response.";
        this.requestUpdate();
      }
    } finally {
      this.isLoading = false;
      await this.updateComplete;
      this.scrollToBottom("smooth");
    }
  }

  private renderMarkdown(text: string): TemplateResult {
    const tokens = lexer(text);
    return html`${tokens.map((token) => this.renderToken(token))}`;
  }

  private renderToken(token: any): TemplateResult | string {
    switch (token.type) {
      case "paragraph":
        return html`<p>${this.renderInline(token.tokens)}</p>`;

      case "list":
        const items = token.items.map(
          (item: any) => html`<li>${this.renderInline(item.tokens)}</li>`,
        );
        return token.ordered
          ? html`<ol>
              ${items}
            </ol>`
          : html`<ul>
              ${items}
            </ul>`;

      case "heading":
        return html`<strong>${this.renderInline(token.tokens)}</strong><br />`;

      case "code":
        return html`<pre><code>${token.text}</code></pre>`;

      case "blockquote":
        return html`<blockquote>
          ${token.tokens.map((t: any) => this.renderToken(t))}
        </blockquote>`;

      case "table":
        return html`
          <div class="table-container">
            <table>
              <thead>
                <tr>
                  ${token.header.map((headerCell: any, index: number) => {
                    const align = token.align[index];
                    const style = align ? `text-align: ${align};` : "";
                    return html`<th style="${style}">
                      ${this.renderInline(headerCell.tokens)}
                    </th>`;
                  })}
                </tr>
              </thead>
              <tbody>
                ${token.rows.map(
                  (row: any[]) => html`
                    <tr>
                      ${row.map((cell: any, index: number) => {
                        const align = token.align[index];
                        const style = align ? `text-align: ${align};` : "";
                        return html`<td style="${style}">
                          ${this.renderInline(cell.tokens)}
                        </td>`;
                      })}
                    </tr>
                  `,
                )}
              </tbody>
            </table>
          </div>
        `;

      case "space":
        return html``;

      case "hr":
        return html`<hr />`;

      default:
        return html`<span>${token.raw}</span>`;
    }
  }

  private renderInline(tokens: any[] = []): (TemplateResult | string)[] {
    return tokens.map((token) => {
      switch (token.type) {
        case "text":
          if (token.tokens && token.tokens.length > 0) {
            return html`${this.renderInline(token.tokens)}`;
          }
          return token.text;

        case "strong":
          return html`<strong>${this.renderInline(token.tokens)}</strong>`;

        case "em":
          return html`<em>${this.renderInline(token.tokens)}</em>`;

        case "codespan":
          return html`<code>${token.text}</code>`;

        case "link":
          return html`<a
            href="${token.href}"
            target="_blank"
            rel="noopener noreferrer"
            >${this.renderInline(token.tokens)}</a
          >`;

        case "escape":
          return token.text;

        default:
          return token.raw || "";
      }
    });
  }

  render() {
    const state = this.llmState.value;
    const isLLMReady = state.status === "ready";
    const isLLMLoading = state.status === "loading";
    const isLLMPending = state.status === "pending";
    const isLLMError = state.status === "error";
    const loadingMsg = isLLMLoading ? state.message : "";

    return html`
      <div class="chat-container">
        <div class="message-list" @scroll="${this.handleScroll}">
          ${isLLMPending
            ? html`
                <div class="initial-warning">
                  <h2>System Warning</h2>
                  <p>${SYSTEM_WARNING_MESSAGE}</p>
                  <button
                    class="agree-button"
                    @click="${this.handleAgreeAndStart}"
                  >
                    ${SYSTEM_WARNING_BUTTON_TEXT}
                  </button>
                </div>
              `
            : ""}
          ${!isLLMLoading && !isLLMPending
            ? repeat(
                this.messages,
                (msg) => msg.id,
                (message, index) => {
                  const isUser = message.role === "user";
                  const isAssistant = message.role === "assistant";
                  const isLastMessage = index === this.messages.length - 1;

                  let messageContent;

                  if (isUser) {
                    // prettier-ignore
                    messageContent = html`<div style="white-space: pre-wrap;">${message.parts[0]?.text}</div>`;
                  } else {
                    const isGenerating = isLastMessage && this.isLoading;
                    if (isGenerating) {
                      // prettier-ignore
                      messageContent = html`<div style="white-space: pre-wrap;">${message.parts[0]?.text}</div>`;
                    } else {
                      messageContent = this.renderMarkdown(
                        message.parts[0]?.text || "...",
                      );
                    }
                  }

                  return html`
                    <div
                      class="message-bubble ${classMap({
                        user: isUser,
                        assistant: isAssistant,
                      })}"
                    >
                      <span class="role">
                        ${isUser ? "You" : assistantName}:
                      </span>
                      <div class="message-content">${messageContent}</div>
                    </div>
                  `;
                },
              )
            : ""}
          ${this.isLoading || isLLMLoading
            ? html`
                <div class="loading-indicator">
                  <div class="spinner"></div>
                  <span class="loading-text">
                    ${isLLMLoading
                      ? `Downloading: ${loadingMsg}`
                      : "Thinking..."}
                  </span>
                </div>
              `
            : ""}

          <div id="messages-end" class="messages-end"></div>
        </div>

        <form @submit="${this.handleSubmit}">
          <input
            .value="${live(this.input)}"
            @input="${this.handleInput}"
            type="text"
            placeholder="${!isLLMReady
              ? isLLMError
                ? "An error occurred"
                : isLLMPending
                  ? INITIAL_INPUT_PLACEHOLDER
                  : "Loading model..."
              : this.isLoading
                ? "Waiting for response..."
                : "Enter your message..."}"
            ?disabled="${this.isLoading || !isLLMReady}"
          />
          <button
            type="submit"
            ?disabled="${this.isLoading || !this.input.trim() || !isLLMReady}"
          >
            ${this.isLoading ? "Sending" : !isLLMReady ? "Waiting" : "Send"}
          </button>
        </form>
      </div>
    `;
  }

  static styles = css`
    :host {
      display: block;
      box-sizing: border-box;
      --border: 1px solid #ccc;
      --background-color: #fff;
      --border-radius: 8px;
      --box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
      --primary-color: #007aff;
      --assistant-bg: #f0f0f5;
    }

    *,
    *::before,
    *::after {
      box-sizing: border-box;
    }

    .message-content {
      font-family:
        -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial,
        sans-serif;
      margin: 0;
      font-size: 0.95rem;
      /* デフォルトで行間を適度に */
      line-height: 1.5;
    }

    .message-content p {
      margin: 0 0 0.5em 0;
    }
    .message-content p:last-child {
      margin-bottom: 0;
    }

    .message-content ul,
    .message-content ol {
      margin: 0.5em 0;
      padding-left: 1.5em;
    }

    .message-content pre {
      background: rgba(0, 0, 0, 0.05);
      padding: 8px;
      border-radius: 4px;
      overflow-x: auto;
      font-family: monospace;
    }

    .message-content code {
      font-family: monospace;
      background: rgba(0, 0, 0, 0.05);
      padding: 2px 4px;
      border-radius: 3px;
    }

    .message-content blockquote {
      border-left: 3px solid #ccc;
      margin: 0.5em 0;
      padding-left: 0.5em;
      color: #555;
    }

    .table-container {
      overflow-x: auto;
      margin: 10px 0;
      border-radius: 4px;
      border: 1px solid #ddd;
    }

    .message-content table {
      width: 100%;
      border-collapse: collapse;
      font-size: 0.9em;
    }

    .message-content th,
    .message-content td {
      padding: 8px 12px;
      border: 1px solid #ddd;
      line-height: 1.4;
    }

    .message-content th {
      background-color: #f5f5f5;
      font-weight: bold;
    }

    .message-content tr:nth-child(even) {
      background-color: #fafafa;
    }

    .chat-container {
      display: flex;
      flex-direction: column;
      height: 80vh;
      height: 80dvh;
      border: var(--border);
      border-radius: var(--border-radius);
      overflow: hidden;
      width: 100%;
      background-color: var(--background-color);

      .message-list {
        display: flex;
        flex-direction: column;
        gap: 16px;
        flex-grow: 1;
        overflow-y: auto;
        padding: 15px;
        scroll-behavior: auto;
      }

      button {
        padding: 10px 15px;
        border: none;
        border-radius: 4px;
        cursor: pointer;
        font-weight: 500;
        transition: opacity 0.2s;
      }

      button[type="submit"] {
        background-color: var(--primary-color);
        color: white;
      }

      button:disabled {
        background-color: #ccc;
        cursor: not-allowed;
      }

      form {
        display: flex;
        gap: 10px;
        padding: 10px;
        border-top: var(--border);
        background-color: #fff;
      }

      input[type="text"] {
        flex-grow: 1;
        padding: 10px;
        border: 1px solid #ddd;
        border-radius: 4px;
        outline: none;
        font-size: 16px;
      }

      input[type="text"]:focus {
        border-color: var(--primary-color);
      }

      .initial-warning {
        border-radius: var(--border-radius);
        box-shadow: var(--box-shadow);
        overflow-wrap: anywhere;
        padding: 15px;
        background-color: #fff3cd;
        color: #856404;
        margin-bottom: 10px;
      }

      .initial-warning h2 {
        font-size: 1rem;
        font-weight: bold;
        margin-top: 0;
      }

      .agree-button {
        background-color: #28a745;
        color: white;
        width: 100%;
        margin-top: 10px;
      }
    }

    .message-bubble {
      max-width: 85%;
      padding: 10px 16px;
      border-radius: 18px;
      box-shadow: 0 1px 2px rgba(0, 0, 0, 0.05);
      overflow-wrap: anywhere;
      line-height: 1.5;
    }

    .user {
      align-self: flex-end;
      background-color: var(--primary-color);
      color: white;
      border-bottom-right-radius: 4px;
    }

    .user .message-content a {
      color: #fff;
    }

    .assistant {
      align-self: flex-start;
      background-color: var(--assistant-bg);
      color: #1a1a1a;
      border-bottom-left-radius: 4px;
    }

    .role {
      font-size: 0.75rem;
      font-weight: bold;
      display: block;
      margin-bottom: 4px;
      opacity: 0.8;
    }

    .loading-indicator {
      align-self: flex-start;
      margin: 0;
      padding: 4px 12px;
      display: flex;
      align-items: center;
      gap: 8px;
      font-size: 0.85rem;
      color: #8e8e93;
      font-weight: 500;
    }

    .spinner {
      border: 2px solid rgba(0, 0, 0, 0.1);
      border-left-color: var(--primary-color);
      border-radius: 50%;
      width: 14px;
      height: 14px;
      animation: spin 1s linear infinite;
    }

    @keyframes spin {
      100% {
        transform: rotate(360deg);
      }
    }
  `;
}
