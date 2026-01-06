import i18nDic from './i18n-dic.json';

export const defaultLang = 'en';

type I18nDic = typeof i18nDic;
type Lang = keyof I18nDic;
type Keys = keyof I18nDic[typeof defaultLang];

export const i18nLangs = Object.keys(i18nDic) as Lang[];

export function useTranslations(lang: string) {
  const validLang = (lang && lang in i18nDic) ? (lang as Lang) : defaultLang;

  return (key: Keys) => {
    return i18nDic[validLang][key] || i18nDic[defaultLang][key];
  };
}
