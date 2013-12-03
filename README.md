Base expert/wizard class for RAD studio
=======================================

Features:
--------
* Supports both BPL and DLL projects
* Initializes several basic RAD studio interfaces
* Log output to file %TEMP%\%wizardname%.log and standard debug channel (view messages in Message tool window in RAD studio or DbgView application)
* Optional config via registry
* Optional delayed init - give RAD studio some time to startup
* Optional forms support
* Optional menu item support (item will be placed inside Help menu)

Usage:
-----

Here's the simplest sample of TBaseWizard descendant.

... set BW_* defines in project options if needed ...

```pascal
type
  TWizard = class(TBaseWizard)
  public
    constructor Create;
    destructor Destroy; override;

    function CheckReady: Boolean; override;
    procedure Startup; override;
    procedure Execute; override;
    procedure Cleanup; override;
  end;

implementation

function CreateInstFunc: TBaseWizard;
begin
  Result := TWizard.Create;
end;

function TWizard.CheckReady: Boolean;
begin
  Result := INSrv.MainMenu <> nil; // wait for main menu to be created
end;

constructor TWizard.Create;
begin
  inherited Create([optUseConfig, optUseDelayed]); // set desired options

  // we'll read options later, on Startup
end;

destructor TWizard.Destroy;
begin
  ... write options using FConfigKey property ...

  inherited;
end;

procedure TWizard.Execute;
begin
  ... if using menu item, here implement some reaction on its click ...
end;

procedure TWizard.Startup;
begin
  ... read options using FConfigKey property ...
end;

procedure TWizard.Cleanup;
begin
  ... saving, closing, freeing, etc ...
end;

initialization
  BaseWiz.SWizardName := '...';
  BaseWiz.SWizardID := '...';
  BaseWiz.SWizardMenuItem := '...';
  BaseWiz.CreateInstFunc := CreateInstFunc;
end.
```