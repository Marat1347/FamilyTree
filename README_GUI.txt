FAMILY TREE — NATIVE GUI VIEWER, first visual stage

This package contains source and a local build script, NOT a prebuilt EXE.
Requirements: Windows, C:\mingw64\bin\g++.exe and the existing project
C:\python\Scripts\F_TREE with ftree.cpp and data\family_data.json.

1. Save build-modern-gui.ps1 into the existing project folder.
2. Open PowerShell:
   Set-Location -LiteralPath 'C:\python\Scripts\F_TREE'
   powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\build-modern-gui.ps1

ExecutionPolicy Bypass applies only to that child PowerShell process.
Read the script before running it. It downloads nothing.

Output: gui_native\dist\FamilyTree.exe and data\family_data.json beside it.
Keep the data folder together with the EXE. No Qt installation needed.
The program links MinGW runtime statically; Windows system DLLs still apply.
The existing CLI source is included as the data model, with its main renamed.
It is a transitional wrapper, NOT a long-term separated architecture.

Capabilities:
- Custom GDI+ antialiased dark tree canvas and card borders.
- Male/female/central color accents, name, dates, generation.
- Zoom with mouse wheel; pan by dragging blank space.
- Click a card to view relatives in the side panel.
- Search list, case-insensitive using Windows character conversion.
- Select in list to center the person; Fit and Venera buttons.
- Reads a COPY of data/family_data.json. Does not edit or save it.

Limitations:
- Read-only viewer, not a full replacement for the CLI editor.
- No relationship editing, PNG/PDF export or layout-crossing optimizer.
- No branch columns, spouse grouping or dragging individual cards.
- Long metadata details do not automatically wrap inside cards.
- Window chrome and some controls are standard Windows controls.
- Changes made in the CLI are not auto-reloaded. Close viewer and rerun build
  script to refresh the copied data.
- Uses your existing model and parser. Their validation limitations remain.
- Windows compilation and runtime are NOT tested in the assistant environment.
  If the compiler reports an error, share the first error and following lines.
