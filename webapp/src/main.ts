import { createApp } from 'vue'
import { createI18n } from 'vue-i18n'
import { messages, defaultLocale } from './locales'
import App from './App.vue'
import router from './router'
import mitt from 'mitt';
import { tooltip } from './plugins/bootstrap'

import './scss/styles.scss'
import "bootstrap"

const app = createApp(App)

const emitter = mitt();
app.config.globalProperties.$emitter = emitter;

app.directive('tooltip', tooltip)

const i18n = createI18n({
    legacy: false,
    globalInjection: true,
    locale: navigator.language.split('-')[0],
    fallbackLocale: defaultLocale,
    messages
})

app.use(router)
app.use(i18n)

app.mount('#app')
