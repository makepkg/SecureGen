import { defineConfig } from 'astro/config';

export default defineConfig({
  site: 'https://makepkg.github.io',
  base: '/SecureGen',
  output: 'static',
  build: {
    assets: 'assets',
  },
});
