<script context="module" lang="ts">
  export interface FileVerifierReleaseAsset {
    name: string;
    size: number;
    digest: string | null;
  }
</script>

<script lang="ts">
  import { fileVerifiedResultStore } from "../stores/FileVerifierStore.js";

  export let id: string;
  export let releaseAssets: FileVerifierReleaseAsset[];

  function findExpectedAsset(
    sha256: string,
  ): FileVerifierReleaseAsset | undefined {
    const matchedAsset = releaseAssets.find(
      (asset) => asset.digest === `sha256:${sha256}`,
    );
    return matchedAsset;
  }
</script>

<dialog {id}>
  <dl>
    {#each $fileVerifiedResultStore as item}
      {#if item.sha256}
        {@const expectedAsset = findExpectedAsset(item.sha256)}
        {#if expectedAsset}
          <dt>
            <strong>{item.name}</strong>
          </dt>
          <dd>✅ File name on our server: {expectedAsset.name}</dd>
          <dd>✅ Hash matched: {item.sha256}</dd>
          {#if expectedAsset.size === item.size}
            <dd>✅ File size matched: {item.size} bytes</dd>
          {:else}
            <dd>
              ❌ File size mismatch: expected {expectedAsset.size} bytes, got {item.size}
              bytes
            </dd>
          {/if}
        {:else}
          <dt><strong>{item.name}</strong></dt>
          <dd>❌ Unknown file!</dd>
        {/if}
      {:else}
        <dt><strong>{item.name}</strong></dt>
        <dd>❌ Verification failed: {item.error}</dd>
      {/if}
    {:else}
      <dt>No files received.</dt>
    {/each}
  </dl>

  <form method="dialog" style="text-align: right;">
    <button>Close</button>
  </form>
</dialog>
