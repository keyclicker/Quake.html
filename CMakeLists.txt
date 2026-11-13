cmake_minimum_required(VERSION 3.20)
project(quake_html C)

include(cmake/winquake-sources.cmake)

# ---------------------------------------------------------------------------
# Engine core
# ---------------------------------------------------------------------------

add_library(winquake_core STATIC ${WINQUAKE_CORE_SOURCES})
target_include_directories(winquake_core PUBLIC ${WINQUAKE_DIR})

# 1996 C: K&R-isms, implicit declarations and int/pointer punning.
target_compile_options(winquake_core PUBLIC
  -std=gnu89
  -fno-strict-aliasing
  -Wno-implicit-function-declaration
  -Wno-implicit-int
  -Wno-int-conversion
  -Wno-incompatible-pointer-types
)

# ---------------------------------------------------------------------------
# Web platform: `quake` executable from `web/src/` lands here (phase 1).
# ---------------------------------------------------------------------------
