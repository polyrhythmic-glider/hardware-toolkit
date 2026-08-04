#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIRECTORY="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd -- "${SCRIPT_DIRECTORY}/.." && pwd)"
ESP32_SKETCH_DIRECTORY="${PROJECT_ROOT}/TheSynthProject"
INDEX_DIRECTORY="${PROJECT_ROOT}/.clangd-build"
ESP32_BUILD_DIRECTORY="${INDEX_DIRECTORY}/esp32"
ESP32_DATABASE="${ESP32_BUILD_DIRECTORY}/compile_commands.json"
NORMALIZED_ESP32_DATABASE="${INDEX_DIRECTORY}/esp32-commands.json"
DAISY_DRY_RUN="${INDEX_DIRECTORY}/daisy-make.txt"
DAISY_ENTRY="${INDEX_DIRECTORY}/daisy-command.json"
FINAL_DATABASE_TEMP="${INDEX_DIRECTORY}/compile_commands.json.tmp"

ARDUINO_CLI_EXECUTABLE="${ARDUINO_CLI_BIN:-}"
if [[ -z "${ARDUINO_CLI_EXECUTABLE}" ]] && command -v arduino-cli >/dev/null 2>&1; then
  ARDUINO_CLI_EXECUTABLE="$(command -v arduino-cli)"
fi

if [[ -z "${ARDUINO_CLI_EXECUTABLE}" ]]; then
  for candidate in "${HOME}"/Scaricati/arduino-ide_*_Linux_64bit/resources/app/lib/backend/resources/arduino-cli; do
    if [[ -x "${candidate}" ]]; then
      ARDUINO_CLI_EXECUTABLE="${candidate}"
      break
    fi
  done
fi

if [[ -z "${ARDUINO_CLI_EXECUTABLE}" || ! -x "${ARDUINO_CLI_EXECUTABLE}" ]]; then
  echo "Errore: arduino-cli non trovato." >&2
  echo "Imposta ARDUINO_CLI_BIN=/percorso/arduino-cli e riprova." >&2
  exit 1
fi

for required_tool in jq make arm-none-eabi-g++; do
  if ! command -v "${required_tool}" >/dev/null 2>&1; then
    echo "Errore: comando richiesto non trovato: ${required_tool}" >&2
    exit 1
  fi
done

LIBDAISY_PATH="${LIBDAISY_DIR:-${PROJECT_ROOT}/../DaisyExamples/libDaisy}"
DAISYSP_PATH="${DAISYSP_DIR:-${PROJECT_ROOT}/../DaisyExamples/DaisySP}"
if [[ ! -d "${LIBDAISY_PATH}" || ! -d "${DAISYSP_PATH}" ]]; then
  echo "Errore: libDaisy o DaisySP non trovati." >&2
  echo "Imposta LIBDAISY_DIR e DAISYSP_DIR con i percorsi corretti." >&2
  exit 1
fi

mkdir -p "${ESP32_BUILD_DIRECTORY}"

echo "[1/3] Arduino ESP32: genero il database reale delle librerie..."
"${ARDUINO_CLI_EXECUTABLE}" compile \
  --fqbn esp32:esp32:esp32 \
  --build-path "${ESP32_BUILD_DIRECTORY}" \
  --only-compilation-database \
  "${ESP32_SKETCH_DIRECTORY}"

if [[ ! -s "${ESP32_DATABASE}" ]]; then
  echo "Errore: Arduino CLI non ha prodotto ${ESP32_DATABASE}." >&2
  exit 1
fi

ESP32_COMPILER="$(jq -r '.[0].arguments[0] // empty' "${ESP32_DATABASE}")"
if [[ -z "${ESP32_COMPILER}" || ! -x "${ESP32_COMPILER}" ]]; then
  echo "Errore: compilatore ESP32 non trovato nel database Arduino." >&2
  exit 1
fi

ESP32_TOOLCHAIN_DIRECTORY="$(cd -- "$(dirname -- "${ESP32_COMPILER}")/.." && pwd)"
ESP32_SYSROOT="${ESP32_TOOLCHAIN_DIRECTORY}/xtensa-esp-elf"
if [[ ! -d "${ESP32_SYSROOT}/include" ]]; then
  echo "Errore: sysroot ESP32 non trovato: ${ESP32_SYSROOT}" >&2
  exit 1
fi

# Arduino indicizza copie dello sketch nella directory di build. Rimappa quelle
# entry ai sorgenti originali e forza C++ per l'estensione .ino.
jq \
  --arg sketch_prefix "${ESP32_BUILD_DIRECTORY}/sketch/" \
  --arg source_prefix "${ESP32_SKETCH_DIRECTORY}/" \
  --arg sysroot "--sysroot=${ESP32_SYSROOT}" \
  'map(
    .arguments = (.["arguments"][0:1] + [$sysroot] + .["arguments"][1:])
    |
    if (.file | startswith($sketch_prefix)) then
      (.file | ltrimstr($sketch_prefix) | sub("\\.ino\\.cpp$"; ".ino")) as $relative_source
      | ($source_prefix + $relative_source) as $source
      | .file = $source
      | .arguments = (
          .arguments
          | map(
              if type == "string" and startswith($sketch_prefix) then
                $source_prefix + (ltrimstr($sketch_prefix) | sub("\\.ino\\.cpp$"; ".ino"))
              else . end
            )
          | .[0:1] + ["-x", "c++"] + .[1:]
        )
    else . end
  )' \
  "${ESP32_DATABASE}" > "${NORMALIZED_ESP32_DATABASE}"

echo "[2/3] Daisy: acquisisco flag ARM, libDaisy e DaisySP dal Makefile..."
make -B -n -C "${PROJECT_ROOT}/DaisySynth" \
  LIBDAISY_DIR="${LIBDAISY_PATH}" \
  DAISYSP_DIR="${DAISYSP_PATH}" > "${DAISY_DRY_RUN}"

DAISY_COMPILE_COMMAND="$(awk '/^arm-none-eabi-g\+\+ -c / { print; exit }' "${DAISY_DRY_RUN}")"
if [[ -z "${DAISY_COMPILE_COMMAND}" ]]; then
  echo "Errore: comando di compilazione Daisy non trovato nel dry-run di make." >&2
  exit 1
fi

jq -n \
  --arg directory "${PROJECT_ROOT}/DaisySynth" \
  --arg command "${DAISY_COMPILE_COMMAND}" \
  --arg file "${PROJECT_ROOT}/DaisySynth/DaisySynth.cpp" \
  '{directory: $directory, command: $command, file: $file}' > "${DAISY_ENTRY}"

echo "[3/3] Unisco i database in compile_commands.json..."
jq -s '.[0] + [.[1]]' \
  "${NORMALIZED_ESP32_DATABASE}" \
  "${DAISY_ENTRY}" > "${FINAL_DATABASE_TEMP}"
mv "${FINAL_DATABASE_TEMP}" "${PROJECT_ROOT}/compile_commands.json"

ENTRY_COUNT="$(jq 'length' "${PROJECT_ROOT}/compile_commands.json")"
echo "OK: ${PROJECT_ROOT}/compile_commands.json (${ENTRY_COUNT} comandi)."
echo "In VSCodium esegui: clangd: Restart language server"
