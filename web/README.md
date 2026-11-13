# `web/`: browser platform

Everything the browser port adds lives here; the engine stays in
`WinQuake/`. Files appear phase by phase (`docs/web-port-plan.md`).

The build output is one self-contained `quake.html`: every file below
gets inlined, nothing is fetched at runtime except the user's data.

```
web/
  src/          C drivers for the engine; JS glue as EM_JS beside it
    sys_web.c     main loop, Sys_*, files, time
    vid_web.c     framebuffer + palette -> <canvas>
    in_web.c      keyboard, pointer-locked mouse
    snd_web.c     SNDDMA_* ring -> AudioWorklet
    cd_web.c      CD tracks -> id1/music/trackNN.ogg
  fs/           embedded at /quake/ (our id1/default.cfg)
  shell/
    quake.html    --shell-file: page, inline CSS, data loader
```

## Running it

`quake.html` loads `id1/pak0.pak` (plus optional `pak1.pak`,
`localization/loc_english.txt`, `music/trackNN.ogg`) from next to
itself. Without them it offers a file picker.

- `?data=<url>`: folder that holds `id1/`
- `?args=<cmd>`: extra command line, e.g. `+map e1m1 -width 800`
