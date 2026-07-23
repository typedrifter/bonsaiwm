import fs from 'node:fs'
import path from 'node:path'
import { fileURLToPath } from 'node:url'

const docsRoot = fileURLToPath(new URL('../docs', import.meta.url))

/** Version folder names under docs/, e.g. ["dev", "master"] */
const versions = fs
  .readdirSync(docsRoot, { withFileTypes: true })
  .filter(entry => entry.isDirectory() && !entry.name.startsWith('.') && entry.name !== 'public')
  .map(entry => entry.name)

/** Sidebar items declared in docs/<version>/docs.json */
function readSidebarItems(version: string) {
  const file = path.join(docsRoot, version, 'docs.json')
  return JSON.parse(fs.readFileSync(file, 'utf8')).items
}

/** Entries of the version picker in the nav bar */
const versionPickerItems = versions.map(name => ({
  text: name,
  link: `/${name}/`,
  activeMatch: `/${name}/`,
}))

/** Per-version theme config, keyed by route prefix (e.g. "/dev/") */
export const versionedConfig = Object.fromEntries(
  versions.map(name => [
    `/${name}/`,
    {
      themeConfig: {
        nav: [
          { text: 'Home', link: '/' },
          { text: name, items: versionPickerItems },
        ],
        sidebar: [{ base: `/${name}/`, items: readSidebarItems(name) }],
      },
    },
  ])
)
