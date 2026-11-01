FH2 Mods 2.0 - for Forza Horizon 2 (Xbox One) on Azimuth
========================================================

One app, FH2 Mods.exe. With the game closed it opens on the Setup tab:
change traffic, crashes, money, skills, the cameras, per-car performance
and packs for every car, then press Apply & Play. In the game, press Insert
for the overlay: boost, nitrous, launch control, drift, steering lock,
stunts, UFO mode, teleport, game modes, run timers and on-screen gauges.

Not affiliated with Microsoft, Playground Games, Turn 10 or Azimuth.
Azimuth's developer does not support modded setups: if something breaks,
use "Restore originals" before asking them for help.


NEW IN 2.0
----------
- One app instead of the window and the overlay. FH2 Mods.bat is gone:
  everything the old window did is on the Setup and Cars tabs, plus Apply,
  Apply & Play, Play and Restore originals. The app stays in the tray and
  becomes the overlay when the game runs. 1.x's FH2 Mods.bat, FH2 Mods.ps1
  and FH2 Mods Overlay.exe are deleted the first time it starts.
- Cars tab: search for a car or press "The car I'm in". New Rev ceiling
  (lets the rev limit go past 10,500 rpm for one car, up to 20,000; at
  your own risk). Export / Import tune to share a car's setup.
- New packs: ultra grip tires (a new tire upgrade for every car), Race+
  weight reduction (up to 60% lighter), AWD and RWD conversions for the
  cars without them, turbos for the 16 engines with no turbo option, free
  upgrades (every part and engine swap costs 1 CR).
- Engine swaps: every engine has a name (size, layout, aspiration, the car
  it comes from; 134 had none), the list is sorted by size, and a swap your
  garage has fitted always stays the same engine.
- Field of view for the hood, bumper, near chase and far chase cameras.
- New gauges: G-force meter (the dot shows the push you feel) and power
  graph (the engine's torque and power curves with your rpm on it).
- Telemetry recording to a CSV file (Gauges tab).
- Fixed: the game hung after fast travel with "Traffic and AI cars for
  sale" on.
- Diagnose also checks your garage against the game database, lists your
  settings and shows the last crashes from Azimuth's log.


REQUIREMENTS
------------
- Windows 10 or 11 (PowerShell 5.1 and .NET Framework 4.8 are built in)
- Forza Horizon 2 for Xbox One (1.0.0.64) running on Azimuth 1.0.4
  (1.0.1 to 1.0.3 work too). On an older Azimuth everything works except
  ghost cars, ExtraOptions' keys, world keys and sky road: update Azimuth
  from almost-there-games.itch.io/azimuth (your saves carry over).
- Python 3 on PATH, for the car list, car mods, packs and the power graph
  (python.org, tick "Add python.exe to PATH" when installing)
- ExtraOptions is included: Apply installs ExtraOptions 0.4.4 with this
  tool's additions (ghost cars, world keys, random car, sky road, traffic
  cars in the Autoshow) as winmm.dll next to azimuth.exe (its settings are
  in FH2_ExtraOptions.ini beside it), and its plugin mods (garage revving,
  time and weather control) in the mods folder. "Restore originals" puts
  back the one you had. You also get everything ExtraOptions 0.3 and 0.4
  added: its mods folder loader, Jump to Manufacturer, all online races
  offline, custom showcases, F1 to hide the HUD, photo mode from the
  garage, more audio and HUD options, time and weather in the pause menu
  (Y), a time slider in photo mode, daytime headlights, a vignette toggle
  and garage revving in the outposts too.


INSTALL
-------
1. Put the "FH2 Mods" folder next to azimuth.exe:
     <Azimuth folder>\azimuth.exe
     <Azimuth folder>\FH2 Mods\FH2 Mods.exe
   The game itself can be anywhere (any folder or drive): the tool finds it
   through Azimuth's settings, so add the game in Azimuth and start it once.
2. Close the game, then double-click "FH2 Mods.exe".

The first Apply copies the original game files it changes into
<Azimuth folder>\backup_original. Every Apply rebuilds from that copy,
so settings never stack up and a car set back to x1 is really stock again.

Updating from 1.x: extract the new "FH2 Mods" folder over the old one.
settings.json, cars.json and overlay.ini carry over (the new options start
switched off). Start FH2 Mods.exe, then press Apply.


USE
---
Setup and Cars change the game files: close the game, change values, then
Apply (or Apply & Play to also start the game). They are saved as you
change them (settings.json, cars.json). Restore originals puts every
original file back. All the other tabs act live in the game (overlay.ini).

Traffic        free roam density in percent (0 = none, 100 = normal, up to 300),
               traffic on/off in races and road trips,
               ghost cars: drive through every other car (they stay visible)
Crashes        less spinning and flipping (reduces it, does not remove it),
               slide along walls, barriers and trees instead of stopping dead,
               light breakables: fences, signs, posts and bins barely slow you
Money          CR per level-up, CR per km driven in races
Skills         skill points and chain XP multipliers, highest chain multiplier,
               seconds allowed between skills (0 = the game's own),
               crashes not ending the skill chain
               (with the Skill Legend perk you get one above the highest
               chain multiplier)
Packs          for every car at once: Race+ engine parts (an extra level of
               every engine part, with the extra torque you choose), engine
               swaps (every engine fits every car, sorted by size), ultra
               grip tires, Race+ weight reduction, AWD and RWD conversions,
               turbos for every engine, traffic and AI cars for sale in the
               Autoshow, free cars and free upgrades (1 CR each). A pack
               your garage uses can only be switched off after fitting the
               stock part in game.
World keys     Ctrl shortcuts for time, weather, traffic, repair (see below)
Cameras        field of view of the cockpit, hood, bumper, near chase and far
               chase cameras; cockpit seat position
Cars tab       pick a car (type to search, or "The car I'm in"), set
               multipliers (x1 = stock, up to x100): power, turbo, rev limit,
               gearing, grip, brakes, air drag, downforce; forward gears
               (1 = stock gearbox, up to 8); rev ceiling. Grip and brakes act
               on the car's best road tires and best brakes, so fit those
               upgrades. Turbo only does something on a turbocharged engine.
               Gearing (higher = taller gears, more top speed) and rev limit
               also affect other cars that share the same swap part.
               Rev limit: x2 always, more only while the engine stays under
               the rev ceiling (10,500 rpm, the game's own highest; a diesel
               that stops at 2,200 rpm can take x4). Raising the ceiling can
               crash the game when the car loads: lower it again if it does.
               "Random car from my garage" picks a car you own. Export /
               Import tune saves a car's setup to a file.

Very high power on a car with stock tires and rear-wheel drive will mostly
spin the wheels. Raise grip and brakes along with power.

Keep your total CR under about 2,000,000,000: the game may store it as a
32-bit number.


KEYBOARD SHORTCUTS
------------------
While driving (from ExtraOptions; controller in brackets):
  L                     headlights on/off (tap right stick)
  hold Q / E 0.5 s      left / right indicator, off by itself after the
                        turn (hold D-pad left / right)
  H                     hazard lights (tap D-pad up)
  hold K 1.5 s          roof on/off when stopped (hold right stick)
  J                     Autodrive to the GPS route (D-pad down)
  F1                    hide / show the HUD
In My Cars (the garage car list, from the main menu or the pause menu):
  R or right stick click   jump to a random car of your garage
  Tab or View              ExtraOptions' Jump to Manufacturer
Change them in FH2_ExtraOptions.ini next to azimuth.exe, section [Bindings]
(a letter, a digit, F1-F24, arrows, Space, Enter, Home, End, PageUp,
PageDown, Insert, Delete or None; Xbox buttons such as LB+Y). Leave F2-F12,
Home, End, Insert and Delete alone if you use the overlay. Apply keeps your
keys (also through Restore originals and when another mod replaces that
file). FH2 has no horn.

World keys (free roam, switch on "World keys" on the Setup tab):
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


IN-GAME OVERLAY
---------------
FH2 Mods.exe keeps running in the tray once the game starts (Apply & Play
and Play start the game for you). Press Insert in the game, or double-click
the tray icon. The pill at the top says LIVE while you drive; effects pause
while the menu is in front, so click the game to drive.
The game must run windowed or borderless (Azimuth's display setting).

  Insert            show / hide the menu
  End               everything off
  F6 / F7 / F8      boost on/off, less, more
  9 (hold)          nitrous
  Home              burnout (stopped)
  F5 / F9 / F11     standing / rolling / wheelie launch
  Delete            more grip (drift and steering lock have no key by default)
  F3                speed limiter
  F10               cruise control (the brake cancels it)
  F12               wheelie mode
  F4                jump
  1 / 2 / 3         barrel roll / 360 spin / backflip
  4 / 5             rocket (or LS click) / instant stop
  Backspace         flip the car upright
  6                 air control
  Page Up / Down    low gravity / UFO mode (R / F or RB / LB up, down)
  8 / 7             save a spot / teleport to it (9 spots)
  0                 ghost mode
  U                 switch mph / km/h
  F2                next game mode (off, speed bomb, score attack, pursuit)

Every key can be changed on the Keys tab. Keys only act while the game is
in front, never with Ctrl or Alt held (the Ctrl world keys keep working).
Effects change only your car, only while the overlay runs, and are put
back when you pause, open the map or a menu, change car, press End, or
close the overlay. The MPH / KM/H switch changes the gauges, the speed
settings and the run timers (0-60 to 0-200 mph and 1/8 to 1 mile, or
0-100 to 0-400 km/h and 100 to 2000 m). Stunts and the rocket need the car
moving. Single-player only: not in online sessions with other players.

Steering lock (Driving tab, Handling): how far the front wheels turn,
20-90 degrees (most cars: 42 stock). It still narrows at speed by the same
share.
Gauges tab: gas / brake graph, G-force meter (the dot shows the push you
feel: back when accelerating, forward when braking, outward in a turn) and
power graph (the fitted engine's torque and power with your rpm and the
redline; stock engine parts), all off by default. "Move gauges" lets you
drag and resize them.
Telemetry (Gauges tab): Record writes logs\telemetry-<date>.csv in the FH2
Mods folder while you drive (speed, rpm, throttle, brake, g-force, drift
angle, heading, position, 20 rows a second).
The cockpit steering wheel turns at most 90 degrees each way: the game
draws it from a fixed animation.


GAME MODES
----------
F2 while driving in free roam goes to the next mode, or start one on the
Modes tab. The mode shows at the top of the status card; End stops it.
Best results are saved.
THE GAME MODES MAY BE BUGGY, PURSUIT MOST OF ALL. If something goes wrong,
press End.

  Speed bomb     reach the bomb speed (50 mph) to arm it, then never drop
                 below it for more than half a second or the car is blown
                 up. 5 mph faster every minute. Best: longest time.
  Score attack   2 minutes: points for speed over 60 mph, drifts, air time
                 and near misses (another car within 3 m). New skills raise
                 the multiplier (up to x5); the chain banks after 3 s
                 without one; a crash loses it. Best: highest score.
  Pursuit        drivatars (traffic if there are none) are dropped behind
                 you and chase you along the roads you drove. Busted:
                 stopped next to one for 2.5 s. Wrecked: hit 3 times.
                 Escape: 250 m away for 8 s, then they come back faster
                 and more of them (up to 3). Ghosting is off during a
                 pursuit. EXPERIMENTAL: chasers are held on your route, so
                 off it they can float or sink a little ("Chaser height"
                 on the Modes tab trims it).


PROBLEMS? (Diagnose)
--------------------
Double-click Diagnose.bat and send the diagnose.txt it writes. It changes
nothing. It compares your game files and the tool's backup with the
genuine game files, shows the FH2 Mods version, your settings and car mods,
checks every car in your garage against the game database (a part that
belongs to another engine crashes the game when that car loads), and shows
the ExtraOptions log and the last crashes from Azimuth's log.
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
  The game crashes when a car loads  run Diagnose. If the garage check
      names that car, press Apply once (game closed) and try again.
  Never delete backup_original after an Apply: the next Apply would back up
      files that are already modded. If you did, put the genuine game files
      back from your game download first.


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
  Swapping car packs  a gamedbRC.slt from another car pack is taken as that
      pack's own, not mixed with the previous one. If your garage has cars
      the new pack does not have, the car mods line warns you.
  One database  the game reads only one car database (mediaoverride before
      mediapc). Two mods that each ship a full database hide each other's
      cars, so keep added cars in one folder.
  Camera FOV mods  that change Camera.zip work too: leave a camera's FOV at
      the game's own value to keep the mod's, or set your own.


FILE OVERRIDES (optional)
-------------------------
A file placed at "FH2 Mods\files\<zip name>\<file name>" replaces that file
inside the game's zip on Apply, for example:
  FH2 Mods\files\Camera.zip\CameraPhysics.xml
The tool's own settings are still applied on top.


FILES
-----
FH2 Mods.exe                  the app: Setup and Cars with the game closed,
                              then the in-game overlay (its settings, keys,
                              spots and best times in overlay.ini beside it)
apply.ps1                     writes the settings into the game files
car_power.py                  writes car mods and packs into the game
                              database; also the car list, the power graph's
                              engine and Diagnose's garage check
settings.json                 the Setup tab's values
cars.json                     the Cars tab's per-car values
logs\                         telemetry recordings
Diagnose.bat, diagnose.ps1    read-only check when something goes wrong
ExtraOptions\winmm.dll        ExtraOptions 0.4.4 with ghost cars, world
                              keys, sky road, random car and traffic cars
                              in the Autoshow added
ExtraOptions\mods\            its plugin mods (garage revving, time and
                              weather control), copied to the mods folder
ExtraOptions\FH2_ExtraOptions.ini   its default settings
ExtraOptions\source\          its source code


LICENSE
-------
The tool is MIT (LICENSE.txt). ExtraOptions\ is a modified version of
ForzaHorizon2_ExtraOptions by RokkuDayo
(https://github.com/RokkuDayo/ForzaHorizon2_ExtraOptions) and stays under
the GPL-3 (ExtraOptions\source\LICENSE).
