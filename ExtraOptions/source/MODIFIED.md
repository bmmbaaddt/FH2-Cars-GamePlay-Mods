# Modified version

This is a modified copy of [ForzaHorizon2_ExtraOptions](https://github.com/RokkuDayo/ForzaHorizon2_ExtraOptions) 0.2.98 by RokkuDayo, changed in 2026 for FH2 Cars & Gameplay Mods. It remains under the GPL-3 (see LICENSE).

Changes:
- `features/vehicle_controls.c`, `features/blinkers.c`, `[Keyboard]` settings: keyboard keys for headlights, left/right indicators and hazard lights. The indicator keys switch the same blinker state as the D-pad holds, so the two never fight.
- `features/world_keys.c`: Ctrl+key world shortcuts (time, weather, traffic, repair, snap to road) for the KeyboardShortcut components FH2 Mods adds to free roam.
- `features/ghost_traffic.c`, `[Traffic] GhostCars`: every car except the player's has its car-to-car collision disabled.
- `features/sky_road.c`: Ctrl+F lifts the player's car onto an endless flat floor (the engine's own infinite flat collision plane) 3000 m above the map; Ctrl+F again drops it back to the road. Experimental.
- `adapters/azimuth.c`: stays off (no patches, no message box) when `data\bin\azimuth-tls.dll` or `azimuth-tls.dll.original` is itself an Extra Options build, such as ForzaInfinity, so the game is never patched twice.

Build: `python build.py <zig> --azimuth --output out/winmm.dll` (Zig 0.15.2, see BUILD.md).
