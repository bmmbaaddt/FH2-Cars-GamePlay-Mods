// The game-file mods (what the FH2 Mods window did): settings.json and cars.json next to the overlay, Apply (apply.ps1, then
// car_power.py), Restore originals, and starting the game. They change the game's files, so the game must be closed.
using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text;
using System.Web.Script.Serialization;

namespace FH2Overlay
{
    sealed class Setup
    {
        public static readonly string Dir = AppDomain.CurrentDomain.BaseDirectory.TrimEnd('\\');
        public static readonly string Root = Path.GetDirectoryName(Dir);   // this folder sits next to azimuth.exe
        static readonly string SettingsPath = Path.Combine(Dir, "settings.json"), CarsPath = Path.Combine(Dir, "cars.json");
        static readonly JavaScriptSerializer Json = new JavaScriptSerializer();

        // the same defaults as the settings.json the tool ships, for keys an older file doesn't have
        public static readonly Dictionary<string, object> Defaults = new Dictionary<string, object> {
            { "CockpitFOV", 48 }, { "SeatOffsetZ", 0 }, { "SeatOffsetY", 0 }, { "CreditsPerLevel", 0 }, { "CreditsPerKm", 1000 },
            { "SkillPointsMultiplier", 1 }, { "SkillXPMultiplier", 1 }, { "MaxSkillMultiplier", 5 }, { "SkillChainSeconds", 0 },
            { "FreeRoamTrafficPercent", 100 }, { "RaceTraffic", true }, { "RoadTripTraffic", true }, { "CrashesKeepSkillChain", false },
            { "GhostTraffic", false }, { "NoCrashSpin", false }, { "SlideOffWalls", false }, { "LightBreakables", false }, { "WorldControls", false },
            { "PackEngineParts", 0 }, { "PackEngineSwaps", false }, { "PackTrafficCars", false }, { "PackFreeCars", false },
            { "PackUltraTires", 0 }, { "PackWeight", 0 }, { "PackDrivetrains", false }, { "PackTurbos", false }, { "PackFreeParts", false },
            { "HoodFOV", 66 }, { "BumperFOV", 48 }, { "ChaseNearFOV", 48.5 }, { "ChaseFarFOV", 58 } };
        public static readonly string[] CarFields = { "power", "drag", "downforce", "grip", "brakes", "turbo", "gearing", "redline", "gears", "rpm_cap" };

        public readonly Dictionary<string, object> Values;
        public readonly SortedDictionary<int, Dictionary<string, double>> Cars = new SortedDictionary<int, Dictionary<string, double>>();
        public List<(int id, string name)> CarList;   // every drivable car, from car_power.py --list (null until loaded)
        public string Status = "";
        public volatile bool Busy;

        public Setup()
        {
            Values = File.Exists(SettingsPath) ? Json.Deserialize<Dictionary<string, object>>(File.ReadAllText(SettingsPath)) : new Dictionary<string, object>();
            foreach (var d in Defaults) if (!Values.ContainsKey(d.Key)) Values[d.Key] = d.Value;
            if (File.Exists(CarsPath))
                foreach (var c in Json.Deserialize<Dictionary<string, Dictionary<string, object>>>(File.ReadAllText(CarsPath)))
                    Cars[int.Parse(c.Key)] = c.Value.ToDictionary(v => v.Key, v => Convert.ToDouble(v.Value));
        }

        public double Num(string key) => Convert.ToDouble(Values[key]);
        public bool Flag(string key) => Convert.ToBoolean(Values[key]);
        public double Car(int id, string field) => Cars.TryGetValue(id, out var m) && m.TryGetValue(field, out double v) ? v : field == "rpm_cap" ? 10500 : 1;
        public void SetCar(int id, string field, double value)
        {
            if (!Cars.TryGetValue(id, out var m)) Cars[id] = m = new Dictionary<string, double>();
            if (field == "rpm_cap" && value == 10500) m.Remove(field); else m[field] = value;
            if (m.All(v => v.Key == "rpm_cap" || v.Value == 1)) Cars.Remove(id);   // all x1: stock (a lone rpm cap does nothing)
            Save();
        }
        public void Save()
        {
            // python's json wants no BOM
            File.WriteAllText(SettingsPath, Json.Serialize(Values), new UTF8Encoding(false));
            File.WriteAllText(CarsPath, Json.Serialize(Cars.ToDictionary(c => c.Key.ToString(), c => c.Value)), new UTF8Encoding(false));
        }
        public string ExportTune(int id) => Json.Serialize(new Dictionary<string, object> { { "car", id }, { "mods", Cars.TryGetValue(id, out var m) ? m : new Dictionary<string, double>() } });
        public int ImportTune(string json)
        {
            var t = Json.Deserialize<Dictionary<string, object>>(json);
            int id = Convert.ToInt32(t["car"]);
            Cars.Remove(id);
            foreach (var kv in (Dictionary<string, object>)t["mods"]) if (CarFields.Contains(kv.Key)) SetCar(id, kv.Key, Convert.ToDouble(kv.Value));
            Save(); return id;
        }

        // the game of this install (another FH2 install's game doesn't count)
        public static bool GameRunning() => Process.GetProcessesByName("azimuth").Any(p => { try { return p.MainModule.FileName.StartsWith(Root + "\\", StringComparison.OrdinalIgnoreCase); } catch { return true; } });

        static (int code, string[] lines) Run(string exe, string args)
        {
            var p = Process.Start(new ProcessStartInfo(exe, args) { WorkingDirectory = Dir, UseShellExecute = false, CreateNoWindow = true, RedirectStandardOutput = true, RedirectStandardError = true });
            var err = p.StandardError.ReadToEndAsync(); string output = p.StandardOutput.ReadToEnd(); p.WaitForExit();
            return (p.ExitCode, (output + "\n" + err.Result).Split('\n').Select(l => l.Trim()).Where(l => l != "").ToArray());
        }
        static (int code, string[] lines) Python(string args) => Run("python", $"-I \"{Path.Combine(Dir, "car_power.py")}\" {args}");

        public void LoadCars()
        {
            var (code, lines) = Python("--list");
            if (code != 0) { Status = "Car list: " + lines.LastOrDefault(); CarList = new List<(int, string)>(); return; }
            CarList = Json.Deserialize<List<Dictionary<string, object>>>(string.Join("", lines)).Select(c => (Convert.ToInt32(c["id"]), (string)c["name"])).ToList();
        }
        public string CarName(int id) => CarList?.FirstOrDefault(c => c.id == id).name ?? $"car {id}";

        // true when it worked; Status says what happened (only the last line of each step, like the old window)
        public bool Apply(bool restore = false)
        {
            if (GameRunning()) { Status = "Close the game first: these change the game's files."; return false; }
            Save();
            var (code, lines) = Run("powershell", $"-NoProfile -ExecutionPolicy Bypass -File \"{Path.Combine(Dir, "apply.ps1")}\"" + (restore ? " -Restore" : ""));
            if (code != 0) { Status = "Failed: " + lines.FirstOrDefault(); return false; }
            var (carCode, carLines) = Python(restore ? "--restore" : "");
            if (carCode != 0) { Status = "Car mods failed: " + carLines.LastOrDefault(); return false; }
            Status = $"Done: {lines.LastOrDefault()}; cars: {carLines.LastOrDefault()}"; return true;
        }
        public string RandomCar()
        {
            var (code, lines) = Python("--random");
            return code == 0 && int.TryParse(lines.LastOrDefault(), out int id) ? id.ToString() : "Random car: " + lines.LastOrDefault();
        }
        public static Dictionary<string, object> Curve(int car)
        {
            var (code, lines) = Python($"--curve {car}");
            return code == 0 ? Json.Deserialize<Dictionary<string, object>>(lines.Last()) : null;
        }

        public static void Play()
        {
            Process.Start(new ProcessStartInfo(Path.Combine(Root, "azimuth.exe")) { WorkingDirectory = Root });
            string impulse = Path.Combine(Root, "SimpleImpulseH2");   // trigger rumble for Xbox controllers; waits for the game, then hooks it
            if (File.Exists(Path.Combine(impulse, "FH2TriggerBridge.dll")))
                Process.Start(new ProcessStartInfo("python", $"\"{Path.Combine(impulse, "source", "fh2_trigger.py")}\"") { WorkingDirectory = impulse, WindowStyle = ProcessWindowStyle.Minimized });
        }
    }
}
