// SPDX-FileCopyrightText: 2025-2026 Kaito Udagawa <umireon@kaito.tokyo>
//
// SPDX-License-Identifier: Apache-2.0

import i18nDic from "./i18n-dic.json";

export const globalLang = "en";

type I18nDic = typeof i18nDic;
export type Lang = keyof I18nDic;
type Keys = keyof I18nDic[typeof globalLang];

export const i18nLangs = Object.keys(i18nDic) as Lang[];

export const i18nLangsWithoutGlobal = i18nLangs.filter((lang) => lang !== globalLang);

export function isValidLang(lang: string): lang is Lang {
  return lang in i18nDic;
}

export function useTranslations(lang: string) {
  const validLang =
    lang != null && lang in i18nDic ? (lang as Lang) : globalLang;

  return (key: Keys, vars?: Record<string, string | number>) => {
    let str = i18nDic[validLang][key] || i18nDic[globalLang][key] || "";
    if (vars) {
      for (const [k, v] of Object.entries(vars)) {
        str = str.replaceAll(`{${k}}`, String(v));
      }
    }
    return str;
  };
}
