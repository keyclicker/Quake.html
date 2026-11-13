#!/usr/bin/env bash
# Link a Quake install into `data/` for local runs. Never commit or deploy it.
#
# Usage: scripts/setup-data.sh <quake-dir>
#   <quake-dir> holds `id1/pak0.pak`; the 2021 re-release also has
#   `QuakeEX.kpf`, from which the English localization is extracted.
set -euo pipefail

quake_dir=$(realpath "${1:?usage: $0 <quake-dir>}")
data_dir=$(git rev-parse --show-toplevel)/data

[[ -f $quake_dir/id1/pak0.pak ]] || {
  echo "no id1/pak0.pak in $quake_dir" >&2
  exit 1
}

mkdir -p "$data_dir/id1"

# Paks and music: symlinks, read-only for us
for item in "$quake_dir"/id1/pak*.pak "$quake_dir"/id1/music; do
  [[ -e $item ]] && ln -sfn "$item" "$data_dir/id1/$(basename "$item")"
done

# Re-release: QuakeC strings like `$qc_got_shells` resolve via this file
if [[ -f $quake_dir/QuakeEX.kpf ]]; then
  unzip -o -q "$quake_dir/QuakeEX.kpf" localization/loc_english.txt \
    -d "$data_dir/id1"
fi

ls -l "$data_dir/id1"
