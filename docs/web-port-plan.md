# Web port plan

Goal: WinQuake (NetQuake, software renderer) compiled to wasm with
Emscripten, running in a browser tab on the user's own game data.

## Decisions

| Topic | Choice | Why |
| --- | --- | --- |
| Engine | `WinQuake/`, software renderer | Pure C path (`id386 == 0`), 8-bit framebuffer is trivial to blit. GL and `QW/` later |
| Toolchain | Emscripten `7.0.0`, CMake + Ninja presets | Pinned in `scripts/setup-emsdk.sh` |
| Platform layer | Own drivers in `web/src/` on `emscripten/html5.h`, no SDL | Quake's driver interfaces are tiny; SDL adds a layer we'd fight (audio, main loop) |
| Main loop | `emscripten_set_main_loop` → `Host_Frame` | Browser can't block; `Host_Error` keeps `setjmp`/`longjmp` via `-sSUPPORT_LONGJMP=wasm` |
| Pointer size | wasm32 | Quake assumes 32-bit pointers (`pr_strings` offsets etc.): no 64-bit fixes needed |
| Game data | Never in git or a deploy | Copyrighted. Dev: `data/` via `scripts/setup-data.sh`. Prod: user picks files, cached in OPFS |
| Upstream code | Edit `WinQuake/` in place, minimally | `git diff 0023db3 -- WinQuake` stays a readable patch |

## Our data: 2021 re-release

Our `id1/pak0.pak` (180 MB) is the re-release, not
1996 data. Checked: maps are BSP29 (fine), but `progs.dat` needs:

- builtins `#79 finaleFinished`, `#80 localsound` (vanilla stops at 78)
- 134 `$qc_*` strings, resolved by `localization/loc_english.txt`
  (from `QuakeEX.kpf`, extracted by `setup-data.sh`)

Phase 6 handles both. Until then, use the 1996 shareware `pak0.pak`.

## Phases

- [x] **0. Scaffold**: CMake presets, `winquake_core` compiles to wasm,
  setup scripts, this plan
- [ ] **1. Headless boot**: `web/src/sys_web.c` (main loop, `Sys_*`,
  files, time) + null drivers + `net_none.c`. Done when
  `node build/web/quake.js +map start +quit` loads the map
- [ ] **2. Video**: `vid_web.c`: palette LUT → RGBA → `<canvas>`,
  `image-rendering: pixelated`, resize. Shell in `web/shell/`
- [ ] **3. Input**: `in_web.c`: keys → `K_*`, mouse with pointer lock
- [ ] **4. Sound**: `snd_web.c`: `SNDDMA_*` ring buffer fed to an
  `AudioWorklet`; resume `AudioContext` on first click (autoplay rules)
- [ ] **5. Music**: `cd_web.c`: `CDAudio_Play(n)` plays
  `id1/music/trackNN.ogg` via Web Audio
- [ ] **6. Re-release compat**: builtins 79–80, `$qc_*` localization
- [ ] **7. Data UX**: pick a folder or `pak0.pak` and cache it in OPFS;
  persist saves and `config.cfg` (IDBFS)
- [ ] **8. Deploy**: static GitHub Pages (`.html`, `.js`, `.wasm`,
  no data). GPL: the repo is the source offer

Later, unordered: WebGL renderer, mods (`-game`), QuakeWorld over
WebSocket/WebRTC, touch controls.

## Risks

- **180 MB pak**: slow first load and the whole file sits in memory.
  Cache it in OPFS; maybe stream files from the pak later
- **Hunk size**: `sys_null.c` uses 8 MB; re-release maps want more.
  Start with `-mem 64` and `-sALLOW_MEMORY_GROWTH`
- **Audio timing**: `SNDDMA_GetDMAPos` must track the worklet's real
  position, or the sound will crackle and drift
- **Old C**: compiles with `-std=gnu89` plus relaxed warnings
  (`CMakeLists.txt`). Some warnings are real bugs, e.g. `abs()` on
  floats in `sv_move.c:321`. Fix them only when they bite
