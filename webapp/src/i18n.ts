import { createI18n } from 'vue-i18n';
import messages from '@intlify/unplugin-vue-i18n/messages';
import type { I18nOptions, IntlDateTimeFormat, IntlNumberFormat } from 'vue-i18n';

export const allLocales = [
    { code: 'en', name: 'English' },
    { code: 'de', name: 'Deutsch' },
    { code: 'fr', name: 'Français' },
];

const dateTimeFormatsTemplate: IntlDateTimeFormat = {
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

const numberFormatTemplate: IntlNumberFormat = {
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

export const dateTimeFormats: I18nOptions['datetimeFormats'] = {};
export const numberFormats: I18nOptions['numberFormats'] = {};

allLocales.forEach((locale) => {
    dateTimeFormats[locale.code] = dateTimeFormatsTemplate;
    numberFormats[locale.code] = numberFormatTemplate;
});

export const i18n = createI18n({
    legacy: false,
    globalInjection: true,
    locale: navigator.language.split('-')[0],
    fallbackLocale: allLocales[0].code,
    messages,
    datetimeFormats: dateTimeFormats,
    numberFormats: numberFormats,
});

const dynamicLocales = await loadAvailLocales();
allLocales.push(...dynamicLocales);

if (localStorage.getItem('locale')) {
    setLocale(localStorage.getItem('locale') || 'en');
} else {
    localStorage.setItem('locale', i18n.global.locale.value);
}

// Set new locale.
export async function setLocale(locale: string) {
    // Load locale if not available yet.
    if (!i18n.global.availableLocales.includes(locale)) {
        const messages = await loadLocale(locale);

        // fetch() error occurred.
        if (messages === undefined) {
            i18n.global.locale.value = allLocales[0].code;
            return;
        }

        // Add locale.
        i18n.global.setLocaleMessage(locale, messages.webapp);
        i18n.global.setNumberFormat(locale, numberFormatTemplate);
        i18n.global.setDateTimeFormat(locale, dateTimeFormatsTemplate);
    }

    // Set locale.
    i18n.global.locale.value = locale;
    localStorage.setItem('locale', i18n.global.locale.value);
}

// Fetch locale.
async function loadLocale(locale: string) {
    return fetch(`/api/i18n/language?code=${locale}`)
        .then((response) => {
            if (response.ok) {
                return response.json();
            }
            throw new Error('Something went wrong!');
        })
        .catch((error) => {
            console.error(error);
        });
}

// Fetch available locales
async function loadAvailLocales() {
    return fetch('/api/i18n/languages')
        .then((response) => {
            if (response.ok) {
                return response.json();
            }
            throw new Error('Something went wrong!');
        })
        .catch((error) => {
            console.error(error);
        });
}
