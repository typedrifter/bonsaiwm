import { docsLoader } from '@astrojs/starlight/loaders';
import type { Loader } from 'astro/loaders';

const deriveTitleFromId = (id: string): string => {
  // id is the entry's slug, e.g. 'references/build', 'lua', 'guides/example'
  const basename = id.split('/').pop() ?? id;
  if (!basename) return 'Untitled';
  return basename.charAt(0).toUpperCase() + basename.slice(1);
};

export function docsLoaderWithDefaultTitle(): Loader {
  const base = docsLoader();
  return {
    name: 'starlight-docs-with-default-title',
    load: async (ctx) => {
      const originalParseData = ctx.parseData;
      const wrappedCtx = {
        ...ctx,
        parseData: (args: { id: string; data: Record<string, unknown>; filePath?: string }) => {
          if (!args.data?.title) {
            args.data = { ...args.data, title: deriveTitleFromId(args.id) };
          }
          return originalParseData(args);
        },
      };
      await base.load(wrappedCtx);
    },
  };
}
