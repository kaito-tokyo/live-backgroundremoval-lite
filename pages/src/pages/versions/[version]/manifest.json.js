// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

/// <reference types="astro/client" />

import { Octokit } from "@octokit/core";
import { composePaginateRest } from "@octokit/plugin-paginate-rest";

import {
  GITHUB_OWNER,
  GITHUB_REPO,
  isManifestEnabled,
} from "../../../lib/info.mjs";
import { sortStringify } from "../../../lib/sort-stringify.mjs";

/**
 * @param {import("astro").GetStaticPathsOptions} options
 * @returns {Promise<import("astro").GetStaticPathsResult>}
 */
export async function getStaticPaths(options) {
  const octokit = new Octokit({ auth: import.meta.env.GITHUB_TOKEN });

  const iterator = composePaginateRest.iterator(
    octokit,
    "GET /repos/{owner}/{repo}/releases",
    {
      owner: GITHUB_OWNER,
      repo: GITHUB_REPO,
      headers: {
        "X-GitHub-Api-Version": "2026-03-10",
      },
    },
  );

  /** @type {import("@octokit/types").Endpoints["GET /repos/{owner}/{repo}/releases"]["response"]["data"]} */
  const allReleases = [];
  for await (const { data } of iterator) {
    if (!data || !Array.isArray(data)) {
      throw new Error("Failed to fetch releases data.");
    }

    allReleases.push(...data);
  }

  const staticPaths = [];
  for (const release of allReleases) {
    if (isManifestEnabled(release.tag_name)) {
      const { data } = await octokit.request(
        "GET /repos/{owner}/{repo}/contents/{path}",
        {
          owner: GITHUB_OWNER,
          repo: GITHUB_REPO,
          path: "data/manifest.json",
          ref: release.tag_name,
          headers: {
            Accept: "application/vnd.github.raw+json",
            "X-GitHub-Api-Version": "2026-03-10",
          },
        },
      );

      if (!data || typeof data !== "string") {
        throw new Error(
          `Failed to fetch the manifest.json content for release ${release.tag_name}.`,
        );
      }

      staticPaths.push({
        params: {
          version: release.tag_name,
        },
        props: {
          compactManifest: sortStringify(JSON.parse(data)),
        },
      });
    } else {
      staticPaths.push({
        params: {
          version: release.tag_name,
        },
        props: {
          compactManifest: sortStringify({
            name: "manifest missing",
            version: release.tag_name,
          }),
        },
      });
    }
  }

  return staticPaths;
}

/**
 * @param {import("astro").APIContext<{compactManifest: string}>} params
 * @returns {Promise<Response>}
 */
export async function GET({ props: { compactManifest } }) {
  return new Response(compactManifest.trimEnd() + "\n", {
    status: 200,
    headers: {
      "Content-Type": "application/json",
    },
  });
}
