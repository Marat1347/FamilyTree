@echo off
chcp 65001 >nul
cd /d "%~dp0"
if not exist "gui_native\dist\FamilyTree.exe" (
    echo [ОШИБКА] gui_native\dist\FamilyTree.exe не найден. Сначала build_gui.cmd.
    exit /b 1
)
start "" "gui_native\dist\FamilyTree.exe"