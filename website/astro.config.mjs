import { defineConfig } from 'astro/config';

export default defineConfig({
  site: 'https://RafaelReyesCarmona.github.io',
  base: '/SecureGen-es_ES',
  output: 'static',
  build: {
    assets: 'assets',
  },
});
