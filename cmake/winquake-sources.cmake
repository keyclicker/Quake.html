# ---------------------------------------------------------------------------
# WinQuake engine sources shared by every platform: client, server, QuakeC VM,
# software renderer (C paths only, `id386 == 0`), sound mixer and loopback
# networking. Platform drivers (`sys_*`, `vid_*`, `snd_*`, `in_*`, `cd_*`,
# `net_*` driver tables) live with their platform, e.g. `web/src/`.
# ---------------------------------------------------------------------------

set(WINQUAKE_DIR ${CMAKE_SOURCE_DIR}/WinQuake)

set(WINQUAKE_CORE_SOURCES
  # Client
  cl_demo.c cl_input.c cl_main.c cl_parse.c cl_tent.c chase.c
  console.c keys.c menu.c sbar.c screen.c view.c

  # Shared
  cmd.c common.c crc.c cvar.c host.c host_cmd.c mathlib.c wad.c zone.c

  # Server and QuakeC VM
  sv_main.c sv_move.c sv_phys.c sv_user.c world.c
  pr_cmds.c pr_edict.c pr_exec.c

  # Software renderer
  draw.c model.c nonintel.c
  d_edge.c d_fill.c d_init.c d_modech.c d_part.c d_polyse.c d_scan.c
  d_sky.c d_sprite.c d_surf.c d_vars.c d_zpoint.c
  r_aclip.c r_alias.c r_bsp.c r_draw.c r_edge.c r_efrag.c r_light.c
  r_main.c r_misc.c r_part.c r_sky.c r_sprite.c r_surf.c r_vars.c

  # Sound mixer (platform supplies `SNDDMA_*`)
  snd_dma.c snd_mem.c snd_mix.c

  # Networking core (platform supplies the `net_drivers` table)
  net_loop.c net_main.c net_vcr.c
)

list(TRANSFORM WINQUAKE_CORE_SOURCES PREPEND ${WINQUAKE_DIR}/)
