# FH2 Cars & Gameplay Mods

A settings window for **Forza Horizon 2 (Xbox One)** running on **Azimuth**. Change traffic, crashes, money, skills, the cockpit camera and per-car performance from one window, and get keyboard keys for lights, indicators, time and weather. No file editing: just change values and press Apply.

**[Download the latest release](../../releases/latest)**

> Not affiliated with Microsoft, Playground Games, Turn 10 or Azimuth. Azimuth's developer does not support modded setups: if something breaks, press **Restore originals** before asking them for help.

## Features

| Section | What you can change |
|---|---|
| Traffic | Free roam density 0–300% (0 = empty roads), traffic on/off in races and road trips, **ghost cars**: drive straight through every other car (they stay visible) |
| Crashes | **Less spinning and flipping** from crashes (reduces it, does not remove it), **slide along walls**, barriers and trees instead of stopping dead, **light breakables**: fences, signs, posts and bins weigh almost nothing, so they barely slow you |
| World keys | Change the time and weather, switch traffic off, repair the car and get back on the road with Ctrl shortcuts in free roam (see below) |
| Money | CR per level-up, CR per km driven in races |
| Skills | Skill points and chain XP multipliers, highest chain multiplier, seconds allowed between skills, crashes not ending the skill chain |
| Cockpit camera | Field of view and seat position |
| Car mods | Pick any car and set power, air drag, downforce, tire grip, brakes, turbo boost, gearing and rev limit (x1 = stock, up to x100) |

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

You can change these keys in `data\bin\FH2_ExtraOptions.ini`, section `[Keyboard]`: use a letter, a digit, F1–F12, or `off`. FH2 has no horn, so there is no horn key.

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

## Requirements

- Windows 10 or 11 (PowerShell 5.1 is built in)
- Forza Horizon 2 for Xbox One (1.0.0.64) on **Azimuth 0.9.9 or 0.9.10**
- **Python 3** on PATH, for the car list and car mods ([python.org](https://www.python.org/downloads/): tick **"Add python.exe to PATH"** when installing)
- **[ExtraOptions](https://github.com/RokkuDayo/ForzaHorizon2_ExtraOptions)** installed in `data\bin`, for ghost cars and the keyboard keys. Apply swaps it for the included build that adds these features (your original is backed up and comes back with Restore originals). Works with the LivingCockpit loader too. Everything else works without it.

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

## File overrides (optional)

A file placed at `FH2 Mods\files\<zip name>\<file name>` replaces that file inside the game's zip on Apply, for example `FH2 Mods\files\Camera.zip\CameraPhysics.xml`. The tool's own settings are still applied on top.

## Files

| File | Purpose |
|---|---|
| `FH2 Mods.bat` | Starts the window |
| `FH2 Mods.ps1` | The window |
| `apply.ps1` | Writes traffic, crash, world key, money, skills and camera settings into the game files |
| `car_power.py` | Writes car mods into the game database |
| `settings.json` | The window's values |
| `cars.json` | Per-car multipliers |
| `ExtraOptions\azimuth-tls.dll` | ExtraOptions with ghost cars and keyboard keys added |
| `ExtraOptions\source\` | Its source code (GPL-3) |

## License

The tool is [MIT](LICENSE). `ExtraOptions\` is a modified version of [ForzaHorizon2_ExtraOptions](https://github.com/RokkuDayo/ForzaHorizon2_ExtraOptions) by RokkuDayo and stays under the [GPL-3](ExtraOptions/source/LICENSE); its full source is in `ExtraOptions\source`.
