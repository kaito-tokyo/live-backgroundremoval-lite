import { LitElement, html, css, type PropertyValues } from 'lit';
import { customElement, state, query } from 'lit/decorators.js';
import { classMap } from 'lit/directives/class-map.js';
import { repeat } from 'lit/directives/repeat.js';
import { live } from 'lit/directives/live.js';
import { StoreController } from '@nanostores/lit';

// 拡張子 .js を忘れずに
import { llmStore, startLLMInitialization } from '../stores/llm.js';

// ?raw でインポートする場合、env.d.ts での型定義が必要です
import FaqContent from '../pages/faq.md?raw';
import type { ChatCompletionMessageParam } from "@mlc-ai/web-llm";

const assistantName = "BR Lite Buddy";

type SimpleMessage = {
  id: string;
  role: "user" | "assistant";
  parts: { type: "text"; text: string }[];
  metadata?: { createdAt: Date };
};

@customElement('lbrl-buddy')
export class LBRLBuddy extends LitElement {
  private llmState = new StoreController(this, llmStore);

  // ▼ accessor は削除し、通常のプロパティとして定義
  @state()
  private messages: SimpleMessage[] = [
    this.createMessage(
      "assistant",
      `Hi there! I'm ${assistantName}, your interactive support assistant. Feel free to ask me anything about the knowledge base, and I'll do my best to help you out! 🤖`
    )
  ];

  @state()
  private input: string = "";

  @state()
  private isLoading: boolean = false;

  @query('#messages-end')
  private messagesEnd?: HTMLDivElement;

  // メッセージ作成ヘルパー
  private createMessage(role: "user" | "assistant", text: string): SimpleMessage {
    return {
      id: Date.now().toString() + "-" + role,
      role,
      parts: [{ type: "text", text }],
      metadata: { createdAt: new Date() },
    };
  }

  updated(changedProperties: PropertyValues) {
    if (changedProperties.has('messages') || changedProperties.has('isLoading')) {
      this.messagesEnd?.scrollIntoView({ behavior: "smooth" });
    }
  }

  private handleAgreeAndStart() {
    if (this.llmState.value.status === 'pending') {
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

    // 配列を新しく生成して代入することでLitの更新をトリガーします
    this.messages = [...this.messages, this.createMessage("user", userMessageContent)];
    this.input = "";

    if (currentState.status === "ready") {
       await this.handleWebLLMChat(userMessageContent, currentState.chat);
    }
  }

  private async handleWebLLMChat(userMessageContent: string, engine: any) {
    let assistantResponse = "";
    this.isLoading = true;

    const systemPrompt = `
You are ${assistantName}, a friendly and helpful interactive support assistant.
Your primary goal is to provide clear, concise, and structured answers to the user's questions, prioritizing the information contained within the "KNOWLEDGE BASE" provided below.

When answering, please adhere to these guidelines:
1.  **Tone and Style:** Be friendly, encouraging, and easy to understand.
2.  **Source Priority:**
    * **Always prioritize** facts and details found in the KNOWLEDGE BASE.
    * If the KNOWLEDGE BASE **does not explicitly contain** the necessary information, you may use your general knowledge, provided you maintain a supportive tone.
3.  **Attribution:** When the answer is based on the KNOWLEDGE BASE, use a natural phrasing such as "**As far as I know from my resources**," or "**Based on the information I have**," to introduce the answer, instead of using phrases like "based ONLY on the KNOWLEDGE BASE."
4.  **Clarity and Structure:** Never quote the Q&A text verbatim, and do not use internal document formatting like 'Q:', 'A:', '###', or '**'. Use bullet points, short paragraphs, or numbered lists to present the information clearly.
5.  **Completeness:** Synthesize the most relevant facts from the knowledge base (or general knowledge if needed) to fully address the user's inquiry.

Your knowledge base is provided below. Always refer to this knowledge first when answering questions related to it.

--- KNOWLEDGE BASE (FAQ) ---
${FaqContent}
---------------------------
`;

    try {
      const messagesToSend = [
        { role: "system", content: systemPrompt },
        { role: "user", content: userMessageContent },
      ] satisfies ChatCompletionMessageParam[];

      // アシスタントの空メッセージを追加（ストリーミング表示用）
      this.messages = [...this.messages, this.createMessage("assistant", "")];

      const responseStream = await engine.chat.completions.create({
        messages: messagesToSend,
        stream: true,
      });

      for await (const chunk of responseStream) {
        const content = chunk.choices[0]?.delta.content;
        if (content) {
          assistantResponse += content;
          // 最後のメッセージを更新
          const newMessages = [...this.messages];
          newMessages[newMessages.length - 1] = this.createMessage("assistant", assistantResponse);
          this.messages = newMessages;
        }
      }
    } catch (error) {
      console.error("LLM conversation failed:", error);
      const newMessages = [...this.messages];
      newMessages[newMessages.length - 1] = this.createMessage("assistant", "Error: LLM failed to generate a response.");
      this.messages = newMessages;
    } finally {
      this.isLoading = false;
    }
  }

  render() {
    const state = this.llmState.value;
    const isLLMReady = state.status === "ready";
    const isLLMLoading = state.status === "loading";
    const isLLMPending = state.status === "pending";
    const isLLMError = state.status === "error";
    const loadingMsg = isLLMLoading ? state.message : "";

    return html`
      <div style="background:#333; color:#fff; padding:10px; font-family:monospace; font-size:12px;">
        <p>DEBUG INFO:</p>
        <ul>
          <li>Input Text: "${this.input}"</li>
          <li>Input Disabled?: ${this.isLoading || !isLLMReady}</li>
          <li>LLM Status: ${state.status}</li>
        </ul>
      </div>

      <div class="chat-container">
        <div class="message-list">

          ${isLLMPending ? html`
            <div class="initial-warning message-bubble assistant">
              <span class="role">System Warning:</span>
              <p>
                To use this chat feature, you must download the LLM model (a large
                file of several GBs). This is only required on the first launch, but
                it may take several minutes to complete (depending on your internet
                speed).
              </p>
              <button class="agree-button" @click="${this.handleAgreeAndStart}">
                Agree and Start Model Download
              </button>
            </div>
          ` : ''}

          ${!isLLMLoading && !isLLMPending ? repeat(
            this.messages,
            (msg) => msg.id,
            (message) => html`
              <div class="message-bubble ${classMap({
                  user: message.role === 'user',
                  assistant: message.role === 'assistant'
                })}">
                <span class="role">
                  ${message.role === 'user' ? 'You' : assistantName}:
                </span>
                <pre class="message-content">${message.parts[0]?.text || "..."}</pre>
              </div>
            `
          ) : ''}

          <div id="messages-end" class="messages-end"></div>
        </div>

        ${this.isLoading || isLLMLoading ? html`
          <div class="loading-indicator">
            <div class="spinner-container">
              <div><div class="spinner"></div></div>
              <p class="loading-text">
                ${isLLMLoading ? `Downloading: ${loadingMsg}` : "Thinking..."}
              </p>
            </div>
          </div>
        ` : ''}

        <form @submit="${this.handleSubmit}">
          <input
            .value="${live(this.input)}"
            @input="${this.handleInput}"
            type="text"
            placeholder="${
              !isLLMReady
                ? isLLMError ? "An error occurred" : isLLMPending ? "Please press the start button" : "Loading model..."
                : this.isLoading ? "Waiting for response..." : "Enter your message..."
            }"
            ?disabled="${this.isLoading || !isLLMReady}"
          />
          <button type="submit" ?disabled="${this.isLoading || !this.input.trim() || !isLLMReady}">
            ${this.isLoading ? "Sending" : !isLLMReady ? "Waiting" : "Send"}
          </button>
        </form>
      </div>
    `;
  }

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
    .message-content {
      white-space: pre-wrap;
      font-family: inherit;
      margin: 0;
    }
    .messages-end {
      height: 0;
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
    .initial-warning p {
      margin-bottom: 10px;
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
      transition: background-color 0.2s;
    }
    .agree-button:hover {
      background-color: #218838;
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
      0% { transform: rotate(0deg); }
      100% { transform: rotate(360deg); }
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
      transition: background-color 0.2s;
    }
    button:disabled {
      background-color: #999;
      cursor: not-allowed;
    }
  `;
}
