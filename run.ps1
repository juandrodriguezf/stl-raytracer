$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $root

python -c "import PIL" 2>$null
if ($LASTEXITCODE -ne 0) {
    Write-Host "Falta Pillow. Instale las dependencias con:"
    Write-Host "  python -m pip install -r requirements.txt"
    exit 1
}

.\build.ps1
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

if (Test-Path "frames") {
    Remove-Item -Force "frames\*.pgm" -ErrorAction SilentlyContinue
}
if (Test-Path "png") {
    Remove-Item -Force "png\*.png" -ErrorAction SilentlyContinue
}

.\raytracer.exe -i busto.stl -o frames -n 120 -w 400 -H 400 -s 2
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

python pgm2png.py frames png --gif animation.gif --fps 30
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Listo: frames en frames\, PNG en png\ y animacion en animation.gif"
