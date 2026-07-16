param(
    [string]$Port = ""
)

$ErrorActionPreference = "Stop"
$Project = Split-Path -Parent $PSScriptRoot
Push-Location $Project
try {
    idf.py set-target esp32s3
    idf.py build
    if ($Port) {
        idf.py -p $Port flash monitor
    }
}
finally {
    Pop-Location
}
