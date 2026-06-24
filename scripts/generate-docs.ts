import { existsSync, mkdirSync, readFileSync, writeFileSync } from 'fs';
import { resolve } from 'path';

interface Param { name: string; type: string; desc: string }
interface Overload { signature: string; desc: string }
interface FuncDoc { name: string; desc: string; params: Param[]; overloads: Overload[] }
interface FieldDoc { parent: string; name: string; desc: string; type: string }

const FUNC_PATTERN = /^function\s+(bonsaiwm\.\w+)\s*\(.*\)\s*end/;
const FIELD_PATTERN = /^(bonsaiwm\.\w+\.\w+)\s*=\s*\S+/;
const PARAM_PATTERN = /^@param\s+(\S+)\s+(\S+)\s*(.*)/;
const TYPE_PATTERN = /^@type\s+(\S+)/;
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

function parseFieldDoc(fullName: string, comments: string[]): FieldDoc {
  const parts = fullName.split('.');
  const parent = parts.slice(0, -1).join('.');
  const name = parts[parts.length - 1];
  let type = 'any';
  const descLines: string[] = [];

  for (const c of comments) {
    const typeMatch = c.match(TYPE_PATTERN);
    if (typeMatch) { type = typeMatch[1]; continue; }
    if (!ANNOTATION_PATTERN.test(c) && c) descLines.push(c);
  }

  return { parent, name, desc: descLines.join('\n'), type };
}

interface ParseResult { funcs: FuncDoc[]; fields: FieldDoc[] }

function parseLuaDefs(filePath: string): ParseResult {
  const funcs: FuncDoc[] = [];
  const fields: FieldDoc[] = [];
  let comments: string[] = [];

  for (const line of readFileSync(filePath, 'utf-8').split('\n')) {
    const t = line.trim();
    if (t.startsWith('---')) { comments.push(t.slice(3).trim()); continue; }
    const funcMatch = t.match(FUNC_PATTERN);
    if (funcMatch) { funcs.push(parseFunctionDoc(funcMatch[1], comments)); }
    const fieldMatch = t.match(FIELD_PATTERN);
    if (fieldMatch) { fields.push(parseFieldDoc(fieldMatch[1], comments)); }
    comments = [];
  }

  return { funcs, fields };
}

function generateMarkdown(result: ParseResult): string {
  const { funcs: docs, fields } = result;

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

  const fieldsByParent = new Map<string, FieldDoc[]>();
  for (const f of fields) {
    if (!fieldsByParent.has(f.parent)) fieldsByParent.set(f.parent, []);
    fieldsByParent.get(f.parent)!.push(f);
  }

  const constantsSections: string[] = [];
  for (const [parent, fs] of fieldsByParent) {
    constantsSections.push(
      `## Constants: \`${parent}\``, '',
      '| Name | Type | Description |',
      '|------|------|-------------|',
      ...fs.map(f => `| \`${f.name}\` | \`${f.type}\` | ${f.desc} |`),
      '', '---', '',
    );
  }

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

  return [...header, ...body, ...constantsSections].join('\n');
}

const root = resolve(import.meta.dirname, '..');
const defsPath = resolve(root, 'bonsaiwm.d.lua');
const outDir = resolve(root, 'docs');
const outPath = resolve(outDir, 'lua.md');

if (!existsSync(defsPath)) { console.error(`Error: ${defsPath} not found`); process.exit(1); }

mkdirSync(outDir, { recursive: true });
writeFileSync(outPath, generateMarkdown(parseLuaDefs(defsPath)));
console.log(`Generated ${outPath}`);
