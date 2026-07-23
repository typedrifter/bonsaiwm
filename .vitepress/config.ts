import { defineConfig } from 'vitepress'

export default defineConfig({
  base: '/bonsaiwm/',
  lang: 'en',
  title: 'BonsaiWM',
  description: 'A compact, opinionated Wayland compositor based on dwl.',
  lastUpdated: true,
  cleanUrls: true,
  ignoreDeadLinks: true,
  themeConfig: {
    socialLinks: [
      { icon: 'github', link: 'https://github.com/typedrifter/bonsaiwm' },
    ],
    search: { provider: 'local' },
    sidebar: [
      {
        text: 'Getting Started',
        items: [
          { text: 'Introduction', link: '/master/' },
          { text: 'Installation', link: '/master/installation' },
          { text: 'Configuration', link: '/master/configuration' },
        ],
      },
      {
        text: 'Project',
        items: [
          { text: 'Roadmap', link: '/master/roadmap' },
          { text: 'Changelog', link: '/master/changelog' },
        ],
      },
    ],
    nav: [
      { text: 'Guide', link: '/master/installation' },
      { text: 'master', link: '/master/' },
    ],
  },
})
