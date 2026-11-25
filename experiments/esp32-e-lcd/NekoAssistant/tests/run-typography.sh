#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
U8G2_DIR="${NEKO_U8G2_DIR:-$ROOT/../../../.build/neko/libraries/U8g2-2.36.19/src/clib}"
TASK_TMP="$(mktemp -d)"
trap 'rm -rf "$TASK_TMP"' EXIT
if [[ ! -f "$U8G2_DIR/u8g2_font.c" ]]; then
  echo 'Eseguire arduino-build.sh prima dei test tipografici, oppure impostare NEKO_U8G2_DIR.' >&2
  exit 1
fi
for source in u8g2_font u8g2_fonts u8x8_8x8; do
 cc -O2 -ffunction-sections -fdata-sections -I "$U8G2_DIR" -c "$U8G2_DIR/$source.c" -o "$TASK_TMP/$source.o"
done
g++ -std=c++17 -Wall -Wextra -Werror -pedantic -Wl,--gc-sections -I "$U8G2_DIR" -I "$ROOT" "$ROOT/tests/host/test_quote_layout.cpp" "$TASK_TMP/"*.o -o "$TASK_TMP/typography"
"$TASK_TMP/typography"
