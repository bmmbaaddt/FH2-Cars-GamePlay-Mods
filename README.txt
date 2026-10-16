FH2 Mods - a settings window for Forza Horizon 2 (Xbox One) on Azimuth
========================================================================

Change traffic, money, skills, the cockpit camera and per-car performance
(power, drag, downforce, grip, brakes, turbo) from one window.

Not affiliated with Microsoft, Playground Games, Turn 10 or Azimuth.
Azimuth's developer does not support modded setups: if something breaks,
use "Restore originals" before asking them for help.


REQUIREMENTS
------------
- Windows 10 or 11 (PowerShell 5.1, already part of Windows)
- Forza Horizon 2 for Xbox One (1.0.0.64) running on Azimuth 0.9.9
- Python 3 on PATH, for the car list and car mods
  (python.org, tick "Add python.exe to PATH" when installing)


INSTALL
-------
1. Put the "FH2 Mods" folder next to azimuth.exe:
     <Azimuth folder>\azimuth.exe
     <Azimuth folder>\FH2 Mods\FH2 Mods.bat
   The game itself can be anywhere (any folder or drive): the tool finds it
   through Azimuth's settings, so add the game in Azimuth and start it once.
2. Double-click "FH2 Mods.bat".

The first run copies the original game files it changes into
<Azimuth folder>\backup_original\media. Every Apply rebuilds from that copy,
so settings never stack up and a car set back to x1 is really stock again.


USE
---
- Close the game first: the tool refuses to change files while it runs.
- Change values, then Apply (or Apply & Play to also start the game).
- Restore originals puts every original file back.

Traffic        free roam density in percent (0 = none, 100 = normal, up to 300),
               traffic on/off in races and road trips
Money          CR per level-up, CR per km driven in races
Skills         skill points and chain XP multipliers, highest chain multiplier,
               seconds allowed between skills (0 = the game's own),
               crashes not ending the skill chain
Cockpit        field of view and seat position
Car mods       pick a car, set multipliers (x1 = stock). Grip and brakes act on the
               car's best road tires and best brakes, so fit those upgrades.
               Turbo only does something on a turbocharged engine.

Very high power on a car with stock tires and rear-wheel drive will mostly
spin the wheels. Raise grip and brakes along with power.

Keep your total CR under about 2,000,000,000: the game may store it as a
32-bit number.


FILE OVERRIDES (optional)
-------------------------
A file placed at "FH2 Mods\files\<zip name>\<file name>" replaces that file
inside the game's zip on Apply, for example:
  FH2 Mods\files\Camera.zip\CameraPhysics.xml
The tool's own settings are still applied on top.


FILES
-----
FH2 Mods.bat     starts the window
FH2 Mods.ps1     the window
apply.ps1        writes traffic/money/skills/camera into the game's zips
car_power.py     writes car mods into the game database
settings.json    the window's values
cars.json        per-car multipliers
