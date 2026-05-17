@echo off
cd /d %~dp0

setlocal enabledelayedexpansion

for %%f in (.\images\*.png) do (
    set "name=%%~nf"

    rem pega o último caractere
    set "last=!name:~-1!"

    rem só processa se for número
    if "!last!"=="1" (
        set "base=!name:~0,-1!"
        ren "%%f" "!base!_top.png"
    )

    if "!last!"=="2" (
        set "base=!name:~0,-1!"
        ren "%%f" "!base!_side.png"
    )

    if "!last!"=="3" (
        set "base=!name:~0,-1!"
        ren "%%f" "!base!_bottom.png"
    )
)

texture_packer.exe --input ./images --out atlas.png --json atlas.json --size 2048 --margin 2

pause