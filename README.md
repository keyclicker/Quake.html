# Quake.html

id Software's 1999 GPL Quake source (`readme.txt`), ported to the
browser via Emscripten. Plan and status: `docs/web-port-plan.md`.

## Layout

```
WinQuake/   engine (upstream, patched in place)
QW/ qw-qc/  QuakeWorld (upstream, out of scope for now)
web/        browser platform layer: web/README.md
cmake/      source lists
scripts/    toolchain and data setup
data/       your Quake files (gitignored, never commit)
```

## Build

```sh
scripts/setup-emsdk.sh                      # once, pinned Emscripten
scripts/setup-data.sh /path/to/Quake        # once per checkout
source .emsdk/emsdk_env.sh                  # main checkout root
cmake --preset web && cmake --build --preset web
```
