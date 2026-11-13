#!/usr/bin/env bash
# Install the pinned Emscripten SDK into the main checkout's `.emsdk/`,
# shared by every worktree. Override the location with `EMSDK_DIR`.
set -euo pipefail

EMSDK_VERSION=7.0.0

main_root=$(cd "$(git rev-parse --git-common-dir)/.." && pwd)
emsdk_dir=${EMSDK_DIR:-$main_root/.emsdk}

if [[ ! -d $emsdk_dir ]]; then
  git clone --depth 1 https://github.com/emscripten-core/emsdk.git "$emsdk_dir"
fi

"$emsdk_dir/emsdk" install "$EMSDK_VERSION"
"$emsdk_dir/emsdk" activate "$EMSDK_VERSION"

echo
echo "Done. In each shell: source \"$emsdk_dir/emsdk_env.sh\""
