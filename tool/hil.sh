#!/usr/bin/env bash
#
# HIL runner: flash each app and verify it on hardware with a single persistent
# OpenOCD session (see test/hil/hil_common.py).
#
#   tool/hil.sh [freertos|lvgl] [app ...]
#       tool/hil.sh                       # all FreeRTOS examples (default)
#       tool/hil.sh lvgl                  # all LVGL examples
#       tool/hil.sh lvgl lvgl_10_arc
#       tool/hil.sh 16_event_group        # category defaults to freertos
#
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cat="${1:-freertos}"
case "$cat" in
    freertos|lvgl|lwip) shift ;;
    *) cat="freertos" ;;
esac

exec python "$here/../test/hil/${cat}_verify.py" "$@"
