import { defineConfig } from 'vite'
import path from 'path'

// https://vite.dev/config/
export default defineConfig({
  base: '/esp32_adofai',
  //optimizeDeps: {
  //  include: ['mediabunny/dist/module/src'],
  //},
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src'),
      'mediabunny': path.resolve(__dirname, './node_modules/mediabunny'),
      buffer: 'buffer'
      
    }
  },
})
