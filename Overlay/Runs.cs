// Run timers (0-60 to 0-200 mph and 1/8 to 1 mile, or 0-100 to 0-400 km/h and 100 to 2000 m) from a standing start, best times, and a drag light tree with
// reaction times. Fed the speed every tick; knows nothing about the game.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;

namespace FH2Overlay
{
    sealed class Runs
    {
        static readonly (string name, float mph)[] MphSpeeds = { ("0-60 mph", 60), ("0-100 mph", 100), ("0-150 mph", 150), ("0-200 mph", 200) };
        static readonly (string name, float mph)[] KmhSpeeds = { ("0-100 km/h", 100 / Units.KmPerMile), ("0-200 km/h", 200 / Units.KmPerMile), ("0-300 km/h", 300 / Units.KmPerMile), ("0-400 km/h", 400 / Units.KmPerMile) };
        static readonly (string name, float metres)[] Miles = { ("1/8 mile", 201.168f), ("1/4 mile", 402.336f), ("1/2 mile", 804.672f), ("1 mile", 1609.344f) };
        static readonly (string name, float metres)[] Metres_ = { ("100 m", 100), ("400 m", 400), ("1000 m", 1000), ("2000 m", 2000) };
        public bool Kmh;   // which set of runs is timed (set from the settings each tick)
        public (string name, float mph)[] Speeds => Kmh ? KmhSpeeds : MphSpeeds;
        public (string name, float metres)[] Distances => Kmh ? Metres_ : Miles;
        public string[] Names => Speeds.Select(x => x.name).Concat(Distances.Select(x => x.name)).ToArray();

        public readonly Dictionary<string, float> Best = new Dictionary<string, float>();
        public readonly Dictionary<string, float> Last = new Dictionary<string, float>();
        public readonly List<string> History = new List<string>();   // last 10 drag runs
        public float Elapsed, Metres, TopMph;
        public bool Running;
        public string Message = "";

        // the light tree: staged = stopped for a second; ambers then green; leaving before green is a red light
        public bool Tree;           // set from the settings each tick
        public bool Pro;
        public int Lights;          // 0 off, 1..3 ambers lit, 4 green, -1 red
        public float Reaction = float.NaN;
        float stoppedFor, treeClock = -1, greenAt;
        bool fouled;

        public void Load(string saved)
        {
            foreach (var part in (saved ?? "").Split(';'))
            {
                var kv = part.Split('=');
                if (kv.Length == 2 && float.TryParse(kv[1], NumberStyles.Float, CultureInfo.InvariantCulture, out float t)) Best[kv[0]] = t;
            }
        }
        public string Save() => string.Join(";", Best.Select(b => b.Key + "=" + b.Value.ToString("R", CultureInfo.InvariantCulture)));

        public bool Step(float speedMs, float dt)   // true when a best time changed
        {
            float mph = speedMs / Motion.MsPerMph;
            bool improved = false;
            if (!Running)
            {
                bool staged = stoppedFor > 0.5f;
                if (speedMs < 0.3f) stoppedFor += dt; else stoppedFor = 0;
                if (Tree)
                {
                    if (stoppedFor > 1 && treeClock < 0) { treeClock = 0; Lights = 0; Reaction = float.NaN; fouled = false; }
                    if (treeClock >= 0)
                    {
                        treeClock += dt;
                        greenAt = Pro ? 0.8f : 1.5f;   // pro: all ambers together, then green 0.4 s later; normal: one amber every 0.5 s
                        Lights = treeClock >= greenAt ? 4 : Pro ? (treeClock >= 0.4f ? 3 : 0) : (int)(treeClock / 0.5f);
                    }
                }
                if (speedMs >= 0.3f && (staged || treeClock >= 0))
                {
                    Running = true; Elapsed = 0; Metres = 0; TopMph = 0; Last.Clear(); Message = "";
                    if (treeClock >= 0)
                    {
                        Reaction = treeClock - greenAt;
                        fouled = Reaction < 0;
                        if (fouled) Lights = -1;
                        Elapsed = Math.Max(0, Reaction);   // the clock runs from the green light
                    }
                    treeClock = -1;
                }
                if (!Running) return false;
            }
            Elapsed += dt; Metres += speedMs * dt; TopMph = Math.Max(TopMph, mph);
            foreach (var (name, target) in Speeds) if (mph >= target && !Last.ContainsKey(name)) improved |= Record(name, Elapsed);
            foreach (var (name, target) in Distances) if (Metres >= target && !Last.ContainsKey(name)) improved |= Record(name, Elapsed);
            if (speedMs < 0.3f || Last.ContainsKey(Distances.Last().name) || Elapsed > 120)
            {
                Running = false; stoppedFor = 0;
                if (Last.Count > 0 && Tree)
                {
                    string q = Last.TryGetValue(Distances[1].name, out float qt) ? $"{Distances[1].name} {qt:0.000}s" : Last.Keys.Last() + $" {Last.Values.Last():0.000}s";
                    History.Insert(0, (fouled ? "FOUL  " : $"RT {Reaction:0.000}  ") + q + $"  top {(Kmh ? TopMph * Units.KmPerMile : TopMph):0} {Units.Name(Kmh)}");
                    if (History.Count > 10) History.RemoveAt(10);
                }
            }
            return improved;
        }

        bool Record(string name, float t)
        {
            Last[name] = t;
            if (fouled || (Best.TryGetValue(name, out float b) && b <= t)) return false;
            Best[name] = t; Message = $"new best {name}: {t:0.000}s";
            return true;
        }

        public static void Test()
        {
            void Ok(bool c, string what) { if (!c) throw new Exception("runs test failed: " + what); }
            var r = new Runs();
            for (int i = 0; i < 100; i++) r.Step(0, 0.01f);              // staged
            float v = 0;
            for (int i = 0; i < 2000 && !r.Last.ContainsKey("0-60 mph"); i++) { v += 9.81f * 0.01f; r.Step(v, 0.01f); }
            Ok(r.Last.ContainsKey("0-60 mph") && Math.Abs(r.Last["0-60 mph"] - 26.8224f / 9.81f) < 0.1f, "0-60 at 1 g takes about 2.7 s (the clock starts at walking pace)");
            Ok(r.Best["0-60 mph"] == r.Last["0-60 mph"], "first run is the best");
            var t = new Runs { Tree = true };
            for (int i = 0; i < 160; i++) t.Step(0, 0.01f);              // 1 s staged, then 0.6 s of tree
            Ok(t.Lights == 1, "first amber after 0.5 s");
            for (int i = 0; i < 100; i++) t.Step(0, 0.01f);
            Ok(t.Lights == 4, "green after 1.5 s");
            t.Step(1, 0.01f);
            Ok(t.Running && t.Reaction > 0.05f && t.Reaction < 0.15f, "reaction time from green (launched about 0.1 s after it)");
        }
    }
}
