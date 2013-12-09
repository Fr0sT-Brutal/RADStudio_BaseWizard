Base expert/wizard class for RAD studio
=======================================

Features:
--------
* Supports both BPL and DLL projects
* Initializes several basic RAD studio interfaces
* Log output to file `%TEMP%\%wizardname%.log` and standard debug channel (view messages in Message tool window in RAD studio or DbgView application)
* Optional config via registry (by default path is `HKCU\Software\Embarcadero\BDS\%BDSver%\Experts\%SWizardID%`)
* Optional delayed init - give RAD studio some time to startup
* Optional forms support
* Optional menu item support (item will be placed inside `Help` menu)

Usage principles:
----------------
* At first, add `$(BDS)\source\ToolsAPI` to library and browsing path (Tools > Options > Delphi options > Library path, Browsing path)
* Due to package compiling limitations, two packages may not use a unit with the same name. So you'll have to use "include hack": create unit `%YourWizardFolder%\%YourWizardName%_BaseWiz.pas` with the following contents:


```pascal
   unit %YourWizardName%_BaseWiz;
   {$I BaseWiz.inc}
```


and use it as usual. Consider adding the path to BaseWizard folder to your project's search path (Project > Options... > Delphi compiler > Search path).
* Next you'll have to add a define of your project type: package/library (Project > Options... > Delphi compiler > Conditional defines). These are `BW_Pack` or `BW_Lib`. Don't try to define both - this could lead to collapse of the Universe :).
* Then you'll have to inherit your own wizard class from `TBaseWizard`, implement `function  CreateInstFunc: TBaseWizard` that would return a new instance of your wizard class and assign `%YourWizardName%_BaseWiz` unit's `SWizard*` and `CreateInstFunc` variables inside initialization section.

Sample:
------
Here's the simplest sample of TBaseWizard descendant.

```pascal

... create stub unit Wizard_BaseWiz.pas with the "include hack"
... set BW_Pack or BW_Lib define in project options ...
... set BW_* optional defines in project options if needed ...

uses ... Wizard_BaseWiz ...

type
  TWizard = class(TBaseWizard)
  public
    constructor Create;

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

procedure TWizard.Execute;
begin
  ... if using a menu item, implement some reaction on its click here ...
end;

procedure TWizard.Startup;
begin
  ... read options using FConfigKey property ...
end;

procedure TWizard.Cleanup;
begin
  ... saving, closing, freeing, etc ...

  ... write options using FConfigKey property ...
end;

initialization
  Wizard_BaseWiz.SWizardName := '...';
  Wizard_BaseWiz.SWizardID := '...';
  Wizard_BaseWiz.SWizardMenuItem := '...';
  Wizard_BaseWiz.CreateInstFunc := CreateInstFunc;
end.
```

You may also check a slightly more advanced sample in demo project.