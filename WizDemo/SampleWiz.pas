(*******************************************************************************
                      Demo expert/wizard for RAD studio
                                Main unit

  Demonstrates principles of using the BaseWizard class.
  Adds an item to submenu Help, displays a dialog on click, saves the value in
  registry and loads it on further runs.

  © Fr0sT
*******************************************************************************)

unit SampleWiz;

interface

uses Windows, SysUtils, Registry, Dialogs,
     ToolsApi,
     SampleWiz_BaseWiz;

type
  TWizard = class(TBaseWizard)
  private
    FPersonName: string;
  public
    constructor Create;

    function CheckReady: Boolean; override;
    procedure Startup; override;
    procedure Execute; override;
    procedure Cleanup; override;
  end;

implementation

resourcestring
  SWizardName = 'DemoWizard';
  SWizardID = 'Fr0sT.DemoWizard';
  SWizardMenuItem = 'Execute demo wizard';
  SMessage = 'Hello, %s! Enter new name or leave it unchanged.';

const
  SRegKeyName = 'Person name';

function CreateInstFunc: TBaseWizard;
begin
  Result := TWizard.Create;
end;

{$REGION 'TWizard'}

constructor TWizard.Create;
begin
  inherited Create([optUseConfig, optUseDelayed]);

  // we'll read options later, on Startup
end;

function TWizard.CheckReady: Boolean;
begin
  Result := INSrv.MainMenu <> nil;
end;

procedure TWizard.Startup;
begin
  // *** read options ***

  FPersonName := FConfigKey.ReadString(SRegKeyName);
end;

procedure TWizard.Execute;
begin
  FPersonName := InputBox(SWizardName, Format(SMessage, [FPersonName]), FPersonName);
end;

procedure TWizard.Cleanup;
begin
  // ** write options **

  FConfigKey.WriteString(SRegKeyName, FPersonName);
end;

{$ENDREGION}

initialization
  SampleWiz_BaseWiz.SWizardName := SWizardName;
  SampleWiz_BaseWiz.SWizardID := SWizardID;
  SampleWiz_BaseWiz.SWizardMenuItem := SWizardMenuItem;
  SampleWiz_BaseWiz.CreateInstFunc := CreateInstFunc;

end.
