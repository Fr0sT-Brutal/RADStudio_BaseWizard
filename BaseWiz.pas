(*******************************************************************************
  Base expert/wizard for RAD studio class
    Config via registry, delayed init, forms support
  Defines:
    BW_Pack - the project is BPL package
    BW_Lib  - the project is DLL expert
    BW_UseForms - support forms (there's some trick with forms from DLL/BPL)
    BW_UseMenuItem - an item with caption = SWizardMenuItem will be added to
      Help menu. Pressing it will launch wizard's Execute method.
  Usage:
    See README and demo project

  © Fr0sT
*******************************************************************************)

unit BaseWiz;

interface

uses Windows, SysUtils, Registry,
     {$IFDEF BW_UseForms} Classes, Forms, {$ENDIF}
     ExtCtrls, // for TTimer
     ToolsApi;

type
  TWizardOption = (
    optUseConfig,    // use fixed registry key for config
    optUseDelayed    // use delayed init (timer checking for RAD app to be ready)
  );
  TWizardOptions = set of TWizardOption;

  // TNotifierObject has stub implementations for the necessary but unused IOTANotifer methods
  TBaseWizard = class(TNotifierObject, IOTAWizard {$IFDEF BW_UseMenuItem}, IOTAMenuWizard{$ENDIF})
  private
    procedure TimerTimer(Sender: TObject);
  protected
    FOptions: TWizardOptions; // Options that could be set by descendants to control base behaviour
    FConfigKey: TRegistry;    // config storage
  public
    constructor Create(Options: TWizardOptions);
    destructor Destroy; override;
    // Launched periodically by timer to check if RAD is loaded completely
    function CheckReady: Boolean; virtual; abstract;
    // First Wizard launch
    procedure Startup; virtual; abstract;
    // For debug
    procedure Log(const msg: string);
    {$IFDEF BW_UseMenuItem}
    // Method is called when a user clicks the wizard's menu item in Help menu
    procedure Execute; virtual; abstract;
    {$ENDIF}

    // IOTAWizard interface methods(required for all wizards/experts)
    function GetIDString: string;
    function GetName: string;
    function GetState: TWizardState;
    {$IFDEF BW_UseMenuItem}
    function GetMenuText: string;
    {$ENDIF}
  end;

  // Unit needs to create an instance of desired class but it couldn't know about it
  // so the descendants must provide a callback
  TCreateInstFunc = function: TBaseWizard;

// base wizard props, must be inited!
var
  {$IFDEF BW_UseMenuItem}
  SWizardMenuItem,
  {$ENDIF}
  SWizardName,
  SWizardID: string;
  CreateInstFunc: TCreateInstFunc;

// global interfaces
var
  INSrv: INTAServices;
  IOSrv: IOTAServices;
  ModuleSrv: IOTAModuleServices;
  ActionSrv: IOTAActionServices;

resourcestring
  SMsgPropsNotInited = 'Wizard props not assigned, set SWizardName, SWizardID and CreateInstFunc variables';
  SMsgUnsupportedIDE = 'Necessary IDE service not found';
  SMsgRegKeyFail = 'Cannot open/create registry key ';
  SMsgErrorRegistering = 'Error registering expert %s.'#13#10'%s';

  SConfigBasePath = '\Experts\'; // config path in registry is %BDS_base%\%SConfigBasePath%\%SWizardID%

{$IFDEF BW_Pack}
procedure Register;
{$ENDIF}
{$IFDEF BW_Lib}
function InitWizard(const ABorlandIDEServices : IBorlandIDEServices;
                    RegisterProc : TWizardRegisterProc;
                    var Terminate: TWizardTerminateProc) : Boolean; stdcall;

exports
  InitWizard name WizardEntryPoint;
{$ENDIF}

implementation

{$IFDEF BW_UseForms}
var
  OldAppHandle: THandle;
{$ENDIF}

// init some global variables
procedure InitGlobals;
begin
  // check inits
  if (SWizardName = '') or (SWizardID = '') or not Assigned(CreateInstFunc) then
    raise Exception.Create(SMsgPropsNotInited);
  // get IDE interfaces
  if not (
    Supports(BorlandIDEServices, INTAServices, INSrv) and
    Supports(BorlandIDEServices, IOTAServices, IOSrv) and
    Supports(BorlandIDEServices, IOTAModuleServices, ModuleSrv) and
    Supports(BorlandIDEServices, IOTAActionServices, ActionSrv)
  ) then raise Exception.Create(SMsgUnsupportedIDE);
  {$IFDEF BW_UseForms}
  // ! change Application handle to the host app to be able to show forms
  OldAppHandle := Application.Handle;
  Application.Handle := IOSrv.GetParentHandle;
  {$ENDIF}
end;

{$IFDEF BW_Pack}
procedure Register;
begin
  try
    InitGlobals;
    RegisterPackageWizard(CreateInstFunc as IOTAWizard);
  except on E: Exception do
    MessageBox(0, PChar(Format(SMsgErrorRegistering, [SWizardName, E.Message])), nil, MB_OK and MB_ICONERROR);
  end;
end;
{$ENDIF}

{$IFDEF BW_Lib}
function InitWizard(const ABorlandIDEServices : IBorlandIDEServices;
                    RegisterProc : TWizardRegisterProc;
                    var Terminate: TWizardTerminateProc) : Boolean;
begin
  Result := False;
  BorlandIDEServices := ABorlandIDEServices;
  try
    InitGlobals;
    RegisterProc(CreateInstFunc as IOTAWizard);
    Result := True;
  except on E: Exception do
    Log(Format(SMsgErrorRegistering, [SWizardName, ExceptionInfo(E)]));
  end;
end;
{$ENDIF}

{$REGION 'TBaseWizard'}

constructor TBaseWizard.Create(Options: TWizardOptions);
var
  Timer: TTimer;
begin
  {$IFDEF DEBUG}
  Log('Create');
  {$ENDIF}
  inherited Create;
  FOptions := Options;

  // init config
  if optUseConfig in FOptions then
  begin
    FConfigKey := TRegistry.Create(KEY_ALL_ACCESS);
    FConfigKey.RootKey := HKEY_CURRENT_USER;
    if not FConfigKey.OpenKey(IOSrv.GetBaseRegistryKey + SConfigBasePath + SWizardID, True) then
      raise Exception.Create(SMsgRegKeyFail + SConfigBasePath + SWizardID);
  end;

  // start timer
  if optUseDelayed in FOptions then
  begin
    // timer to wait for IDE to load completely
    Timer := TTimer.Create(nil);
    Timer.Interval := 300;
    Timer.OnTimer := TimerTimer;
    Timer.Enabled := True;
  end
  else
    Startup;
end;

destructor TBaseWizard.Destroy;
begin
  {$IFDEF DEBUG}
  Log('Destroy');
  {$ENDIF}

  if optUseConfig in FOptions then
    FreeAndNil(FConfigKey);
  inherited;
end;

procedure TBaseWizard.Log(const msg: string);
var
  TmpPath, Inst: string;
  Res: Integer;
  Logfile: TextFile;
begin
  Inst := IntToHex(NativeUInt(Self), 1);
  OutputDebugString(PChar(SWizardName + '.' + Inst + ' ' + msg));
  Res := GetTempPath(0, nil);
  if Res = 0 then Exit;
  SetLength(TmpPath, Res);
  Res := GetTempPath(Res, PChar(TmpPath));
  if Res = 0 then Exit;
  SetLength(TmpPath, Res); // the path returned with leading #0 so truncate it
  AssignFile(Logfile, TmpPath+SWizardName+'.log');
  {$I-}
  Append(Logfile);
  if IOResult <> 0 then
    Rewrite(Logfile);
  if IOResult <> 0 then
    Exit;
  {$I+}
  Writeln(Logfile, DateTimeToStr(Now)+#9+Inst+#9+msg);
  CloseFile(Logfile);
end;

// ! this timer waits for IDE to load completely (i.e., when main menu is created,
// toolbars are read from registry...)
procedure TBaseWizard.TimerTimer(Sender: TObject);
begin
  if CheckReady then
  begin
    Sender.Free;  // switch the timer off
    Startup;      // launch
  end;
end;

// *** wizard properties ***

function TBaseWizard.GetName: string;
begin
  Result := SWizardName;
end;

function TBaseWizard.GetState: TWizardState;
begin
  Result := [wsEnabled];
end;

function TBaseWizard.GetIDString: string;
begin
  Result := SWizardID;
end;

{$IFDEF BW_UseMenuItem}
function TBaseWizard.GetMenuText: string;
begin
  Result := SWizardMenuItem;
end;
{$ENDIF}

{$ENDREGION}

initialization

finalization
  {$IFDEF BW_UseForms}
  Application.Handle := OldAppHandle;
  {$ENDIF}
end.
