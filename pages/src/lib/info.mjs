// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

export const GITHUB_OWNER = "kaito-tokyo";
export const GITHUB_REPO = "live-backgroundremoval-lite";
export const GITHUB_URL = `https://github.com/${GITHUB_OWNER}/${GITHUB_REPO}`;
export const PRODUCTION_BASE_URL =
  "https://kaito-tokyo.github.io/live-backgroundremoval-lite";

export const MATRIX_CHATROOM_URL = "https://matrix.to/#/#live-backgroundremoval-lite:matrix.org";

/**
 * @param {string} version
 * @return {boolean}
 */
export function isManifestEnabled(version) {
  const [major] = version.split(".");
  return Number(major) >= 4;
}

export async function getAggregateRating() {
  return {
    "@type": "AggregateRating",
    ratingValue: "4.0",
    reviewCount: "5",
    bestRating: "5",
    worstRating: "1",
  };
}
