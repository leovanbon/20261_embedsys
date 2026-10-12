#!/usr/bin/env bash
set -euo pipefail
lab_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)"
export WINEPREFIX="$lab_root/.wine-hercules"
export WINEDEBUG=-all
export XCURSOR_THEME=Adwaita
export XCURSOR_SIZE=24
export WINEDLLOVERRIDES=winemenubuilder.exe=d
exec wine explorer /desktop=Hercules,1024x768 "$lab_root/Lab 1/hercules_3-2-8.exe"
