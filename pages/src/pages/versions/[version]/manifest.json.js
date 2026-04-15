// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

import { Octokit } from "@octokit/core";
import { paginateRest } from "@octokit/plugin-paginate-rest";

import { getRepoContents, listReleases } from "../../../lib/github.mjs";
import { isManifestEnabled } from "../../../lib/info.mjs";
import { sortStringify } from "../../../lib/sort-stringify.mjs";

export async function getStaticPaths() {
  const MyOctokit = Octokit.plugin(paginateRest);
  const octokit = new MyOctokit({ auth: import.meta.env.GITHUB_TOKEN });

  const allReleases = await listReleases(octokit);

  /** @type {import("astro").GetStaticPathsResult} */
  const staticPaths = [];

  for (const { tag_name: version } of allReleases) {
    let compactManifest = sortStringify({ name: "manifest missing", version });

    if (isManifestEnabled(version)) {
      const data = await getRepoContents(octokit, "data/manifest.json", version);
      if (data.type === "file" && data.content) {
        try {
          compactManifest = sortStringify(JSON.parse(data.content));
        } catch (e) {
          console.debug(`Invalid manifest for ${version}:`, e);
        }
      }
    }

    staticPaths.push({
      params: { version },
      props: { compactManifest },
    });
  }

  return staticPaths;
}

/**
 * @param {{props: {compactManifest: string}}} params
 */
export async function GET({ props }) {
  return new Response(props.compactManifest.trimEnd() + "\n", {
    status: 200,
    headers: {
      "Content-Type": "application/json",
    },
  });
}
