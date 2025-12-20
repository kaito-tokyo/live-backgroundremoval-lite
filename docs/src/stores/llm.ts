// src/stores/llm.ts
import { atom } from "nanostores";
import {
  CreateMLCEngine,
  type MLCEngineInterface,
  type InitProgressCallback,
  type InitProgressReport,
} from "@mlc-ai/web-llm";

// LLM execution state type
export type LLMState =
  | { status: "uninitialized" }
  | { status: "pending" }
  | { status: "loading"; progress: number; message: string }
  | { status: "ready"; chat: MLCEngineInterface }
  | { status: "error"; error: string };

// 1. ストアの作成 (初期値: pending)
export const llmStore = atom<LLMState>({ status: "pending" });

// デフォルトのモデルID
const DEFAULT_MODEL_ID = "Hermes-3-Llama-3.2-3B-q4f16_1-MLC";

/**
 * Starts MLCEngine initialization and model downloading.
 * @param modelId The ID of the model to load.
 */
export const startLLMInitialization = async (
  modelId: string = DEFAULT_MODEL_ID,
) => {
  // ストアの値を更新 (loading)
  llmStore.set({
    status: "loading",
    progress: 0,
    message: "Creating MLCEngine...",
  });

  const initProgressCallback: InitProgressCallback = (
    progress: InitProgressReport,
  ) => {
    // 読み込み状況の更新
    // 現在の状態を取得したい場合は llmStore.get() を使いますが、
    // ここでは新しいオブジェクトで上書きするため直接 set します
    llmStore.set({
      status: "loading",
      progress: progress.progress,
      message: progress.text,
    });
  };

  try {
    const engine: MLCEngineInterface = await CreateMLCEngine(modelId, {
      initProgressCallback,
    });

    // 完了 (ready)
    llmStore.set({ status: "ready", chat: engine });
  } catch (error) {
    console.error("WebLLM initialization failed:", error);
    llmStore.set({
      status: "error",
      error: error instanceof Error ? error.message : "Unknown error",
    });
  }
};
