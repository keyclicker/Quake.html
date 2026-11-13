cmake_minimum_required(VERSION 3.20)
project(quake_html C)

include(cmake/winquake-sources.cmake)

# ---------------------------------------------------------------------------
# Engine core
# ---------------------------------------------------------------------------

add_library(winquake_core STATIC ${WINQUAKE_CORE_SOURCES})
target_include_directories(winquake_core PUBLIC ${WINQUAKE_DIR})

# 1996 C: K&R-isms, implicit declarations, int/pointer punning, and
# globals defined in headers (`-fcommon` merges them, as in 1996).
target_compile_options(winquake_core PUBLIC
  -std=gnu89
  -fcommon
  -fno-strict-aliasing
  -Wno-implicit-function-declaration
  -Wno-implicit-int
  -Wno-int-conversion
  -Wno-incompatible-pointer-types
)

# ---------------------------------------------------------------------------
# Web platform: one self-contained `quake.html`
# ---------------------------------------------------------------------------

set(WEB_DIR ${CMAKE_SOURCE_DIR}/web)
set(WEB_SHELL ${WEB_DIR}/shell/quake.html)
file(GLOB_RECURSE WEB_FS_FILES CONFIGURE_DEPENDS ${WEB_DIR}/fs/*)

add_executable(quake
  ${WEB_DIR}/src/cd_web.c
  ${WEB_DIR}/src/in_web.c
  ${WEB_DIR}/src/snd_web.c
  ${WEB_DIR}/src/sys_web.c
  ${WEB_DIR}/src/vid_web.c
  ${WINQUAKE_DIR}/net_none.c
)
target_link_libraries(quake PRIVATE winquake_core)
set_target_properties(quake PROPERTIES
  SUFFIX ".html"
  LINK_DEPENDS "${WEB_SHELL};${WEB_FS_FILES}"
)

target_link_options(quake PRIVATE
  # Wasm, JS and the shell's CSS/JS in one HTML file
  -sSINGLE_FILE
  --shell-file ${WEB_SHELL}
  -sENVIRONMENT=web

  # `web/fs/` -> `/quake/`, e.g. our `id1/default.cfg`
  --embed-file ${WEB_DIR}/fs@/quake

  # 64 MB hunk (`sys_web.c`) + video buffers; the renderer's edge and
  # surface arrays live on the C stack
  -sINITIAL_MEMORY=128MB
  -sALLOW_MEMORY_GROWTH
  -sSTACK_SIZE=4MB

  # The shell writes game data into the FS, then calls `main`
  -sINVOKE_RUN=0
  -sEXPORTED_RUNTIME_METHODS=callMain,FS
)
