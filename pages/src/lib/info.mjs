// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

export const GITHUB_OWNER = "kaito-tokyo";
export const GITHUB_REPO = "live-backgroundremoval-lite";
export const GITHUB_URL = `https://github.com/${GITHUB_OWNER}/${GITHUB_REPO}`;
export const PRODUCTION_BASE_URL =
  "https://kaito-tokyo.github.io/live-backgroundremoval-lite";

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
    ratingValue: "4.5",
    reviewCount: "4",
    bestRating: "5",
    worstRating: "1",
  };
}
