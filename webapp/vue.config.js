const { defineConfig } = require('@vue/cli-service')
module.exports = defineConfig({
    transpileDependencies: true,
    productionSourceMap: false,
    outputDir: '../data',
    filenameHashing: false,
    css: {
        extract: false,
    },
    configureWebpack: {
        optimization: {
            splitChunks: false
        }
    },
    pluginOptions: {
        compression: {
            gzip: {
                filename: '[file].gz[query]',
                algorithm: 'gzip',
                include: /\.(js|css|html|svg|json)(\?.*)?$/i,
                deleteOriginalAssets: true,
                minRatio: 0.8,
            }
        }
    },
    devServer: {
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