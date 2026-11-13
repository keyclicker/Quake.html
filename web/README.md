# `web/`: browser platform

Everything the browser port adds lives here; the engine stays in
`WinQuake/`. Files appear phase by phase (`docs/web-port-plan.md`).

The build output is one self-contained `quake.html`: every file below
gets inlined, nothing is fetched at runtime except the user's data.

```
web/
  src/          C drivers for the engine, on emscripten/html5.h
    sys_web.c     main loop, Sys_*, files, time        (phase 1)
    vid_web.c     framebuffer + palette -> <canvas>    (phase 2)
    in_web.c      keyboard, pointer-locked mouse       (phase 3)
    snd_web.c     SNDDMA_* -> AudioWorklet             (phase 4)
    cd_web.c      CD tracks -> id1/music/*.ogg         (phase 5)
  js/           Emscripten JS libraries (--js-library), worklet source
  shell/
    quake.html    --shell-file: page, inline CSS, data loader UI
```
