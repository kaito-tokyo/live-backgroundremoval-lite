// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

/// <reference types="astro/client" />

// Generating a redirect stub since we use GitHub Pages which doesn't support redirecting responses.

import { withTrailingSlash } from "ufo";

import { PRODUCTION_BASE_URL } from "../../lib/info.mjs";

const { BASE_URL } = import.meta.env;

const target = withTrailingSlash(BASE_URL);
const canonicalTarget = withTrailingSlash(PRODUCTION_BASE_URL);

const html = `
<!DOCTYPE html>
<title>Redirecting...</title>
<link rel="canonical" href="${canonicalTarget}">
<meta name="robots" content="noindex, nofollow">
<meta http-equiv="refresh" content="0; url=${target}">
<script>window.location.replace("${target}")</script>
`;

export async function GET() {
  return new Response(html, {
    status: 200,
    headers: {
      "Content-Type": "text/html",
    },
  });
}
