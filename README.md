Base expert/wizard class for RAD studio
=======================================

Features
--------

* Supports both BPL and DLL projects (**DLL not tested yet**)
* Initializes several basic RAD Studio interfaces
* Logs output to file `%TEMP%\%wizardname%.log` (in DEBUG configuration) and to standard debug channel (view the messages in RAD Studio's Message tool window or with DbgView application)
* Optional config via registry (default path is `HKCU\Software\Embarcadero\BDS\%BDSver%\Experts\%SWizardID%`)
* Optional delayed init - gives RAD Studio some time to startup
* Optional forms support
* Optional menu item support (item will be placed inside `Help` menu)

Usage principles
----------------

* At first, add `$(BDS)\source\ToolsAPI` to library and browsing path (`Tools > Options > Delphi options > Library path`, Browsing path)
* Due to package compiling limitations, two packages may not use a unit with the same name (see [Package compiling tricks](#package-compiling-tricks) section below). So you'll have to use "include hack": create unit `%YourWizardFolder%\%YourWizardName%.BaseWiz.pas` with the following contents:


```pascal
   unit %YourWizardName%.BaseWiz;
   {$I BaseWiz.inc}
```


and use it as usual. Consider adding the path to BaseWizard folder to your project's search path (`Project > Options... > Delphi compiler > Search path`).
* Next you'll have to add a define of your project type: package/library (`Project > Options... > Delphi compiler > Conditional defines`). These are `BW_Pack` or `BW_Lib`. Don't try to define both - this could lead to collapse of the Universe :).
* Then you'll have to inherit your own wizard class from `TBaseWizard`, implement `function  CreateInstFunc: TBaseWizard` that would return a new instance of your wizard class and assign `%YourWizardName%.BaseWiz.pas` unit's `SWizard*` and `CreateInstFunc` variables inside initialization section.
* Please read the next section regarding package compiling limitations.

Package compiling tricks
------------------------

When building packages or libraries with `Build with runtime packages` option turned on, RAD Studio forbids usage of units with the same name. If you get an error

`Cannot load package 'X'.
It contains unit 'Y', which is also contained in package 'Z'`

you've got the point.
There are two ways of dealing with the issue:

1. Separate all the used units into package and add it to require list. This is good option for utility units but you'll have to provide that package along with your wizard.
2. Rename the units that caused the conflict. That's what we got to do with wizards.
Probably you have already used this way with "include hack". It is quite tricky but it's the most simple way of using one base class for wizards. Luckily you won't have to do that kind of stuff with another units.

**All you've got to do is name all the units of your wizard in unique way.**

Of course, there are many flavors to do it but I advice using "namespaces" just like RAD Studio does (`System.SysUtils`, `Vcl.Forms` etc). So if your wizard is, say, a tetris integrated into IDE, use prefix `TetWiz` for all the used units: `TetWiz.MainWiz`, `TetWiz.FormSettings`, `TetWiz.FormMain` etc. Thus you'll create a completely autonomous package which won't conflict with another one (maybe yours too!), even if it would have `MainWiz` and `FormSettings` units as well.

Wizard architecture
-------------------

IDE behaves with wizards quite randomly. It could unload them, load again, destroy the object or just ignore it (destructor won't be executed). So some principles should be followed to avoid errors. BaseWizard class has four points of applying custom init/fin actions and all of them have their own use cases.

* **constructor Create(Options: TWizardOptions)**

Constructor is mainly used for defining wizard options in descendants but fits for creating stuff too. Keep in mind that objects you create here could bypass freeing in destructor. **NEVER** try to create here something that will be freed in Cleanup! Follow the rule: what is created in constructor should be freed in destructor.

* **destructor Destroy**

Destructor seem to seldom be called though this happens sometimes. Place only non-critical freeings here. All valuable actions (setting saving etc) must be performed in Cleanup. Freeing of custom objects in descendants SHOULD be performed AFTER calling inherited destructor as it calls Cleanup before actual freeing:

```pascal
destructor TMyWizard.Destroy;
begin
  inherited;
  FreeAndNil(...);
end;
```

* **procedure Startup**

The main point for create/init things. Perform any init actions but remember that Startup/Cleanup pair could be executed ANY NUMBER of times during a single IDE run time and a wizard's lifecycle. Follow the rule: what is created in Startup should be freed in Cleanup. Use `TBaseWizard.WasStartup` to check if Startup was already executed.

* **procedure Cleanup**

Method is called when the wizard is about to be unloaded. You must implement all main closes/frees/etc here because destructor is seldom called for the wizard! Use `TBaseWizard.WasCleanup` to check if Cleanup was already executed.


Sample
------

Here's the simplest sample of TBaseWizard descendant.

```pascal

... create stub unit Wizard.BaseWiz.pas with the "include hack"
... set BW_Pack or BW_Lib define in project options ...
... set BW_* optional defines in project options if needed ...

uses ... Wizard.BaseWiz ...

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
  ... read options using ConfigKey property ...
end;

procedure TWizard.Cleanup;
begin
  ... saving, closing, freeing, etc ...

  ... write options using ConfigKey property ...
end;

initialization
  Wizard.BaseWiz.SWizardName := '...';
  Wizard.BaseWiz.SWizardID := '...';
  Wizard.BaseWiz.SWizardMenuItem := '...';
  Wizard.BaseWiz.CreateInstFunc := CreateInstFunc;
end.
```

You may also check a slightly more advanced sample in demo project.