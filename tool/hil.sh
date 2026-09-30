#!/usr/bin/env bash
#
# Build + flash a lwIP demo and validate it on hardware (serial + framebuffer
# OCR). Thin wrapper around test/hil/hil_run.py.
#
#   tool/hil.sh <app|all>      e.g. tool/hil.sh lwip_16_http
#                                   tool/hil.sh all
#
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
exec python "$here/../test/hil/hil_run.py" "$@"
