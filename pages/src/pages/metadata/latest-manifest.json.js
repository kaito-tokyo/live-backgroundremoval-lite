// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

/// <reference types="astro/client" />

import { Octokit } from "@octokit/core";

import {
  GITHUB_OWNER,
  GITHUB_REPO,
  isManifestEnabled,
} from "../../lib/info.mjs";
import { sortStringify } from "../../lib/sort-stringify.mjs";

export async function GET() {
  const octokit = new Octokit({ auth: import.meta.env.GITHUB_TOKEN });

  const {
    data: { tag_name },
  } = await octokit.request("GET /repos/{owner}/{repo}/releases/latest", {
    owner: GITHUB_OWNER,
    repo: GITHUB_REPO,
  });

  if (!isManifestEnabled(tag_name)) {
    const manifestMissing = {
      name: "manifest missing",
      version: tag_name,
    };

    return new Response(sortStringify(manifestMissing), {
      status: 200,
      headers: {
        "Content-Type": "application/json",
      },
    });
  }

  const { data } = await octokit.request(
    "GET /repos/{owner}/{repo}/contents/{path}",
    {
      owner: GITHUB_OWNER,
      repo: GITHUB_REPO,
      path: "data/manifest.json",
      ref: tag_name,
      headers: {
        Accept: "application/vnd.github.raw+json",
        "X-GitHub-Api-Version": "2026-03-10",
      },
    },
  );

  if (!data || typeof data !== "string") {
    throw new Error("Failed to fetch the manifest.json content.");
  }

  const manifest = JSON.parse(data);

  return new Response(sortStringify(manifest).trimEnd() + "\n", {
    status: 200,
    headers: {
      "Content-Type": "application/json",
    },
  });
}
