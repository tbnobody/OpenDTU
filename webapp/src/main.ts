import mitt from 'mitt'
import { createApp } from 'vue'
import { createI18n } from 'vue-i18n'
import App from './App.vue'
import { defaultLocale, messages, dateTimeFormats, numberFormats } from './locales'
import { tooltip } from './plugins/bootstrap'
import router from './router'

import "bootstrap"
import './scss/styles.scss'

const app = createApp(App)

const emitter = mitt();
app.config.globalProperties.$emitter = emitter;

app.directive('tooltip', tooltip)

const i18n = createI18n({
    legacy: false,
    globalInjection: true,
    locale: navigator.language.split('-')[0],
    fallbackLocale: defaultLocale,
    messages,
    datetimeFormats: dateTimeFormats,
    numberFormats: numberFormats
})

app.use(router)
app.use(i18n)

app.mount('#app')
