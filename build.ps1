$ErrorActionPreference = "Stop"

$gcc = "C:\Strawberry\c\bin\gcc.exe"
if (-not (Test-Path $gcc)) {
    $gcc = "gcc"
}

& $gcc -std=c99 -Wall -Wextra -pedantic -O2 main.c stl.c bvh.c render.c vector.c -o raytracer.exe -lm
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}

Write-Host "Compilado: raytracer.exe"
