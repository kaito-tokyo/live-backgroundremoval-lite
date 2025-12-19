import { atom } from "nanostores";

export interface FileVerifierDroppedFileResult {
  name: string;
  size: number;
  sha256?: string;
  error?: unknown;
}

export const fileVerifiedResultStore = atom<FileVerifierDroppedFileResult[]>(
  [],
);
