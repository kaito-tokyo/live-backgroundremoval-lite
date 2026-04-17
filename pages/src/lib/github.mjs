// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

import { GITHUB_OWNER, GITHUB_REPO } from "./info.mjs";

/**
 * @param {import("@octokit/core").Octokit} octokit
 * @returns {Promise<string>}
 */
export async function getRepoContents(octokit, path, ref, owner = GITHUB_OWNER, repo = GITHUB_REPO) {
  const res = await octokit.request(
    "GET /repos/{owner}/{repo}/contents/{path}",
    {
      owner,
      repo,
      path,
      ref,
      headers: {
        Accept: "application/vnd.github.raw+json",
        "X-GitHub-Api-Version": "2026-03-10",
      },
    },
  );
  if (typeof res.data === "string") {
    return res.data;
  } else {
    throw new Error("InvalidResponseDataError");
  }
}

/**
 * @param {import("@octokit/core").Octokit & { paginate: import("@octokit/plugin-paginate-rest").PaginateInterface }} octokit
 */
export async function getLatestRelease(octokit, owner = GITHUB_OWNER, repo = GITHUB_REPO) {
  const { data } = await octokit.request(
    "GET /repos/{owner}/{repo}/releases/latest",
    /** @type {{ owner: string; repo: string }} */ ({
      owner,
      repo,
      headers: {
        Accept: "application/vnd.github+json",
        "X-GitHub-Api-Version": "2026-03-10"
      },
    }),
  );
  return data;
}

/**
 * @param {import("@octokit/core").Octokit & { paginate: import("@octokit/plugin-paginate-rest").PaginateInterface }} octokit
 */
export async function listReleases(octokit, owner = GITHUB_OWNER, repo = GITHUB_REPO) {
  return await octokit.paginate(
    "GET /repos/{owner}/{repo}/releases",
    /** @type {{ owner: string; repo: string }} */ ({
      owner,
      repo,
      headers: {
        Accept: "application/vnd.github+json",
        "X-GitHub-Api-Version": "2026-03-10"
      },
    }),
  );
}
