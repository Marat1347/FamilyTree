@echo off
cd /d "%~dp0"

echo ============================================
echo  Building GUI: FamilyTree.exe
echo ============================================
echo.

if not exist "_obj" mkdir "_obj"
if not exist "gui_native\dist" mkdir "gui_native\dist"
if not exist "gui_native\dist\data" mkdir "gui_native\dist\data"

echo Compiling resources...
if exist "gui_native\app.rc" (
    "C:\mingw64\bin\windres.exe" gui_native\app.rc -O coff -o gui_native\app.res
    if errorlevel 1 ( echo [ERROR] windres failed. & exit /b 1 )
)

echo Compiling gui_native\main.cpp ...
"C:\mingw64\bin\g++.exe" -std=c++17 -O2 -Wall -Wextra ^
 -finput-charset=UTF-8 -fexec-charset=UTF-8 ^
 -mwindows -municode -static -static-libgcc -static-libstdc++ ^
 "gui_native\main.cpp" "gui_native\app.res" -o "_obj\FamilyTree.exe" ^
 -lgdiplus -lgdi32 -luser32 -lcomdlg32

if errorlevel 1 (
    echo [ERROR] Build failed.
    exit /b 1
)

copy /y "_obj\FamilyTree.exe" "gui_native\dist\FamilyTree.exe" >nul

if exist "data\family_data.json" (
    copy /y "data\family_data.json" "gui_native\dist\data\family_data.json" >nul
    echo [OK] data copied to gui_native\dist\data\
)

echo.
echo ============================================
echo  Done: gui_native\dist\FamilyTree.exe
echo ============================================