(*******************************************************************************
  Base expert/wizard for RAD studio class
    Config via registry, delayed init, forms support
  Defines:
    BW_Pack - the project is BPL package
      or
    BW_Lib  - the project is DLL expert
    BW_UseForms - support forms (there's some trick with forms from DLL/BPL)
    BW_UseMenuItem - an item with caption %SWizardMenuItem% will be added to
      Help menu. Pressing it will launch wizard's Execute method. This is not
      adjustable, if you want custom placement in menu, you should add it via
      INSrv.MainMenu
  Usage:
    See README and demo project

  © Fr0sT
*******************************************************************************)

//unit BaseWiz; ! Due to package compiling limitations, two packages may not use
//                the same unit. So you'll have to use include-hack (see README)

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
  strict private
    FTimer: TTimer;
    FConfigKey: TRegistry;    // config storage
    FWasStartup: Boolean;     // flag showing that DoStartup was executed
    FWasCleanup: Boolean;     // flag showing that DoCleanup was executed
    procedure TimerTimer(Sender: TObject);
  private // used in DoneWiz proc
    procedure DoStartup;
    procedure DoCleanup;
  protected
    property ConfigKey: TRegistry read FConfigKey;
    property WasCleanup: Boolean  read FWasCleanup;
    property WasStartup: Boolean  read FWasStartup;
  public
    // Constructor is mainly used for defining wizard options in descendants
    // but fits for creating stuff too. Keep in mind that stuff you create here
    // could bypass freeing in destructor (see below).
    // NEVER try to create stuff here that will be freed in Cleanup!
    constructor Create(Options: TWizardOptions);
    // Destructor seem to seldom be called! Place non-critical freeings here.
    // All valuable actions (setting saving etc) must be performed in Cleanup.
    // Freeing of custom stuff in descendants SHOULD be performed AFTER calling
    // inherited destructor as it calls Cleanup before actual freeing.
    destructor Destroy; override;
    // Launched periodically by timer to check if RAD is loaded completely
    function CheckReady: Boolean; virtual; abstract;
    // First Wizard launch. Place all the create/init things here.
    // Startup/Cleanup pair could be executed ANY NUMBER of times during a single
    // IDE run time so be warned.
    procedure Startup; virtual; abstract;
    // Method is called when the wizard is about to be freed. You must implement
    // all main closes/frees/etc here because destructor is seldom called for the wizard!
    procedure Cleanup; virtual; abstract;
    // Method is called when a user clicks the wizard's menu item in Help menu
    // Don't try to comment it out - it is contained in IOTAWizard
    procedure Execute; virtual; abstract;
    // For debug
    procedure Log(const msg: string);

    // IOTAWizard interface methods(required for all wizards/experts)
    function GetIDString: string;
    function GetName: string;
    function GetState: TWizardState;
    {$IFDEF BW_UseMenuItem}
    function GetMenuText: string;
    {$ENDIF}
  end;

  // Unit needs to create an instance of desired class but it couldn't know about
  // this class so the descendants must provide a callback
  TCreateInstFunc = function: TBaseWizard;

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

// base wizard props, must be inited!
var
  {$IFDEF BW_UseMenuItem}
  // Name for wizard's menu item in Help menu. Keep it empty to skip item addition.
  SWizardMenuItem,
  {$ENDIF}
  // Short wizard name. Used mainly internally and for log file name
  SWizardName,
  // Wizard ID. Likely must be unique. Used as registry key name as well.
  SWizardID: string;

  CreateInstFunc: TCreateInstFunc;

// global interfaces
var
  INSrv: INTAServices;
  IOSrv: IOTAServices;
  ModuleSrv: IOTAModuleServices;
  ActionSrv: IOTAActionServices;

resourcestring
  SMsgPropsNotInited = 'Wizard props not assigned. SWizardName, SWizardID and CreateInstFunc variables must be assigned.';
  SMsgUnsupportedIDE = 'Necessary IDE service not found';
  SMsgRegKeyFail = 'Cannot open/create registry key ';
  SMsgErrorRegistering = 'Error registering expert %s.'#13#10'%s';

  SConfigBasePath = '\'+isWizards+'\'; // config path in registry is %BDS_base%\%SConfigBasePath%\%SWizardID%

implementation

var
  Wizard: TBaseWizard;  // keep the created instance to be able to launch cleanup
{$IFDEF BW_UseForms}
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

// create and init wizard object
function InitWiz(RegisterProc: TWizardRegisterProc): Boolean;
var
  TmpWiz: TBaseWizard;
begin
  Result := False;
  try
    InitGlobals;
    TmpWiz := CreateInstFunc;
    {$IFDEF BW_Pack}
    RegisterProc := @RegisterPackageWizard;
    {$ENDIF}
    RegisterProc(TmpWiz);
    Wizard := TmpWiz;
    Result := True;
  except on E: Exception do
    MessageBox({$IFDEF BW_UseForms}Application.Handle{$ELSE}0{$ENDIF},
               PChar(Format(SMsgErrorRegistering, [SWizardName, E.Message])), PChar(SWizardName), MB_OK and MB_ICONERROR);
  end;
end;

// prepare wizard object for destruction
procedure DoneWiz;
begin
  if Wizard <> nil then
    try Wizard.DoCleanup; except end;
end;

{$IFDEF BW_Pack}
procedure Register;
begin
  InitWiz(nil);
end;
{$ENDIF}

{$IFDEF BW_Lib}
function InitWizard(const ABorlandIDEServices : IBorlandIDEServices;
                    RegisterProc : TWizardRegisterProc;
                    var Terminate: TWizardTerminateProc) : Boolean;
begin
  BorlandIDEServices := ABorlandIDEServices;
  Result := InitWiz(RegisterProc);
end;
{$ENDIF}

{$REGION 'TBaseWizard'}

constructor TBaseWizard.Create(Options: TWizardOptions);
begin
  {$IFDEF DEBUG}
  Log('TBaseWizard.Create');
  {$ENDIF}
  inherited Create;

  // init config
  if optUseConfig in Options then
  begin
    FConfigKey := TRegistry.Create(KEY_ALL_ACCESS);
    FConfigKey.RootKey := HKEY_CURRENT_USER;
    if not FConfigKey.OpenKey(IOSrv.GetBaseRegistryKey + SConfigBasePath + SWizardID, True) then
      raise Exception.Create(SMsgRegKeyFail + SConfigBasePath + SWizardID);
  end;

  // start timer
  if optUseDelayed in Options then
  begin
    // timer to wait for IDE to load completely
    FTimer := TTimer.Create(nil);
    FTimer.Interval := 300;
    FTimer.OnTimer := TimerTimer;
    FTimer.Enabled := True;
  end
  else
    DoStartup;
end;

destructor TBaseWizard.Destroy;
begin
  FreeAndNil(FTimer);  // first switch the timer off to avoid ticking after destruction
  {$IFDEF DEBUG}
  Log('TBaseWizard.Destroy');
  {$ENDIF}
  DoCleanup;
  FreeAndNil(FConfigKey);
  inherited;
end;

procedure TBaseWizard.DoStartup;
begin
  if FWasStartup then Exit;
  {$IFDEF DEBUG}
  Log('TBaseWizard.DoStartup');
  {$ENDIF}
  Startup;
  FWasStartup := True;
  FWasCleanup := False;
end;

procedure TBaseWizard.DoCleanup;
begin
  if FWasCleanup then Exit;
  {$IFDEF DEBUG}
  Log('TBaseWizard.DoCleanup');
  {$ENDIF}
  Cleanup;
  FWasCleanup := True;
  FWasStartup := False;
end;

procedure TBaseWizard.Log(const msg: string);
var
  TmpPath, Inst: string;
  Res: Integer;
  Logfile: TextFile;
begin
  Inst := IntToHex(NativeUInt(Self), 1);
  OutputDebugString(PChar(SWizardName + '.' + Inst + ' ' + msg));
  {$IFDEF DEBUG}
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
  {$ENDIF}
end;

// ! this timer waits for IDE to load completely (i.e., when main menu is created,
// toolbars are read from registry...)
procedure TBaseWizard.TimerTimer(Sender: TObject);
begin
  if FTimer = nil then Exit; // check if destructor has been executed
  if CheckReady then
  begin
    FreeAndNil(FTimer);  // switch the timer off
    DoStartup;           // launch
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
  DoneWiz;
end.
