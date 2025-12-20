export const GITHUB_OWNER = "kaito-tokyo";
export const GITHUB_REPO = "live-backgroundremoval-lite";
export const GITHUB_URL = `https://github.com/${GITHUB_OWNER}/${GITHUB_REPO}`;
export const PRODUCTION_BASE_URL =
  "https://kaito-tokyo.github.io/live-backgroundremoval-lite";

export async function getAggregateRating() {
  return {
    "@type": "AggregateRating",
    ratingValue: "5.0",
    reviewCount: "1",
    bestRating: "5",
    worstRating: "5",
  };
}
