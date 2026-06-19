// @ts-check
import { defineConfig } from "astro/config";
import starlight from "@astrojs/starlight";
import { unified } from "@astrojs/markdown-remark";
import remarkRelativeLinks from "./src/plugins/remark-relative-links.mjs";

// https://astro.build/config
export default defineConfig({
  integrations: [
    starlight({
      customCss: [
        // Relative path to your custom CSS file
        "./src/styles/custom.css",
      ],

      title: "My Docs",
      social: [
        {
          icon: "github",
          label: "GitHub",
          href: "https://github.com/withastro/starlight",
        },
      ],
      sidebar: [
        {
          label: "BonsaiWM",
          items: [
            { label: "About", slug: "about" },
            { label: "Roadmap", slug: "roadmap" },
          ],
        },
        {
          label: "Guides",
          items: [{ label: "Example Guide", slug: "guides/example" }],
        },
        {
          label: "Reference",
          items: [{ autogenerate: { directory: "references" } }],
        },
      ],
    }),
  ],
  markdown: {
    processor: unified({
      remarkPlugins: [remarkRelativeLinks],
    }),
  },
});
