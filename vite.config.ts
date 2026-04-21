import { defineConfig } from 'vite'
import path from 'path'

// https://vite.dev/config/
export default defineConfig({
  base: '/esp32_adofai',
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src')
    }
  }
})
