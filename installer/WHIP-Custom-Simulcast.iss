#ifndef PluginSourceDir
    #define PluginSourceDir "..\release\Release\whip-custom-simulcast"
#endif
#ifndef PluginVersion
    #define PluginVersion "0.1.0"
#endif
#ifndef InstallerOutputDir
    #define InstallerOutputDir "..\release"
#endif

#define PluginId "whip-custom-simulcast"
#define PluginName "WHIP Custom Simulcast"
#define PluginPublisher "yuyutti"
#define PluginUrl "https://github.com/yuyutti/WHIP-Custom-Simulcast"
#define PluginAppId "{CBAD3AB2-1CDE-42FD-A8F5-41E0D308B97C}"

[Setup]
AppId={{CBAD3AB2-1CDE-42FD-A8F5-41E0D308B97C}
AppName={#PluginName}
AppVersion={#PluginVersion}
AppVerName={#PluginName} {#PluginVersion}
AppPublisher={#PluginPublisher}
AppPublisherURL={#PluginUrl}
AppSupportURL={#PluginUrl}/issues
AppUpdatesURL={#PluginUrl}/releases
AppReadmeFile={app}\README.md
DefaultDirName={autoappdata}\obs-studio\plugins\{#PluginId}
DisableDirPage=yes
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog commandline
UsePreviousPrivileges=yes
UsePreviousAppDir=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0.17763
OutputDir={#InstallerOutputDir}
OutputBaseFilename={#PluginId}-{#PluginVersion}-windows-x64-setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern dynamic
SetupLogging=yes
CloseApplications=yes
RestartApplications=no
ChangesEnvironment=yes
Uninstallable=yes
UninstallLogMode=append
UninstallDisplayName={#PluginName}
VersionInfoVersion={#PluginVersion}.0
VersionInfoCompany={#PluginPublisher}
VersionInfoDescription={#PluginName} installer
VersionInfoProductName={#PluginName}
VersionInfoProductVersion={#PluginVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[Files]
Source: "{#PluginSourceDir}\*"; DestDir: "{app}"; Excludes: "*.pdb"; Flags: ignoreversion recursesubdirs createallsubdirs

[InstallDelete]
Type: files; Name: "{app}\bin\64bit\*.pdb"

[Registry]
Root: HKCU; Subkey: "Environment"; ValueType: expandsz; ValueName: "OBS_PLUGINS_PATH"; ValueData: "{app}\bin\64bit"; Flags: preservestringtype; Check: IsCurrentUserInstall
Root: HKCU; Subkey: "Environment"; ValueType: expandsz; ValueName: "OBS_PLUGINS_DATA_PATH"; ValueData: "{app}\data"; Flags: preservestringtype; Check: IsCurrentUserInstall

[CustomMessages]
english.MaintenanceCaption=Maintain %1
english.MaintenanceDescription=Choose whether to repair or uninstall the existing installation.
english.MaintenanceRepair=Repair the current installation
english.MaintenanceRepairDescription=Restore missing or damaged plugin files and keep the current configuration.
english.MaintenanceUninstall=Uninstall the plugin
english.MaintenanceUninstallDescription=Remove the plugin files. Your OBS plugin configuration will be retained.
english.MaintenanceConfirmUninstall=Exit OBS before continuing.%n%nUninstall %1 now?
english.MaintenanceUninstallFailed=The uninstaller could not be started or did not complete successfully.
english.EnvironmentConflict=Current-user installation cannot continue because %1 is already configured for another custom OBS plugin directory.%n%nExisting value:%n%2%n%nUse the all-users installation, or remove the conflicting environment variable and try again.
english.OtherScopeConflict=WHIP Custom Simulcast is already installed for the other user scope.%n%nUninstall the existing copy before changing between current-user and all-users installation.
japanese.MaintenanceCaption=%1の保守
japanese.MaintenanceDescription=インストール済みのPluginを修復するか、アンインストールするか選択してください。
japanese.MaintenanceRepair=現在のインストールを修復する
japanese.MaintenanceRepairDescription=不足または破損したPluginファイルを再配置します。現在の設定値は保持されます。
japanese.MaintenanceUninstall=Pluginをアンインストールする
japanese.MaintenanceUninstallDescription=Plugin本体を削除します。OBSに保存されたPlugin設定は保持されます。
japanese.MaintenanceConfirmUninstall=続行する前にOBSを終了してください。%n%n%1をアンインストールしますか？
japanese.MaintenanceUninstallFailed=アンインストーラーを起動できなかったか、アンインストールが正常に完了しませんでした。
japanese.EnvironmentConflict=現在のユーザーへのインストールを続行できません。%1が別のOBS Pluginフォルダーに設定されています。%n%n現在の値:%n%2%n%nすべてのユーザーへインストールするか、競合する環境変数を削除してから再実行してください。
japanese.OtherScopeConflict=WHIP Custom Simulcastが別のインストール範囲にインストールされています。%n%n現在のユーザーとすべてのユーザーを切り替える場合は、既存のPluginを先にアンインストールしてください。

[Code]
const
    UninstallRegistryKey = 'Software\Microsoft\Windows\CurrentVersion\Uninstall\{#PluginAppId}_is1';
    UserEnvironmentKey = 'Environment';
    MachineEnvironmentKey = 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment';

var
    MaintenancePage: TInputOptionWizardPage;
    MaintenanceExitRequested: Boolean;

function IsCurrentUserInstall: Boolean;
begin
    Result := not IsAdminInstallMode;
end;

function GetSelectedRootKey: Integer;
begin
    if IsAdminInstallMode then
        Result := HKLM64
    else
        Result := HKCU64;
end;

function GetInstalledUninstallerForRoot(const RootKey: Integer; var Uninstaller: String): Boolean;
begin
    Result := RegQueryStringValue(
        RootKey,
        UninstallRegistryKey,
        'UninstallString',
        Uninstaller
    );

    if Result then begin
        StringChangeEx(Uninstaller, '"', '', True);
        Result := FileExists(Uninstaller);
    end;
end;

function GetInstalledUninstaller(var Uninstaller: String): Boolean;
begin
    Result := GetInstalledUninstallerForRoot(GetSelectedRootKey, Uninstaller);
end;

function IsOtherInstallModeInstalled: Boolean;
var
    Uninstaller: String;
begin
    if IsAdminInstallMode then
        Result := GetInstalledUninstallerForRoot(HKCU64, Uninstaller)
    else
        Result := GetInstalledUninstallerForRoot(HKLM64, Uninstaller);
end;

function IsSelectedModeInstalled: Boolean;
var
    Uninstaller: String;
begin
    Result := GetInstalledUninstaller(Uninstaller);
end;

function EnvironmentValueConflicts(const ValueName, ExpectedValue: String; var ExistingValue: String): Boolean;
begin
    ExistingValue := '';
    if RegQueryStringValue(HKCU, UserEnvironmentKey, ValueName, ExistingValue) and
        (ExistingValue <> '') and (CompareText(ExistingValue, ExpectedValue) <> 0) then begin
        Result := True;
        Exit;
    end;

    ExistingValue := '';
    Result := RegQueryStringValue(HKLM64, MachineEnvironmentKey, ValueName, ExistingValue) and
        (ExistingValue <> '') and (CompareText(ExistingValue, ExpectedValue) <> 0);
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
var
    ExistingValue: String;
    ExpectedValue: String;
begin
    Result := '';
    if IsOtherInstallModeInstalled then begin
        Result := CustomMessage('OtherScopeConflict');
        Exit;
    end;

    if not IsCurrentUserInstall then
        Exit;

    ExpectedValue := ExpandConstant('{app}\bin\64bit');
    if EnvironmentValueConflicts('OBS_PLUGINS_PATH', ExpectedValue, ExistingValue) then begin
        Result := FmtMessage(CustomMessage('EnvironmentConflict'), ['OBS_PLUGINS_PATH', ExistingValue]);
        Exit;
    end;

    ExpectedValue := ExpandConstant('{app}\data');
    if EnvironmentValueConflicts('OBS_PLUGINS_DATA_PATH', ExpectedValue, ExistingValue) then
        Result := FmtMessage(CustomMessage('EnvironmentConflict'), ['OBS_PLUGINS_DATA_PATH', ExistingValue]);
end;

procedure UpdateMaintenanceDescription(Sender: TObject);
begin
    if MaintenancePage.SelectedValueIndex = 0 then
        MaintenancePage.SubCaptionLabel.Caption := CustomMessage('MaintenanceRepairDescription')
    else
        MaintenancePage.SubCaptionLabel.Caption := CustomMessage('MaintenanceUninstallDescription');
end;

procedure InitializeWizard;
begin
    MaintenancePage := CreateInputOptionPage(
        wpSelectDir,
        FmtMessage(CustomMessage('MaintenanceCaption'), ['{#PluginName}']),
        CustomMessage('MaintenanceDescription'),
        '',
        True,
        False
    );
    MaintenancePage.Add(CustomMessage('MaintenanceRepair'));
    MaintenancePage.Add(CustomMessage('MaintenanceUninstall'));
    MaintenancePage.SelectedValueIndex := 0;
    MaintenancePage.CheckListBox.OnClickCheck := @UpdateMaintenanceDescription;
end;

function ShouldSkipPage(PageID: Integer): Boolean;
begin
    Result := False;
    if PageID = MaintenancePage.ID then
        Result := WizardSilent or not IsSelectedModeInstalled;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
    if CurPageID = MaintenancePage.ID then
        UpdateMaintenanceDescription(nil);
end;

function NextButtonClick(CurPageID: Integer): Boolean;
var
    Uninstaller: String;
    ResultCode: Integer;
begin
    Result := True;
    if (CurPageID <> MaintenancePage.ID) or (MaintenancePage.SelectedValueIndex <> 1) then
        Exit;

    Result := False;
    if MsgBox(
        FmtMessage(CustomMessage('MaintenanceConfirmUninstall'), ['{#PluginName}']),
        mbConfirmation,
        MB_YESNO
    ) <> IDYES then
        Exit;

    if not GetInstalledUninstaller(Uninstaller) then begin
        MsgBox(CustomMessage('MaintenanceUninstallFailed'), mbError, MB_OK);
        Exit;
    end;

    if not Exec(
        Uninstaller,
        '/SILENT /SUPPRESSMSGBOXES /NORESTART',
        '',
        SW_SHOWNORMAL,
        ewWaitUntilTerminated,
        ResultCode
    ) or (ResultCode <> 0) then begin
        MsgBox(CustomMessage('MaintenanceUninstallFailed'), mbError, MB_OK);
        Exit;
    end;

    MaintenanceExitRequested := True;
    WizardForm.Close;
end;

procedure CancelButtonClick(CurPageID: Integer; var Cancel, Confirm: Boolean);
begin
    if MaintenanceExitRequested then
        Confirm := False;
end;

procedure RemoveOwnedEnvironmentValue(const ValueName, ExpectedValue: String);
var
    ExistingValue: String;
begin
    if RegQueryStringValue(HKCU, UserEnvironmentKey, ValueName, ExistingValue) and
        (CompareText(ExistingValue, ExpectedValue) = 0) then
        RegDeleteValue(HKCU, UserEnvironmentKey, ValueName);
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
    if CurUninstallStep = usUninstall then begin
        RemoveOwnedEnvironmentValue('OBS_PLUGINS_PATH', ExpandConstant('{app}\bin\64bit'));
        RemoveOwnedEnvironmentValue('OBS_PLUGINS_DATA_PATH', ExpandConstant('{app}\data'));
    end;
end;
