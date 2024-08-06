# OpenDTU web frontend

You can run the webapp locally with `yarn dev`. If you enter the IP of your ESP in the `vite.user.ts` beforehand (template can be found in `vite.config.ts`), all api requests will even be proxied to the real ESP. Then you can develop the webapp as if it were running directly on the ESP. The `yarn dev` also supports hot reload, i.e. as soon as you save a vue file, it is automatically reloaded in the browser.

## Project Setup

```sh
yarn install
```

### Compile and Hot-Reload for Development

```sh
yarn dev
```

### Type-Check, Compile and Minify for Production

```sh
yarn build
```

### Lint with [ESLint](https://eslint.org/)

```sh
yarn lint
```
