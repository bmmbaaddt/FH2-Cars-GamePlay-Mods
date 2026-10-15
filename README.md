# FH2 Cars & Gameplay Mods

A settings window for **Forza Horizon 2 (Xbox One)** running on **Azimuth**. Change traffic, money, skills, the cockpit camera and per-car performance from one window: no file editing, just change values and press Apply.

**[Download the latest release](../../releases/latest)**

> Not affiliated with Microsoft, Playground Games, Turn 10 or Azimuth. Azimuth's developer does not support modded setups: if something breaks, press **Restore originals** before asking them for help.

## Features

| Section | What you can change |
|---|---|
| Traffic | Free roam density 0–300% (0 = empty roads), traffic on/off in races and road trips |
| Money | CR per level-up, CR per km driven in races |
| Skills | Skill points and chain XP multipliers, highest chain multiplier, seconds allowed between skills, crashes not ending the skill chain |
| Cockpit camera | Field of view and seat position |
| Car mods | Pick any car and set power, air drag, downforce, tire grip, brakes and turbo boost (x1 = stock) |

- **Restore originals** puts every game file back in one click.
- The original game files are backed up on first run, and every Apply rebuilds from them, so settings never stack up.
- Refuses to change anything while the game is running.

## Requirements

- Windows 10 or 11 (PowerShell 5.1 is built in)
- Forza Horizon 2 for Xbox One (1.0.0.64) on **Azimuth 0.9.9**
- **Python 3** on PATH, for the car list and car mods ([python.org](https://www.python.org/downloads/): tick **"Add python.exe to PATH"** when installing)

## Setup

1. Download the zip from [Releases](../../releases/latest) and extract it. You get a folder called **FH2 Mods**.
2. Put that folder **next to `azimuth.exe`**:
   ```
   <Azimuth folder>\azimuth.exe
   <Azimuth folder>\FH2 Mods\FH2 Mods.bat
   <Azimuth folder>\<game folder>\Mount\media\...
   ```
3. **Close the game**, then double-click **FH2 Mods.bat**.
4. Change what you want, then press **Apply**, or **Apply & Play** to also start the game.

## Tips

- Raise grip and brakes along with power, or the car will mostly spin its wheels.
- Grip and brakes act on the car's best road tires and best brakes, so fit those upgrades in game. Turbo only does something on a turbocharged engine.
- Keep your total CR under about 2,000,000,000: the game may store it as a 32-bit number.

## File overrides (optional)

A file placed at `FH2 Mods\files\<zip name>\<file name>` replaces that file inside the game's zip on Apply, for example `FH2 Mods\files\Camera.zip\CameraPhysics.xml`. The tool's own settings are still applied on top.

## Files

| File | Purpose |
|---|---|
| `FH2 Mods.bat` | Starts the window |
| `FH2 Mods.ps1` | The window |
| `apply.ps1` | Writes traffic, money, skills and camera settings into the game's zips |
| `car_power.py` | Writes car mods into the game database |
| `settings.json` | The window's values |
| `cars.json` | Per-car multipliers |

## License

[MIT](LICENSE)
