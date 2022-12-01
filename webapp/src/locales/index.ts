import en from './en.json'
import de from './de.json'

export enum Locales {
    EN = 'en',
    DE = 'de',
}

export const LOCALES = [
    { value: Locales.EN, caption: 'English' },
    { value: Locales.DE, caption: 'Deutsch' }
]

export const messages = {
  [Locales.EN]: en,
  [Locales.DE]: de
};

export const defaultLocale = Locales.EN;