// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

/**
 * @template T
 * @param {T} value
 * @returns {T}
 */
export function deepSort(value) {
  if (value == null || typeof value !== "object") {
    return value;
  } else if (Array.isArray(value)) {
    return value.map(deepSort);
  } else {
    return Object.fromEntries(
      Object.keys(value)
        .sort()
        .map((key) => [key, deepSort(value[key])]),
    );
  }
}

/**
 * @param {Parameters<typeof JSON.stringify>[0]} value
 * @param {Parameters<typeof JSON.stringify>[1]} replacer
 * @param {Parameters<typeof JSON.stringify>[2]} space
 * @returns {string}
 */
export function sortStringify(value, replacer = undefined, space = undefined) {
  return JSON.stringify(deepSort(value), replacer, space);
}
