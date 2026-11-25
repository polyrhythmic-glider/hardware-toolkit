$ErrorActionPreference = "Stop"
$Project = Split-Path -Parent $PSScriptRoot
$Build = Join-Path $Project "build-host"
cmake -S (Join-Path $Project "tests/host") -B $Build
cmake --build $Build --config Release
ctest --test-dir $Build -C Release --output-on-failure
