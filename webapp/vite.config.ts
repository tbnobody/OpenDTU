import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

import viteCompression from 'vite-plugin-compression';
import cssInjectedByJsPlugin from 'vite-plugin-css-injected-by-js'
import VueI18nPlugin from '@intlify/unplugin-vue-i18n/vite'

const path = require('path')

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    viteCompression({ deleteOriginFile: true, threshold: 0 }),
    cssInjectedByJsPlugin(),
    VueI18nPlugin({
        /* options */
        include: path.resolve(path.dirname(fileURLToPath(import.meta.url)), './src/locales/**.json'),
        fullInstall: false,
        forceStringify: true,
        strictMessage: false,
    }),
  ],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url)),
      '~bootstrap': path.resolve(__dirname, 'node_modules/bootstrap'),
    }
  },
  build: {
    // Prevent vendor.css being created
    cssCodeSplit: false,
    outDir: '../webapp_dist',
    emptyOutDir: true,
    minify: 'terser',
    rollupOptions: {
      output: {
        // Only create one js file
        inlineDynamicImports: true,
        // Get rid of hash on js file
        entryFileNames: 'js/app.js',
        // Get rid of hash on css file
        assetFileNames: "assets/[name].[ext]",
      },
    },
  },
  esbuild: {
    drop: ['console', 'debugger'],
  },
  server: {
    proxy: {
      '^/api': {
        target: 'http://192.168.2.93/'
      },
      '^/livedata': {
        target: 'ws://192.168.2.93/',
        ws: true,
        changeOrigin: true
      },
      '^/console': {
        target: 'ws://192.168.2.93/',
        ws: true,
        changeOrigin: true
      }
    }
  }
})
