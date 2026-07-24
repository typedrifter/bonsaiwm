---
layout: home
hero:
  name: BonsaiWM
  text: A compact, carefully trimmed and shaped Wayland compositor.
  tagline: Simple, opinionated and extensible with Lua.
  image:
    src: /logo.webp
    alt: BonsaiWM
  actions:
    - theme: brand
      text: Get Started
      link: /latest/
    - theme: alt
      text: View on GitHub
      link: https://github.com/typedrifter/bonsaiwm
---

## What is BonsaiWM?

**BonsaiWM** is a minimalist yet powerful [Wayland] compositor written in C.
It is based on [dwl], with minimal dependencies, and uses [Lua] for configuration and scripting.

## Why another compositor?

After years of using [AwesomeWM] on X, I migrated to [Wayland]. I tried many existing compositors, but I grew frustrated by the trade-off between **minimalism** and **customizability**.
So I started using [dwl], applied numerous [user patches], and heavily customized the code, leading to the birth of BonsaiWM.

### Key Features

- Built on **wlroots**
- **Minimal dependencies** beyond wlroots
- **Lua-configured**, no recompilation needed

### Credits and inspirations

- [dwl] for the awesome suckless base.
- [mangoWM] and [AwesomeWM] for the inspiration.
- Authors of the [dwl] [user patches].

[dwl]: https://codeberg.org/dwl/dwl
[AwesomeWM]: https://awesomewm.org/
[mangoWM]: https://mangowm.github.io/
[user patches]: https://codeberg.org/dwl/dwl-patches
[Lua]: https://www.lua.org/
[Wayland]: https://wayland.freedesktop.org/
