import { defineCollection } from 'astro:content';
import { docsSchema } from '@astrojs/starlight/schema';
import { docsLoaderWithDefaultTitle } from './loaders/docs-with-default-title.ts';

export const collections = {
	docs: defineCollection({ loader: docsLoaderWithDefaultTitle(), schema: docsSchema() }),
};
