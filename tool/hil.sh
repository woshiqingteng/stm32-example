#!/usr/bin/env bash
#
# FreeRTOS example HIL runner: flash each app and verify it on hardware with a
# single persistent OpenOCD session (see test/hil/freertos_verify.py).
#
#   tool/hil.sh [app ...]      e.g. tool/hil.sh 16_event_group
#                                   tool/hil.sh            # every app/freertos/*
#
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python "$here/../test/hil/freertos_verify.py" "$@"
