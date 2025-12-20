import { LitElement, html, css, PropertyValues } from "lit";
import { customElement, state, query } from "lit/decorators.js";
import { classMap } from "lit/directives/class-map.js";
import { repeat } from "lit/directives/repeat.js";

// Nano Stores連携ライブラリ
import { StoreController } from "@nanostores/lit";
// ストアとアクションをインポート
import { llmStore, startLLMInitialization } from "../lib/llm.js";

import type { ChatCompletionMessageParam } from "@mlc-ai/web-llm";
import FaqContent from "../pages/faq.md?raw";

const assistantName = "LBRL Buddy";

type SimpleMessage = {
  id: string;
  role: "user" | "assistant";
  parts: { type: "text"; text: string }[];
  metadata?: { createdAt: Date };
};

@customElement("br-lite-buddy")
export class BrLiteBuddy extends LitElement {
  // --- Stores ---
  // StoreControllerを使うことで、llmStoreの変更を検知して自動で再レンダリングします
  private llmState = new StoreController(this, llmStore);

  // --- Local State ---
  @state() private messages: SimpleMessage[] = [];
  @state() private input: string = "";
  @state() private isLoading: boolean = false;

  @query("#messages-end") private messagesEnd?: HTMLDivElement;

  private createMessage(
    role: "user" | "assistant",
    text: string,
  ): SimpleMessage {
    return {
      id: Date.now().toString() + "-" + role,
      role,
      parts: [{ type: "text", text }],
      metadata: { createdAt: new Date() },
    };
  }

  constructor() {
    super();
    this.messages = [
      this.createMessage(
        "assistant",
        `Hi there! I'm ${assistantName}, your interactive support assistant. Feel free to ask me anything about the knowledge base, and I'll do my best to help you out! 🤖`,
      ),
    ];
  }

  updated(changedProperties: PropertyValues) {
    if (
      changedProperties.has("messages") ||
      changedProperties.has("isLoading")
    ) {
      this.messagesEnd?.scrollIntoView({ behavior: "smooth" });
    }
  }

  // --- Handlers ---

  private handleAgreeAndStart() {
    // llmStore.value で現在の状態にアクセスできます
    if (this.llmState.value.status === "pending") {
      startLLMInitialization();
    }
  }

  private async handleSubmit(e: Event) {
    e.preventDefault();
    const currentState = this.llmState.value; // ストアの値を取得

    const isLLMReady = currentState.status === "ready";
    if (!this.input.trim() || this.isLoading || !isLLMReady) return;

    const userMessageContent = this.input;
    this.messages = [
      ...this.messages,
      this.createMessage("user", userMessageContent),
    ];
    this.input = "";

    // チャットエンジンを渡して処理開始
    if (currentState.status === "ready") {
      await this.handleWebLLMChat(userMessageContent, currentState.chat);
    }
  }

  // --- Core Logic ---

  // chatインターフェースを引数で受け取る形に変更
  private async handleWebLLMChat(userMessageContent: string, engine: any) {
    let assistantResponse = "";
    this.isLoading = true;

    const systemPrompt = `
You are ${assistantName}, a friendly and helpful interactive support assistant.
Your primary goal is to provide clear, concise, and structured answers to the user's questions, prioritizing the information contained within the "KNOWLEDGE BASE" provided below.

--- KNOWLEDGE BASE (FAQ) ---
${FaqContent}
---------------------------
`;

    try {
      const messagesToSend = [
        { role: "system", content: systemPrompt },
        { role: "user", content: userMessageContent },
      ] satisfies ChatCompletionMessageParam[];

      this.messages = [...this.messages, this.createMessage("assistant", "")];

      const responseStream = await engine.chat.completions.create({
        messages: messagesToSend,
        stream: true,
      });

      for await (const chunk of responseStream) {
        const content = chunk.choices[0]?.delta.content;
        if (content) {
          assistantResponse += content;
          const newMessages = [...this.messages];
          newMessages[newMessages.length - 1] = this.createMessage(
            "assistant",
            assistantResponse,
          );
          this.messages = newMessages;
        }
      }
    } catch (error) {
      console.error("LLM conversation failed:", error);
      const newMessages = [...this.messages];
      newMessages[newMessages.length - 1] = this.createMessage(
        "assistant",
        "Error: LLM failed.",
      );
      this.messages = newMessages;
    } finally {
      this.isLoading = false;
    }
  }

  // --- Rendering ---

  render() {
    // this.llmState.value で常に最新の状態が取れます
    const state = this.llmState.value;

    const isLLMReady = state.status === "ready";
    const isLLMLoading = state.status === "loading";
    const isLLMPending = state.status === "pending";
    const isLLMError = state.status === "error";

    // ローディング中のメッセージ取得
    const loadingMsg = isLLMLoading ? state.message : "";

    return html`
      <div class="chat-container">
        <div class="message-list">
          ${isLLMPending
            ? html`
                <div class="initial-warning message-bubble assistant">
                  <span class="role">System Warning:</span>
                  <p>
                    To use this chat feature, you must download the LLM model.
                  </p>
                  <button
                    class="agree-button"
                    @click="${this.handleAgreeAndStart}"
                  >
                    Agree and Start Model Download
                  </button>
                </div>
              `
            : ""}
          ${!isLLMLoading && !isLLMPending
            ? repeat(
                this.messages,
                (msg) => msg.id,
                (message) => html`
                  <div
                    class="message-bubble ${classMap({
                      user: message.role === "user",
                      assistant: message.role === "assistant",
                    })}"
                  >
                    <span class="role">
                      ${message.role === "user" ? "You" : assistantName}:
                    </span>
                    <pre style="white-space: pre-wrap; font-family: inherit;">
${message.parts[0]?.text || "..."}</pre
                    >
                  </div>
                `,
              )
            : ""}
          <div id="messages-end" style="height: 0;"></div>
        </div>

        ${this.isLoading || isLLMLoading
          ? html`
              <div class="loading-indicator">
                <div class="spinner-container">
                  <div><div class="spinner"></div></div>
                  <p class="loading-text">
                    ${isLLMLoading
                      ? `Downloading: ${loadingMsg}`
                      : "Thinking..."}
                  </p>
                </div>
              </div>
            `
          : ""}

        <form @submit="${this.handleSubmit}">
          <input
            .value="${this.input}"
            @input="${(e: any) => (this.input = e.target.value)}"
            type="text"
            placeholder="${!isLLMReady
              ? isLLMError
                ? "An error occurred"
                : isLLMPending
                  ? "Start download first"
                  : "Loading..."
              : this.isLoading
                ? "Waiting..."
                : "Enter message..."}"
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

  // スタイルは前回と同じため省略
  static styles = css`
    :host {
      display: block;
    }
    .chat-container {
      display: flex;
      flex-direction: column;
      height: 80vh;
      max-width: 600px;
      margin: 0 auto;
      border: 1px solid #ccc;
      border-radius: 8px;
      overflow: hidden;
      font-family: sans-serif;
    }
    .message-list {
      flex-grow: 1;
      padding: 15px;
      overflow-y: auto;
      background-color: #f9f9f9;
    }
    .message-bubble {
      margin-bottom: 10px;
      padding: 8px 12px;
      border-radius: 18px;
      max-width: 80%;
      word-wrap: break-word;
      box-shadow: 0 1px 1px rgba(0, 0, 0, 0.05);
    }
    .user {
      align-self: flex-end;
      background-color: #007aff;
      color: white;
      margin-left: auto;
    }
    .assistant {
      align-self: flex-start;
      background-color: #e5e5ea;
      color: #000;
    }
    .role {
      font-size: 0.8em;
      font-weight: bold;
      display: block;
      margin-bottom: 3px;
      opacity: 0.7;
    }
    .initial-warning {
      background-color: #fff3cd;
      color: #856404;
      border: 1px solid #ffeeba;
      padding: 15px;
      border-radius: 8px;
      max-width: 100%;
      margin-bottom: 20px;
    }
    .agree-button {
      background-color: #28a745;
      color: white;
      padding: 10px 15px;
      border: none;
      border-radius: 4px;
      cursor: pointer;
      margin-top: 10px;
      display: block;
      width: 100%;
    }
    .loading-indicator {
      margin: 5px 15px 10px 15px;
      padding: 8px 12px;
      align-self: flex-start;
      background-color: #e5e5ea;
      color: #000;
      border-radius: 18px;
      box-shadow: 0 1px 1px rgba(0, 0, 0, 0.05);
      max-width: 50%;
      display: inline-flex;
      align-items: center;
      gap: 10px;
    }
    .spinner-container {
      display: flex;
      align-items: center;
      gap: 8px;
    }
    .spinner {
      border: 4px solid rgba(0, 0, 0, 0.1);
      border-left-color: #007bff;
      border-radius: 50%;
      width: 20px;
      height: 20px;
      animation: spin 1s linear infinite;
    }
    @keyframes spin {
      0% {
        transform: rotate(0deg);
      }
      100% {
        transform: rotate(360deg);
      }
    }
    .loading-text {
      margin: 0;
      font-size: 0.9em;
      color: #333;
    }
    form {
      display: flex;
      padding: 10px;
      border-top: 1px solid #ccc;
      background-color: white;
    }
    input {
      flex-grow: 1;
      padding: 10px;
      border: 1px solid #ddd;
      border-radius: 4px;
      margin-right: 10px;
    }
    button {
      padding: 10px 15px;
      background-color: #007aff;
      color: white;
      border: none;
      border-radius: 4px;
      cursor: pointer;
    }
    button:disabled {
      background-color: #999;
      cursor: not-allowed;
    }
  `;
}
