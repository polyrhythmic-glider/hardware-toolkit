#!/usr/bin/env bash
set -euo pipefail
PROJECT_ROOT="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
NEKO_ROOT="$PROJECT_ROOT/experiments/esp32-e-lcd"
"$NEKO_ROOT/NekoAssistant/tests/run-host.sh"
TEST_BUILD="$PROJECT_ROOT/.build/tests/neko-idf"
mkdir -p "$TEST_BUILD"
g++ -std=c++17 -Wall -Wextra -Werror -pedantic \
  -I "$NEKO_ROOT/neko-assistant/components/neko_board/include" \
  "$NEKO_ROOT/neko-assistant/tests/host/test_algorithms.cpp" \
  -o "$TEST_BUILD/neko_algorithms_test"
"$TEST_BUILD/neko_algorithms_test"
printf 'Neko ESP-IDF: host algorithm tests passed.\n'
