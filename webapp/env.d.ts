/// <reference types="vite/client" />

import { Router, Route } from 'vue-router'
declare module '@vue/runtime-core' {
  interface ComponentCustomProperties {
    $router: Router
    $route: Route
  }
}
