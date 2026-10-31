# Modified version

This is a modified copy of [ForzaHorizon2_ExtraOptions](https://github.com/RokkuDayo/ForzaHorizon2_ExtraOptions) 0.4.4 by RokkuDayo, changed in 2026 for FH2 Cars & Gameplay Mods. It remains under the GPL-3 (see LICENSE).

Changes:
- `features/world_keys.c`: Ctrl+key world shortcuts (time, weather, traffic, repair, snap to road) for the KeyboardShortcut components FH2 Mods adds to free roam.
- `features/random_car.c`: "Random car" on the garage car list (My Cars): a right stick click or R moves the list to a random car of the garage; the button bar shows it on the right stick. It runs from Jump to Manufacturer's garage hooks (`features/manufacturer_jump.c`), which own that page's slots, and stays off while the manufacturer picker is open.
- `features/ghost_traffic.c`, `[Traffic] GhostCars`: every car except the player's has its car-to-car collision disabled.
- `features/sky_road.c`: Ctrl+F lifts the player's car onto an endless flat floor (the engine's own infinite flat collision plane) 3000 m above the map; Ctrl+F again drops it back to the road. Experimental.

- `adapters/azimuth.c`, `build.py`: build fixes for the published 0.4.4 source, which removed the unfinished FM5 files but still included `features/fm5.h`: the include is behind `FH2_ENABLE_FM5` like its only use, and the build's include scan skips headers that are not there.

The keyboard keys for headlights, indicators and hazards from the 0.2.98-based version are gone: 0.3.x has its own `[Bindings]`.

Build: `python build.py <zig> --azimuth --output out/winmm.dll` (Zig 0.15.2, see BUILD.md).
