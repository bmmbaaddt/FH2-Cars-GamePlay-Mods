// Game modes, switchable in game: speed bomb (stay above a speed or blow up), score attack (a timed skill chain:
// speed, drift, air and near misses; a crash loses the chain) and pursuit (a nearby car chases you; stop near it and
// you are busted, get far enough away and you escape). Fed what the car does every tick; knows nothing about the game,
// so Modes.Test() checks the rules without it. Driving does the game side (the explosion, steering the chaser).
using System;
using System.Collections.Generic;

namespace FH2Overlay
{
    sealed class Modes
    {
        public enum Kind { Off, Bomb, Score, Pursuit }
        public static readonly string[] Names = { "off", "speed bomb", "score attack", "pursuit" };
        readonly Settings s;
        public Modes(Settings s) { this.s = s; }

        public Kind Mode;
        public float Clock;                 // seconds since the mode started (bomb: since armed)
        public string Banner = "", Detail = "", Result = "";   // what the card shows; Result stays after the mode ends
        public bool Explode;                // one-shot: the bomb went off (Driving throws the car)

        // speed bomb
        public bool Armed; public float BombMetres, BelowFor;
        public float BombMs => (s.BombMph + (Armed ? 5 * (float)Math.Floor(Clock / 60) : 0)) * Motion.MsPerMph;   // 5 mph faster every minute
        // score attack
        public float Score, Chain, Since = 99; public int Skills, NearMisses;
        public float Multiplier => Math.Min(5, 1 + 0.25f * Skills);
        bool wasAir, wasDrift;
        // pursuit
        public float Bust, Escape, ChaserMetres = float.NaN; public int Escapes, Hits;
        public const int MaxHits = 3;
        public float ChaserBoost => 1 + 0.1f * Escapes;            // each escape: the next chasers are 10% faster
        public int Chasers => Math.Min(3, 1 + (Escapes + 1) / 2);  // and one more joins every second escape (up to 3)

        public void Start(Kind k)
        {
            Mode = k; Clock = 0; Explode = false; Result = "";
            Armed = false; BombMetres = BelowFor = 0;
            Score = Chain = 0; Since = 99; Skills = NearMisses = 0; wasAir = wasDrift = false;
            Bust = Escape = 0; ChaserMetres = float.NaN; Escapes = Hits = 0;
            near.Clear();
            Show();
        }
        public void Next() => Start((Kind)(((int)Mode + 1) % 4));
        public void Stop() { if (Mode != Kind.Off) Finish("stopped: " + Summary()); }

        void Finish(string result)
        {
            Result = result; Mode = Kind.Off; Banner = Detail = "";
            s.Save();
        }

        // speed in m/s; drift angle in degrees; crashed = a big sudden slowdown this tick; chaserMetres = the nearest chaser;
        // rammed = a chaser hit you this tick
        public void Step(float speedMs, float driftAngle, bool airborne, bool crashed, int nearMisses, float chaserMetres, float dt, bool rammed = false)
        {
            if (Mode == Kind.Off) return;
            if (Mode != Kind.Bomb || Armed) Clock += dt;
            float mph = speedMs / Motion.MsPerMph;
            if (Mode == Kind.Bomb)
            {
                if (!Armed) { if (speedMs >= BombMs) { Armed = true; Clock = 0; } }
                else
                {
                    BombMetres += speedMs * dt;
                    BelowFor = speedMs < BombMs ? BelowFor + dt : 0;
                    if (BelowFor > 0.5f)
                    {
                        Explode = true;
                        bool best = Clock > s.BombBest; if (best) s.BombBest = Clock;
                        Finish($"BOOM  survived {Clock:0.0} s, {Units.Distance(BombMetres, s.Kmh)}" + (best ? "  ★ best" : $"  (best {s.BombBest:0.0} s)"));
                        return;
                    }
                }
            }
            else if (Mode == Kind.Score)
            {
                bool drift = driftAngle > 12 && driftAngle < 100 && speedMs > 8;
                float points = Math.Max(0, mph - 60) * 2 * dt + (drift ? driftAngle * speedMs * dt * 0.2f : 0) + (airborne ? 100 * dt : 0) + 250 * nearMisses;
                // each new skill (a drift, a jump, a near miss) raises the multiplier; speed alone only keeps the chain going
                int fresh = nearMisses + (drift && !wasDrift ? 1 : 0) + (wasAir && !airborne ? 1 : 0);
                wasDrift = drift; wasAir = airborne; NearMisses += nearMisses;
                if (nearMisses > 0) Flash("NEAR MISS +" + 250 * nearMisses); else if (wasAir && !airborne) Flash("JUMP");
                if (crashed && Chain > 0) { Chain = 0; Skills = 0; Since = 99; Flash("CRASH: chain lost"); }
                else if (points > 0) { Chain += points; Skills += fresh; Since = 0; }
                else if ((Since += dt) > 3 && Chain > 0) Bank();
                if (Clock >= s.ScoreSeconds)
                {
                    Bank();
                    bool best = Score > s.ScoreBest; if (best) s.ScoreBest = Score;
                    Finish($"SCORE {Score:#,0}  ({NearMisses} near misses)" + (best ? "  ★ best" : $"  (best {s.ScoreBest:#,0})"));
                    return;
                }
            }
            else if (Mode == Kind.Pursuit)
            {
                ChaserMetres = chaserMetres;
                bool close = !float.IsNaN(chaserMetres) && chaserMetres < 12;
                Bust = V.Clamp(Bust + (close && speedMs < 4 ? dt / 2.5f : -dt / 2), 0, 1);       // stopped next to it for 2.5 s
                Escape = float.IsNaN(chaserMetres) ? Escape : V.Clamp(Escape + (chaserMetres > 250 ? dt / 8 : -dt / 4), 0, 1);   // 8 s far away
                if (rammed) Hits++;
                if (Bust >= 1 || Hits >= MaxHits)
                {
                    bool best = Clock > s.PursuitBest; if (best) s.PursuitBest = Clock;
                    Finish((Bust >= 1 ? "BUSTED" : "WRECKED") + $"  after {Clock:0.0} s, {Escapes} escape(s)" + (best ? "  ★ best" : $"  (best {s.PursuitBest:0.0} s)"));
                    return;
                }
                if (Escape >= 1) { Escapes++; Escape = 0; Bust = 0; Hits = 0; Result = $"ESCAPED ({Escapes}): " + (Chasers > 1 ? $"{Chasers} faster chasers next" : "a faster one is coming"); }
            }
            Show();
        }

        void Bank() { Score += Chain * Multiplier; Chain = 0; Skills = 0; }
        string flash = ""; float flashUntil;
        void Flash(string text) { flash = text; flashUntil = Clock + 1.5f; }

        string Summary() => Mode == Kind.Bomb ? $"{Clock:0.0} s" : Mode == Kind.Score ? $"{Score + Chain * Multiplier:#,0} points" : $"{Clock:0.0} s, {Escapes} escape(s)";

        void Show()
        {
            if (Mode == Kind.Bomb)
            {
                Banner = Armed ? $"SPEED BOMB  {Clock:0.0} s" : "SPEED BOMB";
                Detail = Armed ? (BelowFor > 0 ? $"TOO SLOW! keep above {Units.Speed(BombMs, s.Kmh)}" : $"keep above {Units.Speed(BombMs, s.Kmh)}  ·  +5 mph in {60 - Clock % 60:0} s  ·  {Units.Distance(BombMetres, s.Kmh)}")
                               : $"reach {Units.Speed(BombMs, s.Kmh)} to arm it";
            }
            else if (Mode == Kind.Score)
            {
                Banner = $"SCORE {Score:#,0}   {Math.Max(0, s.ScoreSeconds - Clock):0} s";
                Detail = (Clock < flashUntil ? flash + "   " : "") + (Chain > 0 ? $"chain {Chain:#,0}  x{Multiplier:0.##}" : "speed, drifts, jumps, near misses");
            }
            else if (Mode == Kind.Pursuit)
            {
                Banner = $"PURSUIT  {Clock:0.0} s" + (Escapes > 0 ? $"  ·  {Escapes} escaped" : "") + "  " + new string('■', MaxHits - Hits) + new string('□', Hits);
                Detail = float.IsNaN(ChaserMetres) ? "looking for a car to chase you..." :
                         Bust > 0.05f ? $"BUSTED {Bust * 100:0}%  keep moving!" :
                         Escape > 0.05f ? $"escaping {Escape * 100:0}%  ({ChaserMetres:0} m)" : $"nearest chaser {ChaserMetres:0} m  ·  {MaxHits - Hits} hit(s) left";
            }
        }

        // ---- near misses: another car came within 3 m at 8 m/s or more closing speed, and went away again without a crash
        readonly Dictionary<long, float> near = new Dictionary<long, float>();   // car -> closest distance so far
        public int CountNearMisses(IEnumerable<(long id, float[] pos, float[] vel)> others, float[] pos, float[] vel, bool crashed)
        {
            if (crashed) { near.Clear(); return 0; }
            int count = 0; var seen = new HashSet<long>();
            foreach (var (id, p, v) in others)
            {
                seen.Add(id);
                float d = V.Len(V.Sub(p, pos));
                if (d < 8 && V.Len(V.Sub(v, vel)) >= 8) near[id] = Math.Min(d, near.TryGetValue(id, out float m) ? m : d);
                else if (near.TryGetValue(id, out float closest)) { if (d >= 8 && closest < 3) count++; near.Remove(id); }
            }
            foreach (var id in new List<long>(near.Keys)) if (!seen.Contains(id)) near.Remove(id);
            return count;
        }

        // ---- the chase AI. Far away a chaser follows your trail (where you drove is road, a straight line may be a hill
        // or a house); close by it goes straight for where you will be. It drives along its own nose, never sideways.
        public sealed class Trail
        {
            public readonly List<float[]> Points = new List<float[]>();   // every 4 m you drive, the last 2 km
            public void Add(float[] pos)
            {
                if (Points.Count > 0 && V.Len(V.Sub(pos, Points[Points.Count - 1])) < 4) return;
                if (Points.Count > 0 && V.Len(V.Sub(pos, Points[Points.Count - 1])) > 60) Points.Clear();   // a teleport: the old trail leads nowhere
                Points.Add((float[])pos.Clone()); if (Points.Count > 500) Points.RemoveAt(0);
            }
            // the road height at p: along the trail segment nearest to it (where you drove is road); NaN without a trail
            public float HeightAt(float[] p)
            {
                int i = Nearest(p); if (i < 0) return float.NaN;
                float best = float.MaxValue, y = Points[i][1];
                foreach (int j in new[] { i - 1, i + 1 })
                {
                    if (j < 0 || j >= Points.Count) continue;
                    float[] a = Points[Math.Min(i, j)], b = Points[Math.Max(i, j)];
                    float dx = b[0] - a[0], dz = b[2] - a[2], len = dx * dx + dz * dz;
                    float t = len < 1e-4f ? 0 : V.Clamp(((p[0] - a[0]) * dx + (p[2] - a[2]) * dz) / len, 0, 1);
                    float ex = a[0] + dx * t - p[0], ez = a[2] + dz * t - p[2], d = ex * ex + ez * ez;
                    if (d < best) { best = d; y = a[1] + (b[1] - a[1]) * t; }
                }
                return y;
            }
            // the road's up at p: tilted with the trail's slope there (no side slope); null without a trail
            public float[] UpAt(float[] p)
            {
                int i = Nearest(p); if (i < 0 || Points.Count < 2) return null;
                var d = V.Unit(V.Sub(Points[Math.Min(i + 1, Points.Count - 1)], Points[Math.Max(i - 1, 0)]));
                var right = V.Unit(V.Cross(new float[] { 0, 1, 0 }, d));
                if (V.Len(right) < 0.5f) return null;
                return V.Unit(V.Cross(d, right));
            }
            public int Nearest(float[] p)
            {
                int best = -1; float bd = float.MaxValue;
                for (int i = 0; i < Points.Count; i++)   // on the ground plane: a floating or sunk car still finds the road under it
                {
                    float dx = Points[i][0] - p[0], dz = Points[i][2] - p[2], d = dx * dx + dz * dz;
                    if (d < bd) { bd = d; best = i; }
                }
                return best;
            }
            // a point about `metres` back along the trail from its newest end, and the way the trail runs there; null if too short
            public (float[] pos, float[] dir)? Behind(float metres)
            {
                float run = 0;
                for (int i = Points.Count - 1; i > 0; i--)
                {
                    run += V.Len(V.Sub(Points[i], Points[i - 1]));
                    if (run >= metres) { var dir = V.Sub(Points[i], Points[i - 1]); dir[1] = 0; return (Points[i - 1], V.Unit(dir)); }
                }
                return null;
            }
        }

        // where a chaser aims: your trail about 24 m past the point nearest to it while far, you (half a second ahead) when close
        public static float[] Aim(Trail trail, float[] chaser, float[] you, float[] yourVel)
        {
            float d = V.Len(V.Sub(you, chaser));
            if (d < 45 || trail.Points.Count < 2) return V.Add(you, V.Mul(yourVel, Math.Min(0.6f, d / 40)));
            int i = trail.Nearest(chaser);
            return trail.Points[Math.Min(trail.Points.Count - 1, i + 6)];
        }

        // the chaser's new velocity and spin: turn toward `aim`, speed toward `speed` (slower in a sharp turn), at most `accel` m/s²,
        // and its up turned to `up` (the road's, level when null); rows = its right, up, forward; +yaw turns it right. Height is the caller's: a drivatar has no physics
        // (nothing pulls it down), so its height is set straight from the road under it
        public static (float[] vel, float[] spin) Steer(float[] pos, float[] vel, float[][] rows, float[] aim, float speed, float accel, float dt, float[] up = null)
        {
            var to = V.Sub(aim, pos); to[1] = 0; var want = V.Unit(to);
            var fwd = V.Unit(new[] { rows[2][0], 0, rows[2][2] }); var right = V.Unit(new[] { rows[0][0], 0, rows[0][2] });
            float err = (float)Math.Atan2(V.Dot(want, right), V.Dot(want, fwd));
            float target = speed * Math.Max(0.3f, (float)Math.Cos(err));
            var dv = V.Sub(V.Mul(fwd, target), new[] { vel[0], 0, vel[2] });
            if (V.Len(dv) > accel * dt) dv = V.Mul(V.Unit(dv), accel * dt);
            var nv = V.Add(vel, dv);
            var spin = V.Add(V.Mul(V.Cross(rows[1], up ?? new float[] { 0, 1, 0 }), 4), new[] { 0, V.Clamp(3 * err, -2.5f, 2.5f), 0 });
            return (nv, spin);
        }

        public static void Test()
        {
            void Ok(bool c, string what) { if (!c) throw new Exception("modes test failed: " + what); }
            float[][] north = { new float[] { 1, 0, 0 }, new float[] { 0, 1, 0 }, new float[] { 0, 0, 1 } };
            var (sv, sp) = Steer(new float[3], new float[] { 0, -1, 10 }, north, new float[] { 50, 0, 50 }, 30, 12, 0.1f);
            Ok(sp[1] > 0 && sv[2] > 10 && sv[1] == -1 && Math.Abs(sv[0]) < 1.21f, "steers right toward a target ahead-right, speeds up along its nose");
            (sv, sp) = Steer(new float[3], new float[] { 0, 0, 20 }, north, new float[] { 0, 0, -50 }, 30, 12, 0.1f);
            Ok(Math.Abs(sp[1]) > 2 && sv[2] < 20, "a target behind: hard turn, slows down");
            float[][] tilted = { new float[] { 1, 0, 0 }, V.Unit(new float[] { 0.3f, 1, 0 }), new float[] { 0, 0, 1 } };
            (sv, sp) = Steer(new float[3], new float[] { 0, 0, 20 }, tilted, new float[] { 0, 0, 50 }, 20, 12, 0.1f);
            Ok(V.Dot(V.Cross(sp, tilted[1]), new float[] { 0, 1, 0 }) > 0, "tilted: turned back toward level");
            var hill = new Trail(); for (int z = 0; z <= 80; z += 8) hill.Add(new float[] { 0, z / 4f, z });   // a 25% slope
            Ok(Math.Abs(hill.HeightAt(new float[] { 3, 50, 37 }) - 9.25f) < 1e-3f, "road height between two trail points, beside the trail too");
            var hu = hill.UpAt(new float[] { 0, 0, 40 });
            Ok(hu[1] > 0.95f && hu[2] < -0.2f && Math.Abs(V.Dot(hu, V.Unit(new float[] { 0, 1, 4 }))) < 1e-3f, "uphill northward: up leans back, square to the road");
            Ok(Math.Abs(hill.HeightAt(new float[] { 0, 0, 100 }) - 20) < 1e-3f && float.IsNaN(new Trail().HeightAt(new float[3])), "past the end: its last height; no trail: none");
            var tr = new Trail();
            for (int z = 0; z <= 400; z += 2) tr.Add(new float[] { 0, 0, z });
            Ok(tr.Points.Count == 101 && tr.Behind(100).Value.pos[2] == 300 && tr.Behind(100).Value.dir[2] == 1, "trail every 4 m; 100 m back runs north");
            Ok(Aim(tr, new float[] { 1, 0, 100 }, new float[] { 0, 0, 400 }, new float[3])[2] == 124, "far: follows the trail ahead of itself");
            Ok(Aim(tr, new float[] { 0, 0, 380 }, new float[] { 0, 0, 400 }, new float[] { 0, 0, 20 })[2] == 410, "close: where you will be");
            tr.Add(new float[] { 500, 0, 400 }); Ok(tr.Points.Count == 1, "a teleport starts a new trail");
            var st = new Settings(System.IO.Path.Combine(System.IO.Path.GetTempPath(), "fh2overlay-modes-test.ini")) { BombMph = 50 };
            var m = new Modes(st);
            m.Start(Kind.Bomb);
            m.Step(10, 0, false, false, 0, float.NaN, 0.1f); Ok(!m.Armed, "bomb waits below the speed");
            m.Step(25, 0, false, false, 0, float.NaN, 0.1f); Ok(m.Armed, "bomb arms at 50 mph");
            for (int i = 0; i < 30; i++) m.Step(25, 0, false, false, 0, float.NaN, 0.1f);
            m.Step(15, 0, false, false, 0, float.NaN, 0.3f); Ok(m.Mode == Kind.Bomb, "a short dip is forgiven");
            m.Step(15, 0, false, false, 0, float.NaN, 0.3f); Ok(m.Mode == Kind.Off && m.Explode && m.Result.StartsWith("BOOM"), "too slow for 0.5 s: boom");

            st.ScoreSeconds = 10; m.Start(Kind.Score);
            m.Step(20, 30, false, false, 1, float.NaN, 0.1f); Ok(m.Chain > 250 && m.Skills == 2, "a near miss and a new drift build the chain");
            m.Step(20, 0, false, true, 0, float.NaN, 0.1f); Ok(m.Chain == 0 && m.Score == 0, "a crash loses the chain");
            m.Step(0, 0, false, false, 1, float.NaN, 0.1f);
            for (int i = 0; i < 40; i++) m.Step(0, 0, false, false, 0, float.NaN, 0.1f);
            Ok(m.Score == 250 * 1.25f && m.Chain == 0, "the chain banks after 3 s, times the multiplier");
            for (int i = 0; i < 60; i++) m.Step(0, 0, false, false, 0, float.NaN, 0.1f);
            Ok(m.Mode == Kind.Off && m.Result.StartsWith("SCORE"), "score attack ends on time");

            m.Start(Kind.Pursuit);
            for (int i = 0; i < 90; i++) m.Step(30, 0, false, false, 0, 300, 0.1f);
            Ok(m.Escapes == 1, "8 s far away: escaped");
            Ok(m.Chasers == 2, "a second chaser after the first escape");
            for (int i = 0; i < 24; i++) m.Step(2, 0, false, false, 0, 5, 0.1f);
            Ok(m.Mode == Kind.Pursuit, "not busted before 2.5 s");
            for (int i = 0; i < 3; i++) m.Step(2, 0, false, false, 0, 5, 0.1f);
            Ok(m.Mode == Kind.Off && m.Result.StartsWith("BUSTED"), "2.5 s stopped next to it: busted");
            m.Start(Kind.Pursuit);
            m.Step(30, 0, false, false, 0, 5, 0.1f, true); m.Step(30, 0, false, false, 0, 5, 0.1f, true);
            Ok(m.Mode == Kind.Pursuit && m.Hits == 2, "two hits: still going");
            m.Step(30, 0, false, false, 0, 5, 0.1f, true);
            Ok(m.Mode == Kind.Off && m.Result.StartsWith("WRECKED"), "the third hit wrecks you");

            var n = new Modes(st); float[] Z = { 0, 0, 0 }, fast = { 0, 0, 20 };
            n.CountNearMisses(new[] { (1L, new float[] { 2, 0, -6 }, Z) }, Z, fast, false);
            n.CountNearMisses(new[] { (1L, new float[] { 2, 0, 0 }, Z) }, Z, fast, false);
            Ok(n.CountNearMisses(new[] { (1L, new float[] { 2, 0, 9 }, Z) }, Z, fast, false) == 1, "passing 2 m away is a near miss");
            n.CountNearMisses(new[] { (2L, new float[] { 6, 0, 0 }, Z) }, Z, fast, false);
            Ok(n.CountNearMisses(new[] { (2L, new float[] { 6, 0, 9 }, Z) }, Z, fast, false) == 0, "6 m away is not");
            try { System.IO.File.Delete(System.IO.Path.Combine(System.IO.Path.GetTempPath(), "fh2overlay-modes-test.ini")); } catch { }
        }
    }
}
