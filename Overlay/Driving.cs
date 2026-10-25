// The driving effects. Tick() runs about 200 times a second on its own thread. Every value it changes in the game is
// remembered with what it replaced, and put back when the effect ends, the car changes, a menu or the map opens, or
// the overlay closes. A value someone else changed in the meantime is left alone (the effect lets go instead).
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

namespace FH2Overlay
{
    sealed class Input
    {
        public HashSet<string> Down = new HashSet<string>(), Pressed = new HashSet<string>();   // bound actions held / pressed this tick
        public float StickX, StickY;   // left stick, -1..1 (keyboard arrows count too)
        public bool Has(string a) => Pressed.Contains(a);
        public bool Held(string a) => Down.Contains(a);
    }

    sealed class Driving
    {
        readonly Game game;
        readonly Settings s;
        public Driving(Game game, Settings s) { this.game = game; this.s = s; Modes = new Modes(s); }
        public readonly Modes Modes;

        // switches the menu and the keys share
        public volatile bool Boost, Drift, Grip, Limiter, Wheelie, Air, LowGravity, Ufo, Ghost, Cruise;
        public volatile string Status = "", Error = "";
        public float Tank = 1, SpeedMs, Rpm, RedlineRpm, Throttle;
        // readouts for the gauges: drift angle and score, g-force (along and across the car), compass, trip
        public float DriftAngle, DriftChain, DriftLast, DriftBest, GLong, GLat, Heading, TripMetres, TopMs, CruiseMs;
        float[] lastVelocity; long driftAt;
        public bool NitrousOn, LaunchActive, BurnoutActive, InCar, Live;
        public int Model;
        public int SelectedSpot;
        public List<float[]> SpotList = new List<float[]>();   // 15 floats: position then orientation rows (x, y, z, w each... only xyz kept)

        Car car;
        long nextFind;
        float? torqueSet;                   // the torque scale we wrote, null = untouched
        float[] brakesSaved; bool brakesHeld;
        long[] tyres; float frontStock, rearStock; float? frontSet, rearSet; bool tyresRefused;
        int backfireStock = -1; bool flamesOwned;
        bool ghostSet;
        float? previousVy; float airVyPrev = float.NaN, airAccel; int airCount;
        float sinceNitrous = 1; bool nitrousLocked;
        bool throttleWas; long liftedAt = -1;

        enum Phase { None, Arming, Holding, Burst }
        Phase launch; bool launchStanding, launchWheelie; long phaseEnd, launchedAt;
        long burnoutEnd;

        static long Now => Environment.TickCount & int.MaxValue;

        public void Tick(Input input, float dt)
        {
            if (Environment.TickCount - nextFind > 250 || car == null || !game.Valid(car))
            {
                var found = game.Player();
                nextFind = Environment.TickCount;
                if (car != null && !car.Same(found)) LetGo();
                car = found;
            }
            InCar = car != null; Model = car?.Model ?? 0;
            Live = InCar && game.Driving();
            if (input.Has("AllOff")) AllOff();
            if (!Live)
            {
                Release(); CancelMoves();
                if (InCar) try { SpeedMs = game.SpeedMs(car); Rpm = game.Rpm(car); } catch { }
                Status = InCar ? "paused" : "";
                return;
            }
            try { Step(input, dt); Error = ""; }
            catch (Exception e) { Error = e.Message; Release(); CancelMoves(); }
        }

        void Step(Input input, float dt)
        {
            SpeedMs = game.SpeedMs(car); Rpm = game.Rpm(car); RedlineRpm = game.RedlineRpm(car);
            Throttle = game.Throttle();
            float brake = game.BrakePedal();
            var notes = new List<string>();

            // switches
            if (input.Has("Boost")) Boost = !Boost;
            if (input.Has("BoostDown")) s.Boost = Math.Max(1.05f, (float)Math.Round(s.Boost - 0.1f, 2));
            if (input.Has("BoostUp")) s.Boost = Math.Min(20, (float)Math.Round(s.Boost + 0.1f, 2));
            if (input.Has("Drift")) Drift = !Drift;
            if (input.Has("Grip")) Grip = !Grip;
            if (input.Has("Limiter")) Limiter = !Limiter;
            if (input.Has("Wheelie")) Wheelie = !Wheelie;
            if (input.Has("AirControl")) Air = !Air;
            if (input.Has("LowGravity")) LowGravity = !LowGravity;
            if (input.Has("Ufo")) Ufo = !Ufo;
            if (input.Has("Ghost")) Ghost = !Ghost;
            if (input.Has("SaveSpot")) SaveSpot();
            if (input.Has("Units")) { s.Kmh = !s.Kmh; s.Save(); }
            if (input.Has("Cruise"))
            {
                if (Cruise) Cruise = false;
                else if (SpeedMs < 3) notes.Add("drive faster for cruise control");
                else { Cruise = true; CruiseMs = SpeedMs; }
            }
            if (input.Has("Flip")) Flip();

            // launch: standing (stopped, brakes held, revs held at the launch rpm) or rolling (speed held), then extra power
            long now = Now;
            float launchRpm = Math.Max(1500, RedlineRpm * s.LaunchRpmPercent / 100);
            foreach (var (key, standing, wheelie) in new[] { ("StandingLaunch", true, false), ("WheelieLaunch", true, true), ("RollingLaunch", false, false) })
            {
                if (!input.Has(key)) continue;
                if (launch != Phase.None) { launch = Phase.None; break; }
                if (standing && SpeedMs > 1.5f) { notes.Add("stop the car for a standing launch"); break; }
                launch = Phase.Arming; launchStanding = standing; launchWheelie = wheelie;
            }
            float? holdMs = null; bool holdBrakes = false;
            if (launch == Phase.Arming)
            {
                if (launchStanding)
                {
                    holdBrakes = true;
                    if (Rpm >= launchRpm - 250) { launch = Phase.Holding; phaseEnd = now + (long)(s.LaunchHoldSeconds * 1000); }
                    else notes.Add($"LAUNCH: floor it, rev to {launchRpm:0}");
                }
                else if (Math.Abs(SpeedMs - s.RollingMph * Motion.MsPerMph) < 0.5f) { launch = Phase.Holding; phaseEnd = now + (long)(s.LaunchHoldSeconds * 1000); }
                else notes.Add($"LAUNCH: reach {s.RollingMph:0} mph");
            }
            if (launch == Phase.Holding)
            {
                if (launchStanding)
                {
                    holdBrakes = true;
                    if (SpeedMs > 2.5f) { launch = Phase.None; notes.Add("launch cancelled: the car moved"); }
                    else if (Rpm > launchRpm) game.SetRpm(car, launchRpm);
                }
                else holdMs = s.RollingMph * Motion.MsPerMph;
                if (launch == Phase.Holding) notes.Add($"LAUNCH in {(phaseEnd - now) / 1000f:0.0}s");
                if (launch == Phase.Holding && now >= phaseEnd) { launch = Phase.Burst; launchedAt = now; phaseEnd = now + (long)(s.LaunchPowerSeconds * 1000); }
            }
            if (launch == Phase.Burst)
            {
                if (now >= phaseEnd) launch = Phase.None;
                else notes.Add($"GO +{s.LaunchPower:0}%");
            }
            LaunchActive = launch != Phase.None;

            // burnout: brakes on and extra power while stopped
            if (input.Has("Burnout"))
            {
                if (burnoutEnd != 0) burnoutEnd = 0;
                else if (SpeedMs > 1.5f) notes.Add("stop the car for a burnout");
                else burnoutEnd = now + (long)(s.BurnoutSeconds * 1000);
            }
            if (burnoutEnd != 0 && now >= burnoutEnd) burnoutEnd = 0;
            BurnoutActive = burnoutEnd != 0;
            if (BurnoutActive) { holdBrakes = true; notes.Add($"BURNOUT {(burnoutEnd - now) / 1000f:0.0}s"); }

            // nitrous: a tank that empties while firing and refills (3x slower) a second after letting go
            bool throttle = Throttle > 0.5f;
            NitrousOn = input.Held("Nitrous") && throttle && !nitrousLocked && Tank > 0 && launch != Phase.Holding && !BurnoutActive;
            if (!input.Held("Nitrous") && Tank >= 0.1f) nitrousLocked = false;
            if (NitrousOn) { if (!s.InfiniteNitrous) Tank = Math.Max(0, Tank - dt / s.NitrousSeconds); sinceNitrous = 0; if (Tank <= 0) nitrousLocked = true; }
            else if (input.Held("Nitrous")) sinceNitrous = 0;
            else if ((sinceNitrous += dt) >= 1) Tank = Math.Min(1, Tank + dt / (s.NitrousSeconds * 3));

            // engine power
            float power = 1;
            if (Boost) { power *= s.Boost; notes.Add($"boost x{s.Boost:0.##}"); }
            if (NitrousOn) { power *= 1 + s.NitrousPower / 100; notes.Add("NITROUS"); }
            if (launch == Phase.Burst) power *= 1 + s.LaunchPower / 100;
            if (BurnoutActive) power = Math.Max(power, 2);
            SetPower(power);
            HoldBrakes(holdBrakes);

            // pops and bangs: with boost or nitrous, and anti-lag for 1.5 s after lifting near the redline
            if (throttle) { throttleWas = true; liftedAt = -1; } else if (throttleWas) { throttleWas = false; liftedAt = now; }
            bool antiLag = s.AntiLag && liftedAt >= 0 && now - liftedAt < 1500 && RedlineRpm > 0 && Rpm >= RedlineRpm * 0.75f;
            Flames(s.Flames && (NitrousOn || (Boost && throttle) || launch == Phase.Burst) || antiLag, now);

            // tyre grip
            float rear = (Drift ? s.DriftRearGrip : 1) * (Grip ? s.GripBoost : 1), front = Grip ? s.GripBoost : 1;
            SetGrip(front, rear);
            if (Drift) notes.Add("drift"); if (Grip) notes.Add("grip");

            // ghost
            if (Ghost != ghostSet) { if (game.SetGhost(car, Ghost)) ghostSet = Ghost; else { Ghost = false; notes.Add("ghost not available here"); } }
            if (Ghost) notes.Add("ghost");

            // motion: velocity first, then spin
            var rows = game.Rows(car, Game.Basis);
            var v0 = game.Vec(car, Game.Velocity); var v = v0;
            if (Ufo)
            {
                float climb = (input.Held("UfoUp") ? 1 : 0) - (input.Held("UfoDown") ? 1 : 0);
                float forward = Throttle - brake;
                if (Math.Abs(input.StickY) > Math.Abs(forward)) forward = input.StickY;
                Motion.Fly(rows, forward, climb, input.StickX, s.UfoMph * Motion.MsPerMph, s.UfoClimbMph * Motion.MsPerMph, 1.5f, out var fv, out var fs);
                game.SetVelocity(car, fv); game.SetSpin(car, fs);
                notes.Add("UFO");
                Status = string.Join("  ·  ", notes); previousVy = null;
                return;
            }
            if (holdMs != null) v = V.Mul(V.Len(v) < 0.1f ? rows[2] : V.Unit(v), holdMs.Value);
            if (Cruise)   // holds the set speed on the ground plane while the throttle is up; the brake cancels it
            {
                if (brake > 0.2f) Cruise = false;
                else
                {
                    float flat = (float)Math.Sqrt(v[0] * v[0] + v[2] * v[2]);
                    if (Throttle > 0.2f) CruiseMs = Math.Max(CruiseMs, flat);
                    else if (flat > 1 && holdMs == null) v = new[] { v[0] * CruiseMs / flat, v[1], v[2] * CruiseMs / flat };
                    notes.Add($"cruise {Units.Speed(CruiseMs, s.Kmh)}");
                }
            }
            if (Limiter) { v = Motion.Limit(v, s.LimiterMph * Motion.MsPerMph); notes.Add($"limit {s.LimiterMph:0}"); }
            if (s.SuperBrakes && brake > 0.5f) v = Motion.Brake(v, rows[2], s.SuperBrakeG * Motion.G * dt);
            if (LowGravity) { v = Motion.SlowFall(v, previousVy, s.GravityPercent); notes.Add("low gravity"); }
            previousVy = LowGravity ? v[1] : (float?)null;
            if (input.Has("Jump")) v = Motion.Jump(v, s.JumpMetres);
            if (input.Has("Rocket")) v = Motion.Rocket(rows, v, s.RocketMph * Motion.MsPerMph);
            float[] spin = null;
            foreach (var (key, stunt) in new[] { ("BarrelRoll", Motion.Stunt.BarrelRoll), ("Spin360", Motion.Stunt.Spin360), ("Backflip", Motion.Stunt.Backflip) })
                if (input.Has(key)) Motion.DoStunt(rows, v, stunt, out v, out spin);
            if (input.Has("Stop")) { v = new float[3]; spin = new float[3]; }
            if (!v.SequenceEqual(v0)) game.SetVelocity(car, v);

            // in the air: falling at about 1 g for a few ticks
            if (!float.IsNaN(airVyPrev) && dt > 0 && dt < 0.1f) airAccel = airAccel * 0.5f + (v[1] - airVyPrev) / dt * 0.5f;
            airVyPrev = v[1];
            airCount = airAccel < -3.5f && airAccel > -15 ? airCount + 1 : 0;
            bool airborne = airCount >= 4;
            if (spin == null)
            {
                bool wheelieLaunch = launch == Phase.Burst && launchWheelie && now - launchedAt < 3500;
                float? pitch = Wheelie ? s.WheelieDegrees : wheelieLaunch ? (now - launchedAt < 2000 ? s.WheelieDegrees : 0) : (float?)null;
                if (pitch != null) spin = Motion.Wheelie(rows, game.Vec(car, Game.Spin), pitch.Value);
                if (Wheelie) notes.Add("wheelie");
                if (Air && airborne && (Math.Abs(input.StickX) > 0.15f || Math.Abs(input.StickY) > 0.15f))
                    spin = Motion.AirControl(rows, spin ?? game.Vec(car, Game.Spin), input.StickY, input.StickX, s.AirRate);   // stick forward dips the nose
            }
            if (Air) notes.Add("air control");
            if (spin != null) game.SetSpin(car, spin);

            if (input.Has("Teleport")) Teleport();
            Readouts(rows, v, dt, now);
            GameModes(input, rows, airborne, now, dt, notes);
            Status = string.Join("  ·  ", notes);
        }

        void SetPower(float power)
        {
            float now = game.Torque(car), want = car.StockTorque * power;
            if (torqueSet == null)
            {
                if (power == 1) return;
                if (!Game.Near(now, car.StockTorque)) { Error = "the engine power was changed by another tool"; return; }
            }
            else if (!Game.Near(now, torqueSet.Value)) { torqueSet = null; return; }   // someone else wrote it: let go
            if (power == 1) { game.SetTorque(car, car.StockTorque); torqueSet = null; return; }
            if (!Game.Near(now, want)) game.SetTorque(car, want);
            torqueSet = want;
        }

        void HoldBrakes(bool hold)
        {
            if (hold)
            {
                if (!brakesHeld) { brakesSaved = game.Brakes(car); brakesHeld = true; }
                game.SetBrakes(car, 1, 1);   // the game sets them from the pedal every frame, so they are written every tick
            }
            else if (brakesHeld)
            {
                var b = game.Brakes(car);
                game.SetBrakes(car, Game.Near(b[0], 1) ? brakesSaved[0] : b[0], Game.Near(b[1], 1) ? brakesSaved[1] : b[1]);
                brakesHeld = false;
            }
        }

        void Flames(bool on, long now)
        {
            if (on && !flamesOwned)
            {
                backfireStock = game.BackfireModeOf(car);
                if (backfireStock < 0) return;   // no exhaust effect on this car or build
                if (backfireStock != 0) game.SetBackfireMode(car, 0);
                flamesOwned = true;
            }
            if (!flamesOwned) return;
            if (on) game.Pop(car, now % 1000 < 400);
            else
            {
                game.Pop(car, false);
                if (backfireStock > 0 && game.BackfireModeOf(car) == 0) game.SetBackfireMode(car, backfireStock);
                flamesOwned = false;
            }
        }

        void SetGrip(float front, float rear)
        {
            bool want = front != 1 || rear != 1;
            if (!want && tyres == null) return;
            if (tyres == null)
            {
                if (tyresRefused) return;
                try { tyres = game.TyresOf(car); frontStock = game.Grip(car, tyres[0]); rearStock = game.Grip(car, tyres[1]); }
                catch (Exception e) { tyresRefused = true; Drift = Grip = false; Error = "grip not available for this car: " + e.Message; tyres = null; return; }
            }
            float fNow = game.Grip(car, tyres[0]), rNow = game.Grip(car, tyres[1]);
            bool Ours(float x, float stock, float? set) => Game.Near(x, stock) || (set != null && Game.Near(x, set.Value));
            if (!Ours(fNow, frontStock, frontSet) || !Ours(rNow, rearStock, rearSet)) { tyres = null; frontSet = rearSet = null; Drift = Grip = false; return; }
            float fWant = V.Clamp(frontStock * front, 0.05f, 40), rWant = V.Clamp(rearStock * rear, 0.05f, 40);
            if (!Game.Near(fNow, fWant)) game.SetGrip(car, tyres[0], fWant);
            if (!Game.Near(rNow, rWant)) game.SetGrip(car, tyres[1], rWant);
            frontSet = fWant; rearSet = rWant;
            if (!want) { tyres = null; frontSet = rearSet = null; }
        }

        void SaveSpot()
        {
            var p = game.Vec(car, Game.Position); var b = game.BasisBytes(car);
            var spot = new float[15];
            Array.Copy(p, spot, 3);
            for (int r = 0; r < 3; r++) for (int i = 0; i < 3; i++) spot[3 + r * 4 + i] = BitConverter.ToSingle(b, r * 16 + i * 4);
            if (SpotList.Count >= 9) SpotList.RemoveAt(0);
            SpotList.Add(spot); SelectedSpot = SpotList.Count - 1;
            s.Spots = string.Join(";", SpotList.Select(x => string.Join(",", x.Select(f => f.ToString("R", CultureInfo.InvariantCulture)))));
            s.Save();
        }
        public void LoadSpots()
        {
            SpotList = s.Spots.Split(new[] { ';' }, StringSplitOptions.RemoveEmptyEntries)
                .Select(x => x.Split(',').Select(f => float.Parse(f, CultureInfo.InvariantCulture)).ToArray()).Where(x => x.Length == 15).ToList();
        }
        public void Teleport()
        {
            if (SelectedSpot < 0 || SelectedSpot >= SpotList.Count) { Error = "no saved spot: save one first"; return; }
            var spot = SpotList[SelectedSpot];
            var b = new byte[48];
            for (int r = 0; r < 3; r++) for (int i = 0; i < 4; i++) Buffer.BlockCopy(BitConverter.GetBytes(i < 3 ? spot[3 + r * 4 + i] : 0f), 0, b, r * 16 + i * 4, 4);
            game.Teleport(car, new[] { spot[0], spot[1], spot[2] }, b);
        }

        void Readouts(float[][] rows, float[] v, float dt, long now)
        {
            var fwd = V.Unit(new[] { rows[2][0], 0, rows[2][2] });
            Heading = (float)((Math.Atan2(fwd[0], fwd[2]) * 180 / Math.PI + 360) % 360);
            float flat = (float)Math.Sqrt(v[0] * v[0] + v[2] * v[2]);
            DriftAngle = flat > 5 ? (float)(Math.Acos(V.Clamp(V.Dot(fwd, V.Unit(new[] { v[0], 0, v[2] })), -1, 1)) * 180 / Math.PI) : 0;
            if (DriftAngle > 12 && DriftAngle < 100 && flat > 8) { DriftChain += DriftAngle * flat * dt * 0.2f; driftAt = now; }
            else if (DriftChain > 0 && now - driftAt > 2000) { DriftLast = DriftChain; DriftBest = Math.Max(DriftBest, DriftChain); DriftChain = 0; }
            if (lastVelocity != null && dt > 0.001f)
            {
                var a = V.Mul(V.Sub(v, lastVelocity), 1 / (dt * Motion.G));
                var right = V.Unit(new[] { rows[0][0], 0, rows[0][2] });
                GLong = GLong * 0.9f + V.Dot(a, fwd) * 0.1f; GLat = GLat * 0.9f + V.Dot(a, right) * 0.1f;
            }
            lastVelocity = v;
            TripMetres += flat * dt; TopMs = Math.Max(TopMs, flat);
        }

        // ---- game modes: the rules are in Modes; here the game side
        List<(long id, float[] pos, float[] vel)> others = new List<(long, float[], float[])>();
        long othersAt, speedAt, hitAt, spawnAfter; float speedWas = -1; float[] velWas; int escapesWas;
        readonly Modes.Trail trail = new Modes.Trail();
        sealed class Chaser { public long Id; public int CollisionWas; public float Stuck; public bool Drivatar; }
        readonly List<Chaser> chasers = new List<Chaser>();
        int playerCollisionWas = -1;   // the player's collision flag before the pursuit (4 when the Ghost cars option is on)

        void GameModes(Input input, float[][] rows, bool airborne, long now, float dt, List<string> notes)
        {
            if (input.Has("GameMode")) Modes.Next();
            foreach (var (key, kind) in new[] { ("ModeBomb", Modes.Kind.Bomb), ("ModeScore", Modes.Kind.Score), ("ModePursuit", Modes.Kind.Pursuit) })
                if (input.Has(key)) Modes.Start(kind);
            if (input.Has("ModeStop")) Modes.Stop();
            if (Modes.Mode != Modes.Kind.Pursuit) EndChase();
            if (Modes.Mode == Modes.Kind.Off) { speedWas = -1; velWas = null; escapesWas = 0; return; }

            // a crash: 6 m/s (13 mph) lost within a tenth of a second, far more than any brake
            var pos = game.Vec(car, Game.Position); var vel = game.Vec(car, Game.Velocity);
            bool crashed = false, jolted = false;
            if (now - speedAt >= 100)
            {
                crashed = speedWas >= 0 && speedWas - SpeedMs > 6;
                jolted = velWas != null && V.Len(V.Sub(vel, velWas)) > 4;   // any hard knock, a push from behind too
                speedWas = SpeedMs; velWas = vel; speedAt = now;
            }
            int misses = 0;
            if (Modes.Mode != Modes.Kind.Bomb && now - othersAt >= 50)
            {
                othersAt = now; others = game.Others(car);
                if (Modes.Mode == Modes.Kind.Score) misses = Modes.CountNearMisses(others, pos, vel, crashed);
            }
            float nearest = float.NaN; bool rammed = false;
            if (Modes.Mode == Modes.Kind.Pursuit)
            {
                trail.Add(pos);
                nearest = Chase(pos, vel, dt, now, notes);
                // a hit: knocked hard with a chaser within 7 m, once per 1.5 s
                if (jolted && nearest < 7 && now - hitAt > 1500) { rammed = true; hitAt = now; }
            }
            Modes.Step(SpeedMs, DriftAngle, airborne, crashed, misses, nearest, dt, rammed);
            if (Modes.Escapes != escapesWas) { escapesWas = Modes.Escapes; ReleaseChasers(); spawnAfter = now + 5000; }   // escaped: fresh chasers after a 5 s break
            if (Modes.Explode)   // the bomb: thrown up and rolled
            {
                Modes.Explode = false;
                Motion.DoStunt(rows, Motion.Jump(game.Vec(car, Game.Velocity), 6), Motion.Stunt.BarrelRoll, out var bv, out var bs);
                game.SetVelocity(car, bv); game.SetSpin(car, bs);
            }
        }

        // the chasers: nearby cars taken over, up to Modes.Chasers; returns the nearest one's distance
        float Chase(float[] pos, float[] vel, float dt, long now, List<string> notes)
        {
            // a pursuit needs contact: ghosting is off for you and the chasers (also the FH2 Mods Ghost cars option's)
            if (Ghost) { Ghost = false; notes.Add("ghost is off during a pursuit"); }
            if (playerCollisionWas < 0) playerCollisionWas = game.CollisionOf(car.Sim);
            game.SetCollision(car, car.Sim, true);

            foreach (var c in chasers.Where(c => !game.OtherValid(c.Id)).ToList()) chasers.Remove(c);   // despawned (drivatars may be off the road list, so only the body counts)
            if (now >= spawnAfter && chasers.Count < Modes.Chasers)
            {
                // drivatars first (also ones the game parked far away), nearest first; traffic only when there are none
                var drivatars = game.Drivatars(car);
                var pool = drivatars.OrderBy(o => V.Len(V.Sub(o.pos, pos))).Select(o => o.id)
                               .Concat(others.OrderBy(o => V.Len(V.Sub(o.pos, pos))).Select(o => o.id));
                foreach (var id in pool.Where(id => chasers.All(c => c.Id != id)).Distinct().ToList())
                {
                    if (chasers.Count >= Modes.Chasers) break;
                    var c = new Chaser { Id = id, CollisionWas = game.CollisionOf(id), Drivatar = drivatars.Any(d => d.id == id) };
                    if (!PlaceBehind(c, pos, vel, 40 + 30 * chasers.Count)) break;   // nowhere to put it yet
                    chasers.Add(c);
                }
            }
            if (chasers.Count == 0) return float.NaN;

            float yours = (float)Math.Sqrt(vel[0] * vel[0] + vel[2] * vel[2]);
            float top = s.ChaserMph * Motion.MsPerMph * Modes.ChaserBoost, nearest = float.MaxValue;
            foreach (var c in chasers.ToList())
                try
                {
                    var cp = game.OtherVec(c.Id, Game.Position); var cv = game.OtherVec(c.Id, Game.Velocity);
                    float d = V.Len(V.Sub(pos, cp)), cs = (float)Math.Sqrt(cv[0] * cv[0] + cv[2] * cv[2]);
                    // stuck (against a wall, in a ditch) for 2.5 s: put back on your trail, about 120 m behind you;
                    // one stuck 250 m or more away is left there: you shook it off, and the escape counts
                    c.Stuck = cs < 2 && d > 20 && d < 250 ? c.Stuck + dt : 0;
                    if (c.Stuck > 2.5f && PlaceBehind(c, pos, vel, 60)) { c.Stuck = 0; continue; }
                    // close: ram speed; far: catch up fast (rubber band), never above its top speed
                    float speed = Math.Min(top, d < 45 ? yours + 6 : yours * 1.3f + 15);
                    var (nv, spin) = Modes.Steer(cp, cv, game.OtherRows(c.Id), Modes.Aim(trail, cp, pos, vel), speed, 12, dt, trail.UpAt(cp));
                    if (c.Drivatar)   // no physics: held on the road your trail went over, tilted with it, no vertical speed of its own
                    {
                        float y = trail.HeightAt(cp);
                        game.SetOtherHeight(car, c.Id, (float.IsNaN(y) ? pos[1] : y) + s.ChaserHeight);
                        nv[1] = 0;
                    }
                    game.PushOther(car, c.Id, nv, spin);
                    game.SetCollision(car, c.Id, true);
                    nearest = Math.Min(nearest, d);
                }
                catch { Release(c); }
            if (chasers.Count > 1) notes.Add($"{chasers.Count} chasers");
            return nearest == float.MaxValue ? float.NaN : nearest;
        }

        // puts a chaser on your trail `metres` behind you (straight behind you before there is a trail), already near your speed
        bool PlaceBehind(Chaser c, float[] pos, float[] vel, float metres)
        {
            float[] at, dir;
            var back = trail.Behind(metres);
            if (back != null) { at = back.Value.pos; dir = back.Value.dir; }
            else
            {
                var r = game.Rows(car, Game.Basis); dir = V.Unit(new[] { r[2][0], 0, r[2][2] });
                if (V.Len(dir) < 0.5f) return false;
                at = V.Sub(pos, V.Mul(dir, metres));
            }
            if (V.Len(V.Sub(at, pos)) < 20) return false;   // the trail doubles back: too close to drop a car
            var up = new float[] { 0, 1, 0 }; var right = V.Cross(up, dir);
            var b = new byte[48];
            foreach (var (row, o) in new[] { (right, 0), (up, 16), (dir, 32) })
                for (int i = 0; i < 3; i++) Buffer.BlockCopy(BitConverter.GetBytes(row[i]), 0, b, o + i * 4, 4);
            game.TeleportOther(car, c.Id, at, b);
            game.PushOther(car, c.Id, V.Mul(dir, 0.9f * (float)Math.Sqrt(vel[0] * vel[0] + vel[2] * vel[2])), new float[3]);
            return true;
        }

        void Release(Chaser c)
        {
            chasers.Remove(c);
            try { if (c.CollisionWas == 4 && game.OtherValid(c.Id) && game.CollisionOf(c.Id) == 2) game.SetCollision(car, c.Id, false); } catch { }
        }
        void ReleaseChasers() { foreach (var c in chasers.ToList()) Release(c); }

        // the pursuit is over: chasers go back to the game, your collision flag back to what it was
        void EndChase()
        {
            ReleaseChasers(); trail.Points.Clear();
            if (playerCollisionWas < 0) return;
            try { if (playerCollisionWas == 4 && car != null && game.Valid(car) && game.CollisionOf(car.Sim) == 2) game.SetCollision(car, car.Sim, false); } catch { }
            playerCollisionWas = -1;
        }

        // back on the wheels, facing the same way, a little above where it was
        void Flip()
        {
            var rows = game.Rows(car, Game.Basis);
            var fwd = V.Unit(new[] { rows[2][0], 0, rows[2][2] });
            if (V.Len(fwd) < 0.5f) fwd = V.Unit(V.Cross(rows[0], new float[] { 0, 1, 0 }));
            var up = new float[] { 0, 1, 0 }; var right = V.Cross(up, fwd);
            var b = new byte[48];
            foreach (var (row, at) in new[] { (right, 0), (up, 16), (fwd, 32) })
                for (int i = 0; i < 3; i++) Buffer.BlockCopy(BitConverter.GetBytes(row[i]), 0, b, at + i * 4, 4);
            var p = game.Vec(car, Game.Position);
            game.Teleport(car, new[] { p[0], p[1] + 1, p[2] }, b);
        }

        void CancelMoves() { launch = Phase.None; burnoutEnd = 0; previousVy = null; airVyPrev = float.NaN; airCount = 0; NitrousOn = false; }

        // puts back what this car still holds from us; used for pauses (switches stay as they are)
        void Release()
        {
            if (car == null || !game.Valid(car)) { Forget(); return; }
            try { SetPower(1); } catch { }
            try { HoldBrakes(false); } catch { }
            try { Flames(false, 0); } catch { }
            try { if (tyres != null) SetGrip(1, 1); } catch { }
            Forget();
        }
        void Forget() { torqueSet = null; brakesHeld = false; flamesOwned = false; tyres = null; frontSet = rearSet = null; }

        // the car is gone or changed: also its ghost flag, and the new car gets a fresh grip check
        void LetGo()
        {
            EndChase(); Modes.Stop();
            Release();
            try { if (ghostSet && car != null && game.Valid(car)) game.SetGhost(car, false); } catch { }
            ghostSet = false; tyresRefused = false;
        }

        public void AllOff()
        {
            Modes.Stop(); EndChase();
            Boost = Drift = Grip = Limiter = Wheelie = Air = LowGravity = Ufo = Ghost = Cruise = false;
            CancelMoves();
            try { if (ghostSet && car != null && game.Valid(car)) game.SetGhost(car, false); } catch { }
            ghostSet = false;
            Release();
        }

        public void Shutdown() { AllOff(); }
    }
}
