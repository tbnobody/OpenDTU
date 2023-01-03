import type { I18nOptions } from "vue-i18n";
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

export const messages: I18nOptions["messages"] = {
    [Locales.EN]: en,
    [Locales.DE]: de,
    [Locales.FR]: fr,
};

export const dateTimeFormats: I18nOptions["datetimeFormats"] = {
    [Locales.EN]: {
        'datetime': {
            hour: 'numeric',
            minute: 'numeric',
            second: 'numeric',
            year: 'numeric',
            month: 'numeric',
            day: 'numeric',
            hour12: false
        }
    },
    [Locales.DE]: {
        'datetime': {
            hour: 'numeric',
            minute: 'numeric',
            second: 'numeric',
            year: 'numeric',
            month: 'numeric',
            day: 'numeric',
            hour12: false
        }
    },
    [Locales.FR]: {
        'datetime': {
            hour: 'numeric',
            minute: 'numeric',
            second: 'numeric',
            year: 'numeric',
            month: 'numeric',
            day: 'numeric',
            hour12: false
        }
    }
};

export const defaultLocale = Locales.EN;