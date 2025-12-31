@echo off
set BUILD_DIR=build

IF NOT EXIST %BUILD_DIR% (
    mkdir %BUILD_DIR%
)

echo [+] Configuration avec Ninja...
cd %BUILD_DIR%
:: On specifie le generateur Ninja
cmake -G "Ninja" ..

echo [+] Compilation ultra-rapide via Ninja...
cmake --build .

cd ..
echo.
echo [+] Build termine avec succes !
exit