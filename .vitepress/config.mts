import { defineConfig } from 'vitepress'
import { versionedConfig } from './versions'

// https://vitepress.dev/reference/site-config
export default defineConfig({
  srcDir: 'docs',
  title: 'BonsaiWM',
  description: 'BonsaiWM is a compact, opinionated Wayland compositor based on dwl.',
  base: "/bonsaiwm",
  additionalConfig: versionedConfig,
  themeConfig: {
    socialLinks: [
      { icon: 'github', link: 'https://github.com/typedrifter/bonsaiwm' },
    ],
  },
})
