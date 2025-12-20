import { atom } from "nanostores";
import {
  CreateMLCEngine,
  type MLCEngineInterface,
  type InitProgressCallback,
  type InitProgressReport,
} from "@mlc-ai/web-llm";

export type LLMState =
  | { status: "uninitialized" }
  | { status: "pending" }
  | { status: "loading"; progress: number; message: string }
  | { status: "ready"; chat: MLCEngineInterface }
  | { status: "error"; error: string };

export const llmStore = atom<LLMState>({ status: "pending" });

const DEFAULT_MODEL_ID = "Hermes-3-Llama-3.2-3B-q4f16_1-MLC";

export const startLLMInitialization = async (
  modelId: string = DEFAULT_MODEL_ID,
) => {
  llmStore.set({
    status: "loading",
    progress: 0,
    message: "Creating MLCEngine...",
  });

  const initProgressCallback: InitProgressCallback = (
    progress: InitProgressReport,
  ) => {
    llmStore.set({
      status: "loading",
      progress: progress.progress,
      message: progress.text,
    });
  };

  try {
    const engine: MLCEngineInterface = await CreateMLCEngine(
      modelId,
      {
        initProgressCallback,
      },
      { context_window_size: 8192 },
    );

    llmStore.set({ status: "ready", chat: engine });
  } catch (error) {
    console.error("WebLLM initialization failed:", error);
    llmStore.set({
      status: "error",
      error: error instanceof Error ? error.message : "Unknown error",
    });
  }
};
