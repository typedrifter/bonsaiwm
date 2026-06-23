import { existsSync, mkdirSync, readFileSync, writeFileSync } from 'fs';
import { resolve } from 'path';

interface Param { name: string; type: string; desc: string }
interface Overload { signature: string; desc: string }
interface FuncDoc { name: string; desc: string; params: Param[]; overloads: Overload[] }

const FUNC_PATTERN = /^function\s+(bonsaiwm\.\w+)\s*\(.*\)\s*end/;
const PARAM_PATTERN = /^@param\s+(\S+)\s+(\S+)\s*(.*)/;
const OVERLOAD_PATTERN = /^@overload\s+fun\((.*)\)\s*(.*)/;
const ANNOTATION_PATTERN = /^@/;

function parseFunctionDoc(name: string, comments: string[]): FuncDoc {
  const doc: FuncDoc = { name, desc: '', params: [], overloads: [] };
  const descLines: string[] = [];

  for (const c of comments) {
    const param = c.match(PARAM_PATTERN);
    if (param) { doc.params.push({ name: param[1], type: param[2], desc: param[3] }); continue; }

    const overload = c.match(OVERLOAD_PATTERN);
    if (overload) { doc.overloads.push({ signature: `function ${name}(${overload[1]})`, desc: overload[2] }); continue; }

    if (!ANNOTATION_PATTERN.test(c) && c) descLines.push(c);
  }

  doc.desc = descLines.join('\n');
  return doc;
}

function parseLuaDefs(filePath: string): FuncDoc[] {
  const docs: FuncDoc[] = [];
  let comments: string[] = [];

  for (const line of readFileSync(filePath, 'utf-8').split('\n')) {
    const t = line.trim();
    if (t.startsWith('---')) { comments.push(t.slice(3).trim()); continue; }
    const match = t.match(FUNC_PATTERN);
    if (match) { docs.push(parseFunctionDoc(match[1], comments)); }
    comments = [];
  }

  return docs;
}

function generateMarkdown(docs: FuncDoc[]): string {
  const header = [
    '---',
    'title: Lua',
    '',
    '---',
    '# bonsaiwm Lua API Reference',
    '',
    'This document describes the Lua API exposed by bonsaiwm.',
    '',
  ];

  const body = docs.flatMap(doc => [
    `## \`${doc.name}\``,
    '',
    ...(doc.desc ? [doc.desc, ''] : []),
    ...doc.overloads.flatMap(o => ['```lua', o.signature, '```', '', ...(o.desc ? [o.desc, ''] : [])]),
    ...(doc.params.length ? [
      '| Parameter | Type | Description |',
      '|-----------|------|-------------|',
      ...doc.params.map(p => `| \`${p.name}\` | \`${p.type}\` | ${p.desc} |`),
      '',
    ] : []),
    '---',
    '',
  ]);

  return [...header, ...body].join('\n');
}

const root = resolve(import.meta.dirname, '..');
const defsPath = resolve(root, 'bonsaiwm.d.lua');
const outDir = resolve(root, 'docs');
const outPath = resolve(outDir, 'lua.md');

if (!existsSync(defsPath)) { console.error(`Error: ${defsPath} not found`); process.exit(1); }

mkdirSync(outDir, { recursive: true });
writeFileSync(outPath, generateMarkdown(parseLuaDefs(defsPath)));
console.log(`Generated ${outPath}`);
