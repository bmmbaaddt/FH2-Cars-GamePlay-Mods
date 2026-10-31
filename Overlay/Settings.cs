// Every setting is a public field; overlay.ini (next to the exe) stores them as name=value, and keys as Key.<action>=<key>.
using System;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Windows.Forms;

namespace FH2Overlay
{
    sealed class Settings
    {
        // Boost
        public float Boost = 1.5f;              // x stock torque while boost is on
        public float NitrousPower = 100;        // % more torque while nitrous fires
        public float NitrousSeconds = 6;        // a full tank lasts this long
        public bool InfiniteNitrous = false;
        public bool Flames = true;              // pops and bangs with boost and nitrous
        public bool AntiLag = true;             // pops when lifting off near the redline
        public float BurnoutSeconds = 5;
        // Launch
        public float LaunchRpmPercent = 70;     // of the redline
        public float LaunchHoldSeconds = 3;
        public float LaunchPower = 30;          // % more torque after the launch
        public float LaunchPowerSeconds = 5;
        public float RollingMph = 40;
        public float WheelieDegrees = 20;
        // Handling
        public float DriftRearGrip = 0.5f;      // x rear tyre grip while drift is on
        public float GripBoost = 1.5f;          // x grip on every tyre while more grip is on
        public float SteerDegrees = 60;         // front wheel lock while steering lock is on (most cars: 42 stock)
        public bool SuperBrakes = false;
        public float SuperBrakeG = 2;           // extra braking, in g, when the brake is pressed hard
        public float LimiterMph = 80;
        // Stunts and air
        public float JumpMetres = 3;
        public float RocketMph = 35;
        public float AirRate = 2.5f;            // rad/s of pitch and roll from the stick in the air
        public float GravityPercent = 40;
        public float UfoMph = 70;
        public float UfoClimbMph = 25;
        // Timers and HUD
        public bool Kmh = false;
        public bool ShowSpeedo = true, ShowNitrous = true, ShowStatus = true, ShowTimer = true, ShowShiftLight = true, ShowStats = true, ShowPedals = false;
        public bool DragTree = false, ProTree = false;
        public float HudScale = 1;
        public int SpeedoX = -1, SpeedoY = -1, CardX = -1, CardY = -1, PedalsX = -1, PedalsY = -1;   // -1 = default corner
        public string Accent = "#22D3EE";
        public int MenuOpacity = 94;
        // Game modes
        public float BombMph = 50;              // speed bomb: arms at this speed, blows up below it (5 mph more every minute)
        public float ScoreSeconds = 120;        // score attack length
        public float ChaserMph = 150;           // pursuit: the chaser's top speed
        public float ChaserHeight = 0;          // metres added to a drivatar chaser's height (cars sit at different heights than yours)
        public float BombBest = 0, ScoreBest = 0, PursuitBest = 0;
        // saved teleport spots: x,y,z then 12 orientation floats, ';' between spots
        public string Spots = "";
        public string BestTimes = "";

        public Dictionary<string, Keys> Bind = new Dictionary<string, Keys>(Actions.Defaults);
        readonly string path;

        public Settings(string path)
        {
            this.path = path;
            if (!File.Exists(path)) return;
            foreach (var line in File.ReadAllLines(path))
            {
                int eq = line.IndexOf('='); if (eq < 1 || line.StartsWith(";")) continue;
                string k = line.Substring(0, eq).Trim(), v = line.Substring(eq + 1).Trim();
                if (k.StartsWith("Key.")) { if (Enum.TryParse(v, out Keys key) && Bind.ContainsKey(k.Substring(4))) Bind[k.Substring(4)] = key; continue; }
                var f = typeof(Settings).GetField(k); if (f == null) continue;
                try { f.SetValue(this, Convert.ChangeType(v, f.FieldType, CultureInfo.InvariantCulture)); } catch { }
            }
            if (Accent == "#E0282E") Accent = "#22D3EE";   // the first test build's red
        }

        public void Save()
        {
            var lines = new List<string> { "; FH2 Mods Overlay settings. Delete this file to go back to the defaults." };
            foreach (var f in typeof(Settings).GetFields(BindingFlags.Public | BindingFlags.Instance).Where(f => f.FieldType != typeof(Dictionary<string, Keys>)))
                lines.Add(f.Name + "=" + Convert.ToString(f.GetValue(this), CultureInfo.InvariantCulture));
            lines.AddRange(Bind.Select(b => "Key." + b.Key + "=" + b.Value));
            try { File.WriteAllLines(path, lines); } catch { }
        }
    }

    static class Actions
    {
        // action -> (default key, what it does); a key of None is unbound
        public static readonly (string name, Keys key, string help)[] All = {
            ("Menu", Keys.Insert, "show / hide this menu"),
            ("AllOff", Keys.End, "turn every effect off"),
            ("Boost", Keys.F6, "boost on / off"),
            ("BoostDown", Keys.F7, "less boost"),
            ("BoostUp", Keys.F8, "more boost"),
            ("Nitrous", Keys.D9, "nitrous (hold)"),
            ("Burnout", Keys.Home, "burnout (stopped)"),
            ("StandingLaunch", Keys.F5, "standing launch"),
            ("RollingLaunch", Keys.F9, "rolling launch"),
            ("WheelieLaunch", Keys.F11, "wheelie launch"),
            ("Drift", Keys.None, "drift on / off"),
            ("Grip", Keys.Delete, "more grip on / off"),
            ("SteerLock", Keys.None, "steering lock on / off"),
            ("Limiter", Keys.F3, "speed limiter on / off"),
            ("Wheelie", Keys.F12, "wheelie mode on / off"),
            ("Jump", Keys.F4, "jump"),
            ("BarrelRoll", Keys.D1, "barrel roll"),
            ("Spin360", Keys.D2, "360 spin"),
            ("Backflip", Keys.D3, "backflip"),
            ("Rocket", Keys.D4, "rocket boost"),
            ("Stop", Keys.D5, "instant stop"),
            ("AirControl", Keys.D6, "air control on / off"),
            ("LowGravity", Keys.PageUp, "low gravity on / off"),
            ("Ufo", Keys.PageDown, "UFO mode on / off"),
            ("UfoUp", Keys.R, "UFO up (hold)"),
            ("UfoDown", Keys.F, "UFO down (hold)"),
            ("Teleport", Keys.D7, "teleport to the selected spot"),
            ("SaveSpot", Keys.D8, "save a spot here"),
            ("Ghost", Keys.D0, "ghost mode on / off"),
            ("Cruise", Keys.F10, "cruise control on / off"),
            ("Flip", Keys.Back, "flip the car upright"),
            ("Units", Keys.U, "switch mph / km/h"),
            ("GameMode", Keys.F2, "game mode: off, bomb, score, pursuit"),
        };
        public static readonly Dictionary<string, Keys> Defaults = All.ToDictionary(a => a.name, a => a.key);
    }
}
