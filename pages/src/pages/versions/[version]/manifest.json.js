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

  for (const release of allReleases) {
    if (release.draft || !release.published_at) {
      continue;
    }

    const version = release.tag_name;

    let compactManifest = sortStringify({ name: "manifest missing", version });

    if (isManifestEnabled(version)) {
      const data = await getRepoContents(octokit, "data/manifest.json", version);
      try {
        compactManifest = sortStringify(JSON.parse(data));
      } catch (e) {
        console.error(`Malformed manifest for ${version}:`, e);
        throw e;
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
