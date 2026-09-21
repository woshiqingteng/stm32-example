#!/usr/bin/env bash
# Configure/build (and optionally flash) one or more firmwares.
#
# The OS (baremetal / freertos) is detected from the app directory:
#   app/baremetal/<name> -> APP_TARGET=app_baremetal_<name>
#   app/freertos/<name>  -> APP_TARGET=app_freertos_<name>
#
# Build directories follow CMakePresets.json (build/<debug|release>).
#
# usage:
#   tools/build.sh [debug|release] <app|all|all-freertos> [<app> ...] [--flash]
#
#   all           -> every baremetal app
#   all-freertos  -> every freertos app
set -uo pipefail

usage() {
    echo "usage: $0 [debug|release] <app|all|all-freertos> [<app> ...] [--flash]" >&2
    exit 1
}

[ "$#" -ge 1 ] || usage

config="debug"
[ "$1" = "debug" ] || [ "$1" = "release" ] && { config="$1"; shift; }

flash=0
apps=()
for arg in "$@"; do
    case "$arg" in
        --flash) flash=1 ;;
        --debug) config="debug" ;;
        --release) config="release" ;;
        *) apps+=("$arg") ;;
    esac
done
[ "${#apps[@]}" -ge 1 ] || usage

root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Resolve an app name to "<os>/<name>"; prints nothing when not found.
resolve_app() {
    local name="$1"
    if [ -d "$root/app/baremetal/$name" ]; then
        echo "baremetal/$name"
    elif [ -d "$root/app/freertos/$name" ]; then
        echo "freertos/$name"
    fi
}

expanded=()
for app in "${apps[@]}"; do
    case "$app" in
        all)
            for dir in "$root"/app/baremetal/[0-9]*; do
                expanded+=("baremetal/$(basename "$dir")")
            done
            ;;
        all-freertos)
            for dir in "$root"/app/freertos/[0-9]*; do
                expanded+=("freertos/$(basename "$dir")")
            done
            ;;
        *)
            if [[ "$app" == */* ]]; then
                expanded+=("$app")   # explicit "<os>/<name>"
            else
                resolved="$(resolve_app "$app")"
                if [ -z "$resolved" ]; then
                    echo "!!! app not found: $app" >&2
                    exit 1
                fi
                expanded+=("$resolved")
            fi
            ;;
    esac
done

cd "$root"
failed=0
for entry in "${expanded[@]}"; do
    os="${entry%%/*}"
    app="${entry##*/}"
    target="app_${os}_${app}"
    echo "=== $config / $os / $app"
    if ! cmake --preset "$config" -DAPP_TARGET="$target"; then
        echo "!!! configure failed: $app"
        failed=1
        continue
    fi
    if ! cmake --build --preset "$config"; then
        echo "!!! build failed: $app"
        failed=1
        continue
    fi
    if [ "$flash" -eq 1 ]; then
        if ! cmake --build --preset "$config" --target flash; then
            echo "!!! flash failed: $app"
            failed=1
        fi
    fi
done
exit "$failed"
