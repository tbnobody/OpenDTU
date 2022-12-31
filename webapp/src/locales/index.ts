import en from './en.json'
import de from './de.json'
import fr from './fr.json'

export enum Locales {
    EN = 'en',
    DE = 'de',
    FR = 'fr',
}

export const LOCALES = [
    { value: Locales.EN, caption: 'English' },
    { value: Locales.DE, caption: 'Deutsch' },
    { value: Locales.FR, caption: 'Français' },
]

export const messages = {
    [Locales.EN]: en,
    [Locales.DE]: de,
    [Locales.FR]: fr,
};

export const defaultLocale = Locales.EN;