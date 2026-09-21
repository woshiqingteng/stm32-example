#!/usr/bin/env bash
# Configure/build/flash one firmware with a layered build tree:
#   build/<BUILD_TYPE>/<BOARD>/<OS>/<APP_NAME>/
# Intermediates and artifacts share that directory.
set -euo pipefail

usage() {
    echo "usage: $0 <debug|release> <app_name> [build|flash]" >&2
    exit 1
}

[ "$#" -ge 2 ] || usage
config="$1"
app="$2"
action="${3:-build}"

case "$config" in debug|release) ;; *) usage ;; esac
case "$action" in build|flash) ;; *) usage ;; esac

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
board="openedv_stm32f4"
os="baremetal"
build_dir="$root/build/$config/$board/$os/$app"
target="app_${os}_${app}"

cd "$root"
cmake --preset "$config" -B "$build_dir" -DAPP_TARGET="$target"
cmake --build "$build_dir"

if [ "$action" = "flash" ]; then
    cmake --build "$build_dir" --target flash
fi
