import path from "node:path";

/**
 * @returns {import('remark').Plugin}
 */
export default function remarkRelativeLinks() {
  /**
   * @param {import('mdast').Root} tree
   * @param {import('vfile').VFile} file
   */
  return (tree, file) => {
    const filePath = file.history[0] ?? file.path;
    if (!filePath) return;

    const marker = "src/content/docs";
    const idx = filePath.indexOf(marker);

    let rel, sourceDir;
    if (idx !== -1) {
      rel = filePath.slice(idx + marker.length + 1);
      sourceDir = path.dirname(rel);
    } else {
      rel = path.basename(filePath);
      sourceDir = ".";
    }

    const toUrlPath = (/** @type {string} */ p) =>
      p
        .replace(/\.mdx?$/, "")
        .toLowerCase()
        .replace(/\/?index$/, "");

    const current = "/" + toUrlPath(rel);

    /** @param {import('mdast').Link} node */
    const fix = (node) => {
      const raw = node.url;
      if (!raw) return;
      if (
        raw.startsWith("http") ||
        raw.startsWith("/") ||
        raw.startsWith("#") ||
        raw.startsWith("mailto:")
      )
        return;
      const m = raw.match(/^([^#]+)(#.*)?$/);
      if (!m) return;
      const [_, href, hash = ""] = m;
      if (!href.endsWith(".md") && !href.endsWith(".mdx")) return;

      const resolved = path.resolve("/" + sourceDir, href).slice(1);
      const remapped = resolved.replace(/^docs\//, "references/");
      const target = "/" + toUrlPath(remapped);

      let out = path.relative(current, target) || ".";
      if (!out.startsWith(".")) out = "./" + out;
      if (!out.endsWith("/")) out += "/";

      node.url = out + hash;
    };

    /** @param {import('mdast').Node} node */
    const walk = (node) => {
      if (node.type === "link") fix(/** @type {import('mdast').Link} */ (node));
      if ("children" in node && Array.isArray(node.children)) {
        for (const child of node.children) walk(child);
      }
    };

    walk(tree);
  };
}
