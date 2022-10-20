import { fileURLToPath, URL } from 'node:url'

import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'

import viteCompression from 'vite-plugin-compression';
import cssInjectedByJsPlugin from 'vite-plugin-css-injected-by-js'

// https://vitejs.dev/config/
export default defineConfig({
  plugins: [
    vue(),
    viteCompression({ deleteOriginFile: true, threshold: 0 }),
    cssInjectedByJsPlugin()],
  resolve: {
    alias: {
      '@': fileURLToPath(new URL('./src', import.meta.url))
    }
  },
  build: {
    // Prevent vendor.css being created
    cssCodeSplit: false,
    outDir: '../webapp_dist',
    emptyOutDir: true,
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
  server: {
    proxy: {
      '^/api': {
        target: 'http://192.168.20.110/'
      },
      '^/livedata': {
        target: 'ws://192.168.20.110/',
        ws: true,
        changeOrigin: true
      }
    }
  }
})
