/// <reference types="vite/client" />

import { Router, Route } from 'vue-router'
declare module 'vue' {
  interface ComponentCustomProperties {
    $router: Router
    $route: Route
  }
}
