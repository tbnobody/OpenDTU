import mitt from 'mitt';
import { createApp } from 'vue';
import App from './App.vue';
import { tooltip } from './plugins/bootstrap';
import router from './router';
import { i18n } from './i18n';

import 'bootstrap';
import './scss/styles.scss';

const app = createApp(App);

const emitter = mitt();
app.config.globalProperties.$emitter = emitter;

app.directive('tooltip', tooltip);

app.use(router);
app.use(i18n);

app.mount('#app');
