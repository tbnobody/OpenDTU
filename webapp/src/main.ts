import { createApp } from 'vue'
import App from './App.vue'
import router from './router'
import mitt from 'mitt';

import './scss/styles.scss'
import "bootstrap"

const app = createApp(App)

const emitter = mitt();
app.config.globalProperties.$emitter = emitter;

app.use(router)

app.mount('#app')
