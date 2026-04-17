// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

export const defaultContentSecurityPolicy = [
  "default-src 'self';",

  "connect-src 'self' https://cloudflareinsights.com;",
  "img-src 'self' data: https://github.com/kaito-tokyo/live-backgroundremoval-lite/actions/workflows/daily-website-security-check.yml/badge.svg;",
  "script-src 'self' https://static.cloudflareinsights.com;",
  "style-src 'self';",

  "require-trusted-types-for 'script';",
  "trusted-types 'none';",

  "base-uri 'none';",
  "font-src 'self';",
  "form-action 'self';",
  "frame-src 'none';",
  "manifest-src 'self';",
  "object-src 'none';",
  "upgrade-insecure-requests;",
  "worker-src 'self';",
  "block-all-mixed-content;"
];
