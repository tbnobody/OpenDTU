import type { I18nOptions } from 'vue-i18n';

export enum Locales {
    EN = 'en',
    DE = 'de',
    FR = 'fr',
}

export const LOCALES = [
    { value: Locales.EN, caption: 'English' },
    { value: Locales.DE, caption: 'Deutsch' },
    { value: Locales.FR, caption: 'Français' },
];

export const dateTimeFormats: I18nOptions['datetimeFormats'] = {};
export const numberFormats: I18nOptions['numberFormats'] = {};

LOCALES.forEach((locale) => {
    dateTimeFormats[locale.value] = {
        datetime: {
            hour: 'numeric',
            minute: 'numeric',
            second: 'numeric',
            year: 'numeric',
            month: 'numeric',
            day: 'numeric',
            hour12: false,
        },
    };

    numberFormats[locale.value] = {
        decimal: {
            style: 'decimal',
        },
        decimalNoDigits: {
            style: 'decimal',
            minimumFractionDigits: 0,
            maximumFractionDigits: 0,
        },
        decimalOneDigit: {
            style: 'decimal',
            minimumFractionDigits: 1,
            maximumFractionDigits: 1,
        },
        decimalTwoDigits: {
            style: 'decimal',
            minimumFractionDigits: 2,
            maximumFractionDigits: 2,
        },
        percent: {
            style: 'percent',
        },
        percentOneDigit: {
            style: 'percent',
            minimumFractionDigits: 1,
            maximumFractionDigits: 1,
        },
        byte: {
            style: 'unit',
            unit: 'byte',
        },
        kilobyte: {
            style: 'unit',
            unit: 'kilobyte',
        },
        megabyte: {
            style: 'unit',
            unit: 'megabyte',
        },
        celsius: {
            style: 'unit',
            unit: 'celsius',
            maximumFractionDigits: 1,
        },
    };
});

export const defaultLocale = Locales.EN;
