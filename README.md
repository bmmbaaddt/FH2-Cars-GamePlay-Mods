# FH2 Cars & Gameplay Mods

One app for **Forza Horizon 2 (Xbox One)** running on **Azimuth**. With the game closed, change traffic, crashes, money, skills, the cameras, per-car performance and packs for every car, then press **Apply & Play**. In the game, press `Insert` for the overlay: boost, nitrous, launch control, drift, steering lock, stunts, UFO mode, teleport, game modes, run timers and on-screen gauges. No file editing.

**2.0 is one app, `FH2 Mods.exe`.** The old settings window (`FH2 Mods.bat`) is gone: everything it did is now on the **Setup** and **Cars** tabs of the overlay's menu, which opens by itself when the game isn't running. Needs **Azimuth 1.0.4** (1.0.1 to 1.0.3 work too) and comes with ExtraOptions 0.4.4 (see [Requirements](#requirements)).

**[Download the latest release](../../releases/latest)**

> Not affiliated with Microsoft, Playground Games, Turn 10 or Azimuth. Azimuth's developer does not support modded setups: if something breaks, press **Restore originals** before asking them for help.

## What's new in 2.0.1

- **Fixed:** Apply failed with "pattern not found in AI/CarLoadingSettings.xml: MaxActiveTraffic" when another traffic mod had changed that file before FH2 Mods made its backup. The traffic cap is now scaled from whatever value the file has.

## What's new in 2.0

- **One app instead of the window and the overlay.** Double-click `FH2 Mods.exe`: with the game closed its menu opens on the **Setup** tab (everything the old window had, plus Apply, Apply & Play, Play and Restore originals). It then stays in the tray and becomes the overlay when the game runs. 1.x's `FH2 Mods.bat`, `FH2 Mods.ps1` and `FH2 Mods Overlay.exe` are deleted the first time it starts.
- **Cars tab:** pick any car (type to search) or press **The car I'm in**. New **Rev ceiling**: lets the rev limit go past the game's 10,500 rpm for one car, up to 20,000 (for slow-revving diesels; at your own risk). **Export / Import tune** saves a car's setup to a file you can share.
- **New packs:**
  - **Ultra grip tires**: a new tire upgrade for every car, after the rally tires, with the extra grip over race slicks you choose.
  - **Race+ weight reduction**: an extra stage, up to 60% lighter than stock.
  - **AWD and RWD conversions** for the cars that didn't have them.
  - **Turbos for every engine**: a twin turbo upgrade for the 16 engines with no turbo or supercharger option (Ferrari 330 P4, Lamborghini Miura, Mazda 787B and others).
  - **Free upgrades**: every part and engine swap costs 1 CR.
- **Engine swaps:**
  - Every engine now has a name, such as `4.0L V12 TT - 330P4 '67` (size, layout, aspiration, the car it comes from). 134 engines had no name before.
  - The swap list is sorted by engine size.
  - A swap your garage has fitted always stays the same engine, even when the list order changes. Before this, a re-sorted list could make a car crash the game when it loaded.
- **Camera field of view** for the hood, bumper, near chase and far chase cameras, beside the cockpit's.
- **New gauges:**
  - **G-force meter**: a circle with a one-second trail. The dot shows the push you feel: back when you accelerate, forward when you brake, outward in a turn.
  - **Power graph**: the fitted engine's torque and power curves with your rpm and the redline on it.
- **Telemetry recording:** speed, rpm, throttle, brake, g-force, drift angle, heading and position, 20 times a second, to a CSV file you can open in Excel or Google Sheets.
- **Fixed: the game hung after fast travel** with *Traffic and AI cars for sale* on. The pack no longer empties the game's traffic list. Instead, the included ExtraOptions shows those cars in the Autoshow.
- **Diagnose** now also checks your garage against the game database (it flags any car that would crash the game when it loads), lists your settings and shows the last crashes from Azimuth's log.

## Setup and Cars change the game files; the other tabs act live

| | **Setup** and **Cars** tabs | **Driving, Stunts & air, Modes, Timers, Teleport, Gauges, Keys** |
|---|---|---|
| What they change | The game files: traffic, crashes, money, skills, cameras, per-car mods, packs | Your car while you drive, live |
| When it takes effect | Close the game, **Apply**, then start the game | At once, no restart |
| Where it is saved | `settings.json`, `cars.json` (as you change them) | `overlay.ini` |

They work together: the overlay builds on what Apply wrote. Boost and nitrous multiply the car's power *after* its car mods, and switching an effect off puts your car back to exactly that.

## Features

| Section | What you can change |
|---|---|
| Traffic | Free roam density 0–300% (0 = empty roads), traffic on/off in races and road trips, **ghost cars**: drive straight through every other car (they stay visible) |
| Crashes | **Less spinning and flipping** from crashes (reduces it, does not remove it), **slide along walls**, barriers and trees instead of stopping dead, **light breakables**: fences, signs, posts and bins weigh almost nothing, so they barely slow you |
| World keys | Change the time and weather, switch traffic off, repair the car and get back on the road with Ctrl shortcuts in free roam (see below) |
| Money | CR per level-up, CR per km driven in races |
| Skills | Skill points and chain XP multipliers, highest chain multiplier (with the Skill Legend perk you get one above it), seconds allowed between skills, crashes not ending the skill chain |
| Random car | In **My Cars**, click the right stick or press `R` to jump to a random car of your garage; the Cars tab has a **Random car from my garage** button too |
| Cameras | Field of view of the cockpit, hood, bumper, near chase and far chase cameras; cockpit seat position |
| Car mods | Pick any car (the traffic bus and box truck too) and set power, air drag, downforce, tire grip, brakes, turbo boost, gearing and rev limit (x1 = stock, up to x100), the number of forward gears (up to 8), and the **rev ceiling**. **Export / Import tune** |
| Packs | For every car at once: **Race+ engine parts** (an extra level of every engine part with the extra torque you choose), **engine swaps** (every engine fits every car, sorted by size), **ultra grip tires**, **Race+ weight reduction**, **AWD and RWD conversions**, **turbos for every engine**, **traffic and AI cars for sale** in the Autoshow, **free cars** and **free upgrades** (1 CR each). A pack your garage uses can only be switched off after fitting the stock part |
| Overlay | In game: boost, nitrous, pops and bangs, anti-lag, burnout, standing / rolling / wheelie launch, drift, more grip, **steering lock** (how far the front wheels turn, 20-90°), super brakes, speed limiter, cruise control, barrel roll, 360, backflip, rocket, jump, instant stop, flip upright, air control, low gravity, UFO mode, ghost mode, 9 teleport spots, run timers with best times, drag light tree with reaction times |
| Gauges | Speed and rev panel, shift flash, nitrous strip, compass, g-force, drift angle and score, trip and top speed, status and timer card, **gas / brake graph**, **G-force meter**, **power graph**, **telemetry recording** to CSV. **mph or km/h** with one switch. Drag and resize them all |
| Game modes | In game, switchable with `F2`: **speed bomb**, **score attack**, **pursuit** (drivatars chase you), with best results kept |

- **Restore originals** puts every game file back in one click, including the original ExtraOptions.
- The original game files are backed up on first run, and every Apply rebuilds from them, so settings never stack up.
- Refuses to change the game files while the game is running.

## Keyboard shortcuts

**While driving** (from ExtraOptions):

| Key | Controller | Action |
|---|---|---|
| `L` | Tap right stick | Headlights on/off |
| Hold `Q` / `E` (0.5 s) | Hold D-pad left / right | Left / right indicator (switches off by itself after the turn) |
| `H` | Tap D-pad up | Hazard lights |
| `K` (hold 1.5 s, stopped) | Hold right stick | Roof on/off, on cars that have one |
| `J` | D-pad down | Autodrive to the GPS route |
| `F1` | | Hide / show the HUD |

**In My Cars** (the garage car list, from the main menu or the pause menu): click the **right stick** or press **`R`** to jump to a random car of your garage, then press A to drive it. `Tab` or View is ExtraOptions' Jump to Manufacturer.

Change these keys in `FH2_ExtraOptions.ini` next to `azimuth.exe`, section `[Bindings]`: a letter, a digit, F1–F24, arrows, Space, Enter, Home, End, Page Up/Down, Insert, Delete, or `None`, and Xbox buttons such as `LB+Y`. Leave F2–F12, Home, End, Insert and Delete alone if you use the overlay, which has those keys. Apply keeps your keys (also through Restore originals and when another mod replaces that file). FH2 has no horn, so there is no horn key.

**World keys** (free roam, switch on *World keys* on the Setup tab):

| Key | Action |
|---|---|
| `Ctrl+T` | Midnight |
| `Ctrl+Y` | Sunrise (06:30) |
| `Ctrl+U` | Midday |
| `Ctrl+I` | Sunset (17:50) |
| `Ctrl+P` | Freeze / unfreeze the clock |
| `Ctrl+W` | Rain |
| `Ctrl+R` | Clear weather |
| `Ctrl+G` | Traffic off / on |
| `Ctrl+N` | Repair the car |
| `Ctrl+B` | Put the car back on the road |
| `Ctrl+F` | Sky road: lifts the car onto an endless flat floor high above the map; press again to come back down. **Experimental and buggy** |

> **Sky road is buggy.** The car can get stuck or behave oddly up there. For top speed runs, keep **grip and downforce low** on the car rather than relying on the sky road.

## In-game overlay

`FH2 Mods.exe` keeps running in the tray once the game starts (**Apply & Play** and **Play** start the game for you). Press `Insert` in the game for the menu, or double-click the tray icon. The pill at the top of the menu says **LIVE** while you drive; effects pause while the menu is in front, so click the game to drive.

| Key | Action |
|---|---|
| `Insert` | Show / hide the menu |
| `End` | Everything off |
| `F6` / `F7` / `F8` | Boost on/off, less, more |
| `9` (hold) | Nitrous |
| `Home` | Burnout (stopped) |
| `F5` / `F9` / `F11` | Standing / rolling / wheelie launch |
| `Delete` | More grip on/off (drift and steering lock have no key by default) |
| `F3` | Speed limiter |
| `F10` | Cruise control |
| `F12` | Wheelie mode |
| `F4` | Jump |
| `1` / `2` / `3` | Barrel roll / 360 spin / backflip |
| `4` (or LS click) / `5` | Rocket / instant stop |
| `Backspace` | Flip the car upright |
| `6` | Air control |
| `Page Up` / `Page Down` | Low gravity / UFO mode (`R` / `F` or RB / LB up and down) |
| `8` / `7` | Save a spot / teleport to it |
| `0` | Ghost mode |
| `U` | Switch mph / km/h |
| `F2` | Next game mode: off, speed bomb, score attack, pursuit |

Every key can be changed on the **Keys** tab (telemetry recording has no key by default). They only act while the game window is in front, and never with Ctrl or Alt held, so the Ctrl world keys above keep working.

- Effects change only your own car, only while the overlay runs, and are put back when you pause, open the map or a menu, change car, press `End`, or close the overlay.
- The **MPH / KM/H** switch in the menu header (or `U`) changes the gauges, every speed setting and the run timers (0-60 to 0-200 mph and 1/8 to 1 mile, or 0-100 to 0-400 km/h and 100 to 2000 m; best times are kept for both).
- Stunts and the rocket need the car moving: a parked car is held by the game's brakes.
- **Steering lock** (Driving tab, Handling) sets how far the front wheels turn: most cars turn 42° stock, try 60-70° for tight turns and drifting. The lock still narrows at speed by the same share.
- **Gauges tab:** switch on the gas / brake graph, G-force meter and power graph (all off by default); **Move gauges** lets you drag and resize them. The power graph shows the engine fitted in your save (after an engine swap, once the game has saved) with stock engine parts; its hp and Nm are the game's own figures.
- **Telemetry** (Gauges tab): **Record** writes `logs\telemetry-<date>.csv` in the FH2 Mods folder while you drive; **Open the logs folder** shows them.
- The cockpit steering wheel turns at most 90° each way: the game draws it from a fixed animation, so this can't be raised.
- The game must run in **windowed or borderless** mode (Azimuth's display setting) for the menu and gauges to show over it.
- Single-player only: do not use it in online sessions with other players.

### Game modes

Press `F2` while driving in free roam to go to the next mode (off, speed bomb, score attack, pursuit), or start one from the **Modes** tab. The mode, its score and its warnings show at the top of the status card. `End` (everything off) stops it. Best results are saved.

> **The game modes may be buggy, pursuit most of all.** If something goes wrong, press `End` or `F2` until the mode is off.

| Mode | How it works |
|---|---|
| **Speed bomb** | Reach the bomb speed (50 mph by default) to arm it, then never drop below it for more than half a second, or the car is blown into the air. It gets 5 mph faster every minute. Best: the longest time survived |
| **Score attack** | A timed run (2 minutes by default). Points for speed over 60 mph, drifts, air time and **near misses** (passing another car within 3 m). Each new skill raises the multiplier up to x5, and the chain banks after 3 s without one. **A crash loses the chain.** Best: the highest score |
| **Pursuit** | Drivatars (traffic if there are none) are dropped on the road behind you and chase you along the roads you drove, a little faster than you, ramming when close. **Busted**: stopped next to one for 2.5 s. **Wrecked**: hit 3 times. **Escape**: stay 250 m away for 8 s; then they come back faster, and more of them (up to 3). Ghosting is switched off during a pursuit, also the Ghost cars option. Best: the longest time |

Pursuit notes (experimental):

- Drivatars have no physics while the game drives them, so the overlay holds a chaser on the road you drove. Off your route (cutting a corner through a field) it can float or sink a little. **Chaser height** on the Modes tab trims it: raise it if chasers sink, lower it if they float.
- A chaser stuck against something is put back on your route behind you.
- Hits only count when the chasers actually collide with you; if they pass through, you can still be busted by stopping.

## Requirements

- Windows 10 or 11 (PowerShell 5.1 and .NET Framework 4.8 are built in)
- Forza Horizon 2 for Xbox One (1.0.0.64) on **Azimuth 1.0.4** (1.0.1 to 1.0.3 work too). On an older Azimuth everything works except ghost cars, ExtraOptions' keys, world keys and sky road; update Azimuth from [itch.io](https://almost-there-games.itch.io/azimuth) (your saves carry over)
- **Python 3** on PATH, for the car list, car mods, packs and the power graph ([python.org](https://www.python.org/downloads/): tick **"Add python.exe to PATH"** when installing)
- **[ExtraOptions](https://github.com/RokkuDayo/ForzaHorizon2_ExtraOptions) is included.** Apply installs ExtraOptions 0.4.4 with this tool's additions (ghost cars, world keys, random car, sky road, traffic cars in the Autoshow) as `winmm.dll` next to `azimuth.exe`, with its settings in `FH2_ExtraOptions.ini` beside it, and its two plugin mods (garage revving, time and weather control) in the `mods` folder; Restore originals puts back the one you had. You also get everything ExtraOptions 0.3 and 0.4 added: its `mods` folder loader, Jump to Manufacturer, all online races offline, custom showcases, F1 to hide the HUD, photo mode from the garage, more audio and HUD options, time and weather controls in the pause menu (Y), a time slider in photo mode, headlights that light the road in daytime, a vignette toggle and garage revving in the outposts too. Its own settings are in the in-game menus.

## Setup

1. Download the zip from [Releases](../../releases/latest) and extract it. You get a folder called **FH2 Mods**.
2. Put that folder **next to `azimuth.exe`**:
   ```
   <Azimuth folder>\azimuth.exe
   <Azimuth folder>\FH2 Mods\FH2 Mods.exe
   ```
   The game itself can be anywhere (any folder or drive): the tool finds it through Azimuth's settings, so add the game in Azimuth and start it once first.
3. **Close the game**, then double-click **FH2 Mods.exe**.
4. Change what you want on the **Setup** and **Cars** tabs, then press **Apply**, or **Apply & Play** to also start the game.

**Updating from 1.x:** extract the new **FH2 Mods** folder over the old one. Your `settings.json`, `cars.json` and `overlay.ini` carry over (the new options start switched off). Start `FH2 Mods.exe` (it removes the old `FH2 Mods.bat`, `FH2 Mods.ps1` and `FH2 Mods Overlay.exe`), then press Apply.

## Tips

- Raise grip and brakes along with power, or the car will mostly spin its wheels.
- Grip and brakes act on the car's best road tires and best brakes, so fit those upgrades in game. Turbo only does something on a turbocharged engine.
- Gearing (higher = taller gears, more top speed, slower pull) and rev limit change the fitted transmission and engine, so every car sharing that swap part gets them too.
- Rev limit: x2 always, more only while the engine stays under the **rev ceiling** (10,500 rpm, the game's own highest). A diesel that stops at 2,200 rpm can take x4. Raising the ceiling can crash the game when the car loads: lower it again if it does.
- For top speed runs, keep grip and downforce low rather than relying on the sky road.
- Keep your total CR under about 2,000,000,000: the game may store it as a 32-bit number.
- Ghost cars applies to race opponents too, not only free roam traffic.
- Big solid scenery (trees, light poles, walls, bridges) stays solid; only breakable objects are made light.

## Problems? (Diagnose)

Double-click **Diagnose.bat** in the FH2 Mods folder and send the `diagnose.txt` it writes. It changes nothing. It shows:

- your game files and the tool's backup against the genuine game files;
- the FH2 Mods version, your settings and car mods;
- every car in your garage checked against the game database (a part that belongs to another engine crashes the game when that car loads);
- the ExtraOptions log, the `mods` folder, and the last sessions and crashes from Azimuth's log.

Common errors:

- **`pattern not found in ...`**: that game file was already changed by something else (another tool, a hand edit, or an Apply from another Azimuth folder) before this tool made its backup. Put the original file back from your game download, delete the same file from `backup_original\media` next to azimuth.exe, then Apply again.
- **`game not found`**: put the FH2 Mods folder next to azimuth.exe, add Forza Horizon 2 (the `Anthem_1.0.0.64` folder) in Azimuth and start it once. FH2 Presents Fast & Furious (`Spire`) is not supported.
- **`backup_original ... was made from another game`**: an older version (up to v1.0.6) changed FH2 Presents Fast & Furious instead of FH2. Press Restore originals in that older version, delete `backup_original` next to azimuth.exe, then Apply with this version.
- **Empty car list** on the Cars tab: install Python 3 and tick "Add python.exe to PATH".
- **The game crashes when a car loads:** run Diagnose. If the garage check names that car, press Apply once (with the game closed) and try again.
- **Never delete `backup_original` after an Apply**: the next Apply would back up files that are already modded, and Restore could no longer undo them. If you did, put the genuine game files back from your game download first, then Apply.

## Added cars (mediaoverride / mediapc)

Car mods that add cars through ExtraOptions' `Mount\mediaoverride` or `Mount\mediapc` folder (for example the Rolls-Royce Wraith) ship their own `zipmanifest.xml` and game database. Apply keeps that `zipmanifest.xml` in step with this tool's zips and writes car mods into that database, so the added car stays, shows up in the car list and can be modded too. Install or update such a mod with the game closed, then press Apply again.

- **FM5 Car Importer:** cars it adds after an Apply are kept, and car mods are not applied twice. Press Restore originals before using its `Uninstall.ps1`, then Apply again.
- **Moved database:** car mods still work when a car mod moved the game's `gamedbRC.slt` into `mediaoverride` instead of copying it.
- **Swapping car packs:** a `gamedbRC.slt` from another car pack is taken as that pack's own instead of being mixed with the previous one, so you can switch packs back and forth. If your garage has cars the new pack does not have, the car mods result warns you: put the old pack back or sell those cars first.
- **One database:** the game reads only one car database (`mediaoverride` before `mediapc`). Two mods that each ship a full database hide each other's cars, so keep added cars in one folder.
- **Camera FOV mods** that change `Camera.zip` work too: leave a camera's FOV at the game's own value to keep the mod's, or set your own.

## File overrides (optional)

A file placed at `FH2 Mods\files\<zip name>\<file name>` replaces that file inside the game's zip on Apply, for example `FH2 Mods\files\Camera.zip\CameraPhysics.xml`. The tool's own settings are still applied on top.

## Files

| File | Purpose |
|---|---|
| `FH2 Mods.exe` | The app: Setup and Cars with the game closed, then the in-game overlay. Its overlay settings, keys, teleport spots and best times are in `overlay.ini` beside it |
| `apply.ps1` | Writes traffic, crash, world key, money, skills and camera settings into the game files (run by Apply) |
| `car_power.py` | Writes car mods and packs into the game database (run by Apply); also the car list, the power graph's engine and Diagnose's garage check |
| `settings.json` | The Setup tab's values |
| `cars.json` | The Cars tab's per-car values |
| `logs\` | Telemetry recordings (made when you first record) |
| `Diagnose.bat`, `diagnose.ps1` | Read-only check when something goes wrong; writes `diagnose.txt` |
| `ExtraOptions\winmm.dll` | ExtraOptions 0.4.4 with ghost cars, world keys, sky road, random car and traffic cars in the Autoshow added |
| `ExtraOptions\mods\` | ExtraOptions' plugin mods (garage revving, time and weather control); Apply copies them to the `mods` folder and keeps their settings files if you changed them |
| `ExtraOptions\FH2_ExtraOptions.ini` | Its default settings |
| `ExtraOptions\source\` | Its source code (GPL-3) |
| `Overlay\` (repository only) | The app's source code (C#) |

## License

The tool is [MIT](LICENSE). `ExtraOptions\` is a modified version of [ForzaHorizon2_ExtraOptions](https://github.com/RokkuDayo/ForzaHorizon2_ExtraOptions) by RokkuDayo and stays under the [GPL-3](ExtraOptions/source/LICENSE); its full source is in `ExtraOptions\source`.
