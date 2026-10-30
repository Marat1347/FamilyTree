[Setup]
AppName=FamilyTree
AppVersion=1.0
AppPublisher=Семейная история
DefaultDirName={autopf}\FamilyTree
DefaultGroupName=FamilyTree
UninstallDisplayIcon={app}\FamilyTree.exe
Compression=lzma2
SolidCompression=yes
OutputDir=installer
OutputBaseFilename=FamilyTree_Setup
SetupIconFile=gui_native\app_icon.ico
WizardStyle=modern
PrivilegesRequired=lowest

[Files]
Source: "gui_native\dist\FamilyTree.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "gui_native\dist\data\family_data.json"; DestDir: "{app}\data"; Flags: ignoreversion

[Icons]
Name: "{group}\FamilyTree"; Filename: "{app}\FamilyTree.exe"
Name: "{userdesktop}\Семейное дерево"; Filename: "{app}\FamilyTree.exe"; IconFilename: "{app}\FamilyTree.exe"

[Run]
Filename: "{app}\FamilyTree.exe"; Description: "Запустить FamilyTree"; Flags: nowait postinstall skipifsilent