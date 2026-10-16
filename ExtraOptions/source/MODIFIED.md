# Modified version

This is a modified copy of [ForzaHorizon2_ExtraOptions](https://github.com/RokkuDayo/ForzaHorizon2_ExtraOptions) by RokkuDayo, changed in 2026 for FH2 Cars & Gameplay Mods. It remains under the GPL-3 (see LICENSE).

Changes:
- `features/vehicle_controls.c`, `[Keyboard]` settings: keyboard keys for headlights, left/right indicators and hazard lights.
- `features/world_keys.c`: Ctrl+key world shortcuts (time, weather, traffic, repair, snap to road) for the KeyboardShortcut components FH2 Mods adds to free roam.
- `features/ghost_traffic.c`, `[Traffic] GhostCars`: every car except the player's has its car-to-car collision disabled.
- `features/steering_wheel.c`, `[Cockpit] SteeringWheelLock`: experimental cockpit wheel angle scaling (default 0 = off; had no visible effect in testing).
- `patch_engine.c`: the photo mode sample density site is left as loaded when Azimuth 0.9.10 has already changed it, instead of refusing all patches.

Build: see BUILD.md (`python build.py <zig> --azimuth --output out/azimuth-tls.dll`, Zig 0.15.2).
