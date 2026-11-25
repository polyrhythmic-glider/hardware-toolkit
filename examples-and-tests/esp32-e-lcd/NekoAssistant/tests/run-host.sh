#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
TASK_TMP="$(mktemp -d)"
trap 'rm -rf "$TASK_TMP"' EXIT
FLAGS=(-std=c++17 -Wall -Wextra -Werror -pedantic)
g++ "${FLAGS[@]}" -I "$ROOT" "$ROOT/tests/host/test_interaction.cpp" -o "$TASK_TMP/interaction"
"$TASK_TMP/interaction"
g++ "${FLAGS[@]}" -I "$ROOT/tests/host" -I "$ROOT" "$ROOT/tests/host/test_scheduler.cpp" "$ROOT/NekoScheduler.cpp" -o "$TASK_TMP/scheduler"
"$TASK_TMP/scheduler"
g++ "${FLAGS[@]}" -I "$ROOT" "$ROOT/tests/host/test_display.cpp" -o "$TASK_TMP/display"
"$TASK_TMP/display"
if command -v node >/dev/null; then
  g++ "${FLAGS[@]}" -I "$ROOT" "$ROOT/tests/host/dump_pet_frames.cpp" -o "$TASK_TMP/frames"
  "$TASK_TMP/frames" > "$TASK_TMP/frames.txt"
  node "$ROOT/tests/host/test_pet_web.cjs" "$ROOT/NekoWebPage.h" "$TASK_TMP/frames.txt" "$ROOT/NekoPetAssets.h"
fi
g++ "${FLAGS[@]}" -I "$ROOT" "$ROOT/tests/host/test_clock_phrases.cpp" -o "$TASK_TMP/clock-phrases"
"$TASK_TMP/clock-phrases"
g++ "${FLAGS[@]}" -I "$ROOT" "$ROOT/tests/host/test_pet.cpp" -o "$TASK_TMP/pet"
"$TASK_TMP/pet"
g++ "${FLAGS[@]}" -I "$ROOT/tests/host" -I "$ROOT" "$ROOT/tests/host/test_navigation.cpp" -o "$TASK_TMP/navigation"
"$TASK_TMP/navigation"
printf 'Neko: buttons, hidden menu, coherent pet, partial display updates, scheduler, Orbitron clock and five-minute phrases tests passed.\n'
