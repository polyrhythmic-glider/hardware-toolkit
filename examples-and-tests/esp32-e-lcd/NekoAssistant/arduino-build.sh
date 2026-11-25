#!/usr/bin/env bash
set -euo pipefail
SKETCH_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_DIR="$(cd -- "$SKETCH_DIR/../../.." && pwd)"
BUILD_DIR="$REPO_DIR/.build/neko"
CLI="${ARDUINO_CLI_BIN:-}"
if [[ -z "$CLI" ]] && command -v arduino-cli >/dev/null; then CLI="$(command -v arduino-cli)"; fi
if [[ -z "$CLI" ]]; then
  for candidate in "$HOME"/Scaricati/arduino-ide_*_Linux_64bit/resources/app/lib/backend/resources/arduino-cli; do
    if [[ -x "$candidate" ]]; then CLI="$candidate"; break; fi
  done
fi
[[ -x "$CLI" ]] || { echo 'Imposta ARDUINO_CLI_BIN al percorso di arduino-cli.' >&2; exit 1; }
mkdir -p "$BUILD_DIR/libraries"
if [[ ! -d "$BUILD_DIR/libraries/U8g2-2.36.19" ]]; then
  curl -fL https://downloads.arduino.cc/libraries/github.com/olikraus/U8g2-2.36.19.zip -o "$BUILD_DIR/u8g2.zip"
  (cd "$BUILD_DIR" && echo 'c9de6e5a5aa2e2884d3296c6a86be4bc2770e27b11d4302ab5134273ccb6a670  u8g2.zip' | sha256sum -c -)
  unzip -q "$BUILD_DIR/u8g2.zip" -d "$BUILD_DIR/libraries"
fi
python3 - "$BUILD_DIR" "$HOME/.arduino15" <<'PY'
import json,sys
from pathlib import Path
root=Path(sys.argv[1])
(root/'arduino-cli.yaml').write_text(json.dumps({'directories':{'data':sys.argv[2], 'downloads':str(root/'downloads'), 'user':str(root/'user')}}))
PY
FQBN='esp32:esp32:esp32s3:USBMode=hwcdc,CDCOnBoot=cdc,CPUFreq=240,FlashMode=qio,FlashSize=16M,PartitionScheme=app3M_fat9M_16MB,PSRAM=opi,UploadMode=default'
"$CLI" compile --config-file "$BUILD_DIR/arduino-cli.yaml" --fqbn "$FQBN" --libraries "$BUILD_DIR/libraries" --build-path "$BUILD_DIR/firmware" --jobs 4 "$SKETCH_DIR"
if [[ "${1:-}" == '--upload' ]]; then
  "$CLI" upload --config-file "$BUILD_DIR/arduino-cli.yaml" --fqbn "$FQBN" --input-dir "$BUILD_DIR/firmware" --upload-property upload.flags=--no-stub --upload-property upload.speed=460800 --port "${2:-/dev/ttyACM0}" "$SKETCH_DIR"
fi
