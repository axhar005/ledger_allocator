@echo off
set EXE_PATH=build\bin\ledger.exe

if exist %EXE_PATH% (
    echo [!] Lancement du programme...
    echo ---------------------------------
    %EXE_PATH%
    echo ---------------------------------
) else (
    echo [!] Erreur : binaire introuvable. Executez build.bat.
)
exit