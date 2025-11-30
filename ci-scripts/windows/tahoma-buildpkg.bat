@echo off

cd toonz

mkdir build

cd build

echo ">>> Creating Tahoma2D directory"

IF EXIST Tahoma2D rmdir /S /Q Tahoma2D

mkdir Tahoma2D

echo ">>> Creating Tahoma2D Windows Portable package"

xcopy /Y /E /I ..\..\stuff Tahoma2D\tahomastuff

echo ">>> Remove unnecessary files"
REM Remove github keep files
del /A- /S Tahoma2D\tahomastuff\*.gitkeep

7z --help

IF EXIST Tahoma2D-portable-win.zip del Tahoma2D-portable-win.zip
7z a -r0 Tahoma2D-portable-win.zip Tahoma2D\*

7z l Tahoma2D-portable-win.zip

cd ../..
