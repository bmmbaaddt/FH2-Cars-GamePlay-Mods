# FH2 Cars & Gameplay Mods

A settings window for **Forza Horizon 2 (Xbox One)** running on **Azimuth**. Change traffic, crashes, money, skills, the cockpit camera and per-car performance from one window, and get keyboard keys for lights, indicators, time and weather. No file editing: just change values and press Apply.

Since 1.1.0 there is also an **in-game overlay** (press `Insert` while driving): boost, nitrous, launch control, drift, stunts, UFO mode, teleport, cruise control, run timers, a drag light tree and on-screen gauges.

## Two menus that work together

| | **FH2 Mods window** (`FH2 Mods.bat`) | **In-game overlay** (`Insert` in game) |
|---|---|---|
| What it changes | The game files: traffic, crashes, money, skills, camera, per-car mods, packs | Your car while you drive, live |
| When it takes effect | Apply, then start the game | At once, no restart |
| Where it is saved | `settings.json`, `cars.json` | `overlay.ini` |

Use both at the same time: set your car mods and packs in the window, press **Apply & Play**, then fine-tune on the road with the overlay. They never overwrite each other. The overlay builds on what the window applied: boost and nitrous multiply the car's power *after* its car mods, and the overlay puts your car back to exactly that when you switch an effect off. **Apply & Play** can start the overlay for you (tick it in the window), or start it any time with the **Overlay** button.

**[Download the latest release](../../releases/latest)**

> Not affiliated with Microsoft, Playground Games, Turn 10 or Azimuth. Azimuth's developer does not support modded setups: if something breaks, press **Restore originals** before asking them for help.

## Features

| Section | What you can change |
|---|---|
| Traffic | Free roam density 0–300% (0 = empty roads), traffic on/off in races and road trips, **ghost cars**: drive straight through every other car (they stay visible) |
| Crashes | **Less spinning and flipping** from crashes (reduces it, does not remove it), **slide along walls**, barriers and trees instead of stopping dead, **light breakables**: fences, signs, posts and bins weigh almost nothing, so they barely slow you |
| World keys | Change the time and weather, switch traffic off, repair the car and get back on the road with Ctrl shortcuts in free roam (see below) |
| Money | CR per level-up, CR per km driven in races |
| Skills | Skill points and chain XP multipliers, highest chain multiplier (with the Skill Legend perk you get one above it), seconds allowed between skills, crashes not ending the skill chain |
| Random car | In **My Cars**, click the right stick or press `R` to jump to a random car of your garage; the window has a **Random car from my garage** button too |
| Cockpit camera | Field of view and seat position |
| Car mods | Pick any car (the traffic bus and box truck too) and set power, air drag, downforce, tire grip, brakes, turbo boost, gearing and rev limit (x1 = stock, up to x100; rev limit up to x2, the game crashes above that), and the number of forward gears (up to 8) |
| Packs | For every car at once: **Race+ engine parts** (an extra level of every engine part with the extra torque you choose), **engine swaps** (every engine fits every car), **traffic and AI cars for sale** in the Autoshow, **free cars** (1 CR each). A pack your garage uses can only be switched off after fitting the stock part |
| Overlay | In game: boost, nitrous, pops and bangs, anti-lag, burnout, standing / rolling / wheelie launch, drift, more grip, super brakes, speed limiter, cruise control, barrel roll, 360, backflip, rocket, jump, instant stop, flip upright, air control, low gravity, UFO mode, ghost mode, 9 teleport spots, run timers with best times, drag light tree with reaction times, on-screen speed and rev panel, nitrous strip, compass, g-force, drift angle and score, trip and top speed. **mph or km/h** with one switch |

- **Random car from my garage** picks one of the cars you own (read from your save) and shows it, so you know what to drive next.
- **Restore originals** puts every game file back in one click, including the original ExtraOptions.
- The original game files are backed up on first run, and every Apply rebuilds from them, so settings never stack up.
- Refuses to change anything while the game is running.

## Keyboard shortcuts

**While driving** (always on):

| Key | Action |
|---|---|
| `L` | Headlights on/off |
| `Q` | Left indicator (press again to switch off) |
| `E` | Right indicator (press again to switch off) |
| `H` | Hazard lights (press again to switch off) |

**In My Cars** (the garage car list, from the main menu or the pause menu): click the **right stick** or press **`R`** to jump to a random car of your garage, then press A to drive it.

You can change these keys in `FH2_ExtraOptions.ini` next to `azimuth.exe`, section `[Keyboard]`: use a letter, a digit, F1–F12, or `off`. FH2 has no horn, so there is no horn key.

**On a controller** (from ExtraOptions 0.2.98):

| Button | Action |
|---|---|
| Hold D-pad left / right (1 s) | Left / right indicator (switches off by itself after the turn) |
| Tap D-pad up | Hazard lights |
| Tap right stick | Headlights |

The keyboard and the controller switch the same indicators.

**World keys** (free roam, tick *World keys* in the window):

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

Start it from the window (**Overlay** button, or tick *Start the overlay with Apply & Play*), then press `Insert` in the game. The pill at the top of the menu says **LIVE** while you drive; effects pause while the menu is in front, so click the game to drive.

| Key | Action |
|---|---|
| `Insert` | Show / hide the overlay menu |
| `End` | Everything off |
| `F6` / `F7` / `F8` | Boost on/off, less, more |
| `9` (hold) | Nitrous |
| `Home` | Burnout (stopped) |
| `F5` / `F9` / `F11` | Standing / rolling / wheelie launch |
| `Delete` | More grip on/off (drift has no key by default) |
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

Every key can be changed on the overlay's **Keys** tab. They only act while the game window is in front, and never with Ctrl or Alt held, so the Ctrl world keys above keep working.

- Effects change only your own car, only while the overlay runs, and are put back when you pause, open the map or a menu, change car, press `End`, or close the overlay.
- The **MPH / KM/H** switch in the menu header (or `U`) changes the gauges, every speed setting and the run timers (0-60 to 0-200 mph and 1/8 to 1 mile, or 0-100 to 0-400 km/h and 100 to 2000 m; best times are kept for both).
- Stunts and the rocket need the car moving: a parked car is held by the game's brakes.
- The game must run in **windowed or borderless** mode (Azimuth's display setting) for the menu and gauges to show over it.
- Single-player only: do not use it in online sessions with other players.

## Requirements

- Windows 10 or 11 (PowerShell 5.1 and .NET Framework 4.8, for the overlay, are built in)
- Forza Horizon 2 for Xbox One (1.0.0.64) on **Azimuth 0.9.10** (0.9.9 works too, without ghost cars, keyboard keys and sky road)
- **Python 3** on PATH, for the car list and car mods ([python.org](https://www.python.org/downloads/): tick **"Add python.exe to PATH"** when installing)
- **[ExtraOptions](https://github.com/RokkuDayo/ForzaHorizon2_ExtraOptions) is included.** Apply installs ExtraOptions 0.2.98 with this tool's additions as `winmm.dll` next to `azimuth.exe`, with its settings in `FH2_ExtraOptions.ini` beside it. An older ExtraOptions in `data\bin` is backed up and swapped for Azimuth's own helper; Restore originals puts everything back. You also get everything new in 0.2.98: D-pad indicators, aero tuning without Forza aero, Return to Garage in the pause menu, the new drift camera, stock rims and livery quality in the in-game Video settings. Works with the LivingCockpit loader too.

## Setup

1. Download the zip from [Releases](../../releases/latest) and extract it. You get a folder called **FH2 Mods**.
2. Put that folder **next to `azimuth.exe`**:
   ```
   <Azimuth folder>\azimuth.exe
   <Azimuth folder>\FH2 Mods\FH2 Mods.bat
   ```
   The game itself can be anywhere (any folder or drive): the tool finds it through Azimuth's settings, so add the game in Azimuth and start it once first.
3. **Close the game**, then double-click **FH2 Mods.bat**.
4. Change what you want, then press **Apply**, or **Apply & Play** to also start the game.

Updating from an older version: replace the whole **FH2 Mods** folder (keep your `settings.json` and `cars.json` if you want your values), then press Apply.

## Tips

- Raise grip and brakes along with power, or the car will mostly spin its wheels.
- Grip and brakes act on the car's best road tires and best brakes, so fit those upgrades in game. Turbo only does something on a turbocharged engine.
- Gearing (higher = taller gears, more top speed, slower pull) and rev limit change the fitted transmission and engine, so every car sharing that swap part gets them too.
- For top speed runs, keep grip and downforce low rather than relying on the sky road.
- Keep your total CR under about 2,000,000,000: the game may store it as a 32-bit number.
- Ghost cars applies to race opponents too, not only free roam traffic.
- Big solid scenery (trees, light poles, walls, bridges) stays solid; only breakable objects are made light.

## Using ForzaInfinity?

[ForzaInfinity](https://github.com/anarqz/ForzaInfinity) (online play) is its own ExtraOptions build in `data\bin`, and only one ExtraOptions can run. Apply leaves ForzaInfinity in place and tells you so, and the FH2 Mods `winmm.dll` switches itself off whenever it finds ForzaInfinity (or any other ExtraOptions) in `data\bin`, so the game is never patched twice. With ForzaInfinity installed:

| Works | Off |
|---|---|
| Traffic density, crashes, light breakables, money, skills, cockpit camera, world key scripts | **Ghost cars**, **keyboard keys** (lights, indicators, Ctrl world keys) and **sky road**: they need the FH2 Mods ExtraOptions |
| Car mods, **but** ForzaInfinity then plays **offline**: its server only takes the game's original car database | |

What to do:
- **To play online with ForzaInfinity:** press **Restore originals** first. Its server checks saves, and money, XP or car mods can get a profile flagged or reset.
- **To play offline with every feature:** run ForzaInfinity's `Uninstall.cmd`, then press **Apply** in FH2 Mods.
- Switch back and forth the same way; Restore originals always puts everything back.

## Apply failed? (troubleshooting)

Copy **Diagnose.bat** into the FH2 Mods folder if it isn't there, double-click it, and send the `diagnose.txt` it writes. It changes nothing; it compares your game files and the tool's backup with the genuine game files and shows what is different.

- **`pattern not found in ...`**: that game file was already changed by something else (another tool, a hand edit, or an Apply from another Azimuth folder) before this tool made its backup. Put the original file back from your game download, delete the same file from `backup_original\media` next to azimuth.exe, then Apply again.
- **`game not found`**: put the FH2 Mods folder next to azimuth.exe, add Forza Horizon 2 (the `Anthem_1.0.0.64` folder) in Azimuth and start it once. FH2 Presents Fast & Furious (`Spire`) is not supported.
- **`backup_original ... was made from another game`**: an older version (up to v1.0.6) changed FH2 Presents Fast & Furious instead of FH2. Press Restore originals in that older version, delete `backup_original` next to azimuth.exe, then Apply with this version.
- **Empty car list**: install Python 3 and tick "Add python.exe to PATH".

## Added cars (mediaoverride / mediapc)

Car mods that add cars through ExtraOptions' `Mount\mediaoverride` or `Mount\mediapc` folder (for example the Rolls-Royce Wraith) ship their own `zipmanifest.xml` and game database. Apply keeps that `zipmanifest.xml` in step with this tool's zips and writes car mods into that database, so the added car stays, shows up in the car list and can be modded too. Install or update such a mod with the game closed, then press Apply again.

- **FM5 Car Importer:** cars it adds after an Apply are kept, and car mods are not applied twice. Press Restore originals before using its `Uninstall.ps1`, then Apply again.
- **Moved database:** car mods still work when a car mod moved the game's `gamedbRC.slt` into `mediaoverride` instead of copying it.
- **One database:** the game reads only one car database (`mediaoverride` before `mediapc`). Two mods that each ship a full database hide each other's cars, so keep added cars in one folder.
- **Cockpit FOV mods** that change `Camera.zip` work too: leave Cockpit FOV at 48 to keep the mod's value, or set your own.

## File overrides (optional)

A file placed at `FH2 Mods\files\<zip name>\<file name>` replaces that file inside the game's zip on Apply, for example `FH2 Mods\files\Camera.zip\CameraPhysics.xml`. The tool's own settings are still applied on top.

## Files

| File | Purpose |
|---|---|
| `FH2 Mods.bat` | Starts the window |
| `FH2 Mods.ps1` | The window |
| `apply.ps1` | Writes traffic, crash, world key, money, skills and camera settings into the game files |
| `car_power.py` | Writes car mods and packs into the game database |
| `FH2 Mods Overlay.exe` | The in-game overlay; its settings, keys, teleport spots and best times are in `overlay.ini` beside it |
| `settings.json` | The window's values |
| `cars.json` | Per-car multipliers |
| `Diagnose.bat`, `diagnose.ps1` | Read-only check when Apply fails; writes `diagnose.txt` |
| `ExtraOptions\winmm.dll` | ExtraOptions 0.2.98 with ghost cars, keyboard keys, world keys, sky road and random car added |
| `ExtraOptions\FH2_ExtraOptions.ini` | Its default settings |
| `ExtraOptions\azimuth-tls.stock.dll` | Azimuth's own TLS helper, put back in `data\bin` in place of an older ExtraOptions |
| `ExtraOptions\source\` | Its source code (GPL-3) |

## License

The tool is [MIT](LICENSE). `ExtraOptions\` is a modified version of [ForzaHorizon2_ExtraOptions](https://github.com/RokkuDayo/ForzaHorizon2_ExtraOptions) by RokkuDayo and stays under the [GPL-3](ExtraOptions/source/LICENSE); its full source is in `ExtraOptions\source`.
