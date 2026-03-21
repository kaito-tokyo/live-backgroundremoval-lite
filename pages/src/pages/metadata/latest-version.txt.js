// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

import { Octokit } from "@octokit/core";

import { GITHUB_OWNER, GITHUB_REPO } from "../../lib/info.mjs";

export async function GET() {
  const octokit = new Octokit({ auth: import.meta.env.GITHUB_TOKEN });

  const {
    data: { tag_name },
  } = await octokit.request("GET /repos/{owner}/{repo}/releases/latest", {
    owner: GITHUB_OWNER,
    repo: GITHUB_REPO,
  });

  if (!tag_name) {
    throw new Error("Failed to fetch the latest release tag name.");
  }

  return new Response(tag_name.trimEnd() + "\n", {
    status: 200,
    headers: {
      "Content-Type": "text/plain",
    },
  });
}
