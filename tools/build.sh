#!/usr/bin/env bash
# Configure/build (and optionally flash) one or more firmwares, each with its
# own layered build tree: build/<BUILD_TYPE>/<BOARD>/<OS>/<APP_NAME>/
#
# usage:
#   tools/build.sh <debug|release> <app|all> [<app> ...] [--flash]
#
# app names are the directory names under app/baremetal (e.g. 01_led).
set -uo pipefail

usage() {
    echo "usage: $0 <debug|release> <app|all> [<app> ...] [--flash]" >&2
    exit 1
}

[ "$#" -ge 2 ] || usage

config="$1"
shift

flash=0
apps=()
for arg in "$@"; do
    case "$arg" in
        --flash) flash=1 ;;
        *) apps+=("$arg") ;;
    esac
done
[ "${#apps[@]}" -ge 1 ] || usage
case "$config" in debug|release) ;; *) usage ;; esac

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
board="openedv_stm32f4"
os="baremetal"

# Expand "all" to every app directory (names starting with a digit).
expanded=()
for app in "${apps[@]}"; do
    if [ "$app" = "all" ]; then
        for dir in "$root"/app/baremetal/[0-9]*; do
            expanded+=("$(basename "$dir")")
        done
    else
        expanded+=("$app")
    fi
done

cd "$root"
failed=0
for app in "${expanded[@]}"; do
    build_dir="$root/build/$config/$board/$os/$app"
    target="app_${os}_${app}"
    echo "=== $config / $app -> $build_dir"
    if ! cmake --preset "$config" -B "$build_dir" -DAPP_TARGET="$target"; then
        echo "!!! configure failed: $app"
        failed=1
        continue
    fi
    if ! cmake --build "$build_dir"; then
        echo "!!! build failed: $app"
        failed=1
        continue
    fi
    if [ "$flash" -eq 1 ]; then
        if ! cmake --build "$build_dir" --target flash; then
            echo "!!! flash failed: $app"
            failed=1
        fi
    fi
done
exit "$failed"
