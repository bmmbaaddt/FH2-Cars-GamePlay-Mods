FH2 Mods - a settings window for Forza Horizon 2 (Xbox One) on Azimuth
========================================================================

Change traffic, crashes, money, skills, the cockpit camera and per-car
performance (power, drag, downforce, grip, brakes, turbo) from one window,
and get keyboard keys for lights, indicators, time and weather.

Not affiliated with Microsoft, Playground Games, Turn 10 or Azimuth.
Azimuth's developer does not support modded setups: if something breaks,
use "Restore originals" before asking them for help.


REQUIREMENTS
------------
- Windows 10 or 11 (PowerShell 5.1, already part of Windows)
- Forza Horizon 2 for Xbox One (1.0.0.64) running on Azimuth 0.9.10
  (0.9.9 works too, but without ghost cars, keyboard keys and sky road)
- Python 3 on PATH, for the car list and car mods
  (python.org, tick "Add python.exe to PATH" when installing)
- ExtraOptions is included: Apply installs ExtraOptions 0.2.98 with this
  tool's additions as winmm.dll next to azimuth.exe (its settings are in
  FH2_ExtraOptions.ini beside it). An older ExtraOptions in data\bin is
  backed up and swapped for Azimuth's own helper; "Restore originals" puts
  everything back. Works with the LivingCockpit loader too.


INSTALL
-------
1. Put the "FH2 Mods" folder next to azimuth.exe:
     <Azimuth folder>\azimuth.exe
     <Azimuth folder>\FH2 Mods\FH2 Mods.bat
   The game itself can be anywhere (any folder or drive): the tool finds it
   through Azimuth's settings, so add the game in Azimuth and start it once.
2. Double-click "FH2 Mods.bat".

The first run copies the original game files it changes into
<Azimuth folder>\backup_original. Every Apply rebuilds from that copy,
so settings never stack up and a car set back to x1 is really stock again.

Updating from an older version: replace the whole "FH2 Mods" folder (keep
settings.json and cars.json if you want your values), then press Apply.


USE
---
- Close the game first: the tool refuses to change files while it runs.
- Change values, then Apply (or Apply & Play to also start the game).
- Restore originals puts every original file back.

Traffic        free roam density in percent (0 = none, 100 = normal, up to 300),
               traffic on/off in races and road trips,
               ghost cars: drive through every other car (they stay visible)
Crashes        less spinning and flipping (reduces it, does not remove it),
               slide along walls, barriers and trees instead of stopping dead,
               light breakables: fences, signs, posts and bins barely slow you
World keys     Ctrl shortcuts for time, weather, traffic, repair (see below)
Money          CR per level-up, CR per km driven in races
Skills         skill points and chain XP multipliers, highest chain multiplier,
               seconds allowed between skills (0 = the game's own),
               crashes not ending the skill chain
               (with the Skill Legend perk you get one above the highest
               chain multiplier)
Cockpit        field of view and seat position
Car mods       pick a car, set multipliers (x1 = stock). Grip and brakes act on the
               car's best road tires and best brakes, so fit those upgrades.
               Turbo only does something on a turbocharged engine.
               Gearing (higher = taller gears, more top speed) and rev limit
               also affect other cars that share the same swap part.
               Multipliers go up to x100, rev limit up to x2 (the game
               crashes above that). Forward gears: 1 = stock gearbox, up
               to 8. The traffic bus and box truck are in the list too.
               "Random car from my garage" picks a car you own (from your
               save) and shows it; choose that car in the game's garage.

Very high power on a car with stock tires and rear-wheel drive will mostly
spin the wheels. Raise grip and brakes along with power.

Keep your total CR under about 2,000,000,000: the game may store it as a
32-bit number.


KEYBOARD SHORTCUTS
------------------
While driving (always on):
  L        headlights on/off
  Q        left indicator (press again to switch off)
  E        right indicator (press again to switch off)
  H        hazard lights (press again to switch off)
In My Cars (the garage car list, from the main menu or the pause menu):
  R or right stick click   jump to a random car of your garage
Change them in FH2_ExtraOptions.ini next to azimuth.exe, section [Keyboard]
(a letter, a digit, F1-F12, or off). FH2 has no horn.

On a controller (from ExtraOptions 0.2.98):
  hold D-pad left / right 1 second   left / right indicator (switches off
                                     by itself after the turn)
  tap D-pad up                       hazard lights
  tap right stick                    headlights
The keyboard and controller switch the same indicators.

Also from ExtraOptions 0.2.98: aero tuning without Forza aero fitted,
"Return to Garage" in the pause menu, the new drift camera, stock rims,
and livery quality in the in-game Video settings.

World keys (free roam, tick "World keys" in the window):
  Ctrl+T   midnight
  Ctrl+Y   sunrise (06:30)
  Ctrl+U   midday
  Ctrl+I   sunset (17:50)
  Ctrl+P   freeze / unfreeze the clock
  Ctrl+W   rain
  Ctrl+R   clear weather
  Ctrl+G   traffic off / on
  Ctrl+N   repair the car
  Ctrl+B   put the car back on the road
  Ctrl+F   sky road: endless flat floor above the map, press again to
           come back down. EXPERIMENTAL AND BUGGY: the car can get stuck.
           For top speed runs, prefer low grip and low downforce instead.


USING FORZAINFINITY?
--------------------
ForzaInfinity (online play) is its own ExtraOptions build in data\bin, and
only one ExtraOptions can run. Apply leaves ForzaInfinity in place, and the
FH2 Mods winmm.dll switches itself off when it finds ForzaInfinity (or any
other ExtraOptions) in data\bin, so the game is never patched twice.
With ForzaInfinity installed:
  Works:  traffic density, crashes, light breakables, money, skills,
          cockpit camera, world key scripts
  Off:    ghost cars, keyboard keys (lights, indicators, Ctrl world keys)
          and sky road (they need the FH2 Mods ExtraOptions)
  Car mods work, but ForzaInfinity then plays OFFLINE (its server only
  takes the game's original car database).
What to do:
  - Online with ForzaInfinity: press "Restore originals" first. Its server
    checks saves; money, XP or car mods can get a profile flagged or reset.
  - Offline with every feature: run ForzaInfinity's Uninstall.cmd, then
    press Apply.


APPLY FAILED? (troubleshooting)
-------------------------------
Double-click Diagnose.bat and send the diagnose.txt it writes. It changes
nothing; it compares your game files and the tool's backup with the genuine
game files and shows what is different.
  "pattern not found in ..."  that game file was already changed by
      something else before this tool made its backup. Put the original file
      back from your game download, delete the same file from
      <Azimuth folder>\backup_original\media, then Apply again.
  "game database missing"  put the original
      Mount\media\stripped\gamedbRC.slt back from your game download.
  "game not found"  put FH2 Mods next to azimuth.exe, add Forza Horizon 2
      (the Anthem_1.0.0.64 folder) in Azimuth and start it once. FH2
      Presents Fast & Furious (Spire) is not supported.
  "backup_original ... was made from another game"  an older version (up to
      v1.0.6) changed FH2 Presents Fast & Furious instead of FH2. Press
      Restore originals in that older version, delete backup_original next
      to azimuth.exe, then Apply with this version.
  Empty car list  install Python 3 with "Add python.exe to PATH".


ADDED CARS (mediaoverride / mediapc)
-------------------------------------
Car mods that add cars through ExtraOptions' Mount\mediaoverride or
Mount\mediapc folder (for example the Rolls-Royce Wraith) ship their own
zipmanifest.xml and game database. Apply keeps that zipmanifest.xml in step
with this tool's zips and writes car mods into that database, so the added
car stays, shows up in the car list and can be modded too. Install or update
such a mod with the game closed, then press Apply again.
  FM5 Car Importer  cars it adds after an Apply are kept, and car mods are
      not applied twice. Press Restore originals before using its
      Uninstall.ps1, then Apply again.
  Moved database  car mods still work when a car mod moved the game's
      gamedbRC.slt into mediaoverride instead of copying it.
  One database  the game reads only one car database (mediaoverride before
      mediapc). Two mods that each ship a full database hide each other's
      cars, so keep added cars in one folder.
  Cockpit FOV mods  that change Camera.zip work too: leave Cockpit FOV at 48
      to keep the mod's value, or set your own.


FILE OVERRIDES (optional)
-------------------------
A file placed at "FH2 Mods\files\<zip name>\<file name>" replaces that file
inside the game's zip on Apply, for example:
  FH2 Mods\files\Camera.zip\CameraPhysics.xml
The tool's own settings are still applied on top.


FILES
-----
FH2 Mods.bat                  starts the window
FH2 Mods.ps1                  the window
apply.ps1                     writes the settings into the game files
car_power.py                  writes car mods into the game database
settings.json                 the window's values
cars.json                     per-car multipliers
Diagnose.bat, diagnose.ps1    read-only check when Apply fails
ExtraOptions\winmm.dll        ExtraOptions 0.2.98 with ghost cars, keyboard
                              keys, world keys, sky road and random
                              car added
ExtraOptions\FH2_ExtraOptions.ini   its default settings
ExtraOptions\azimuth-tls.stock.dll  Azimuth's own TLS helper, put back in
                              data\bin in place of an older ExtraOptions
ExtraOptions\source\          its source code


LICENSE
-------
The tool is MIT (LICENSE.txt). ExtraOptions\ is a modified version of
ForzaHorizon2_ExtraOptions by RokkuDayo
(https://github.com/RokkuDayo/ForzaHorizon2_ExtraOptions) and stays under
the GPL-3 (ExtraOptions\source\LICENSE).
