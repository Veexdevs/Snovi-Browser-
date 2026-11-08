#!/bin/sh
set -eu
HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
# Activate the project's venv automatically if it exists; otherwise use system Python.
ROOT=$(CDPATH= cd -- "$HERE/../../.." && pwd)
if [ -x "$ROOT/.venv/bin/python" ]; then
    PYTHON="$ROOT/.venv/bin/python"
else
    PYTHON=python3
fi
exec "$PYTHON" "$HERE/main.py" "$@"
