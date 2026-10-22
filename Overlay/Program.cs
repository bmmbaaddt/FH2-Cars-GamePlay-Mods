// FH2 Mods Overlay: waits for the game, runs the driving effects on their own thread, and shows the menu (Insert) and
// gauges over the game. "--test" runs the self-checks and exits.
using System;
using System.Collections.Concurrent;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;

namespace FH2Overlay
{
    static class Program
    {
        [DllImport("user32")] static extern short GetAsyncKeyState(int key);
        [DllImport("winmm")] static extern uint timeBeginPeriod(uint ms);
        [DllImport("winmm")] static extern uint timeEndPeriod(uint ms);
        [StructLayout(LayoutKind.Sequential)] struct Pad { public uint Packet; public ushort Buttons; public byte LT, RT; public short LX, LY, RX, RY; }
        [DllImport("xinput1_4", EntryPoint = "XInputGetState")] static extern int XInputGetState(int user, out Pad state);

        static Settings s;
        static Runs runs = new Runs();
        static volatile Game game;
        static volatile Driving driving;
        static volatile string state = "WAITING FOR GAME";
        static volatile bool stop, menuToggle;
        static readonly ConcurrentQueue<string> requests = new ConcurrentQueue<string>();
        static readonly System.Collections.Generic.HashSet<string> wasDown = new System.Collections.Generic.HashSet<string>();

        [STAThread]
        static int Main(string[] args)
        {
            if (args.Contains("--test"))
            {
                try { Motion.Test(); Runs.Test(); Console.WriteLine("PASS: motion, runs"); return 0; }
                catch (Exception e) { Console.WriteLine(e.Message); return 1; }
            }
            if (args.Contains("--probe"))   // read-only: what the overlay would see right now
            {
                try
                {
                    using (var g = Game.Attach())
                    {
                        if (g == null) { Console.WriteLine("game not running"); return 1; }
                        Console.WriteLine($"build ok, exhaust pops {(g.Backfires ? "supported" : "not supported")}, game in front: {g.Focused}, driving: {g.Driving()}");
                        var car = g.Player();
                        if (car == null) { Console.WriteLine("no player car (menus or loading)"); return 0; }
                        Console.WriteLine($"car {car.Model}: speed {g.SpeedMs(car) / Motion.MsPerMph:0.0} mph, {g.Rpm(car):0} / {g.RedlineRpm(car):0} rpm, torque {g.Torque(car)} (stock {car.StockTorque})");
                        Console.WriteLine($"position {string.Join(", ", g.Vec(car, Game.Position).Select(x => x.ToString("0.0")))}, up {string.Join(", ", g.Rows(car, Game.Basis)[1].Select(x => x.ToString("0.00")))}");
                        try { var t = g.TyresOf(car); Console.WriteLine($"tyre grip front {g.Grip(car, t[0])}, rear {g.Grip(car, t[1])}"); } catch (Exception e) { Console.WriteLine("tyres: " + e.Message); }
                        Console.WriteLine($"exhaust mode {g.BackfireModeOf(car)}, brakes {string.Join(", ", g.Brakes(car))}, throttle {g.Throttle():0.00}");
                    }
                    return 0;
                }
                catch (Exception e) { Console.WriteLine(e.Message); return 1; }
            }
            if (args.Contains("--live-test"))   // runs each effect on the player car through Driving, reads the game back, then all off
            {
                using (var g = Game.Attach())
                {
                    if (g == null) { Console.WriteLine("game not running"); return 1; }
                    var car = g.Player(); if (car == null || !g.Driving()) { Console.WriteLine("get in the car with the game window in front"); return 1; }
                    var st = new Settings(Path.Combine(Path.GetTempPath(), "fh2overlay-test.ini"));
                    var d = new Driving(g, st);
                    int fails = 0;
                    void Run(int ticks, string press = null, string hold = null)
                    {
                        for (int i = 0; i < ticks; i++)
                        {
                            var input = new Input();
                            if (i == 0 && press != null) input.Pressed.Add(press);
                            if (hold != null) { input.Down.Add(hold); }
                            d.Tick(input, 0.005f); Thread.Sleep(5);
                        }
                    }
                    void Check(string what, bool ok) { Console.WriteLine((ok ? "ok    " : "FAIL  ") + what + (d.Error != "" ? "   (" + d.Error + ")" : "")); if (!ok) fails++; }
                    float stock = car.StockTorque;
                    Run(5, "Boost"); Check($"boost: torque {g.Torque(car)} = {stock} x 1.5", Game.Near(g.Torque(car), stock * 1.5f));
                    Run(5, "Boost"); Check($"boost off: torque {g.Torque(car)} back to {stock}", Game.Near(g.Torque(car), stock));
                    var t = g.TyresOf(car); float f0 = g.Grip(car, t[0]), r0 = g.Grip(car, t[1]);
                    Run(5, "Drift"); Check($"drift: rear grip {g.Grip(car, t[1])} = {r0} x 0.5, front {g.Grip(car, t[0])} unchanged", Game.Near(g.Grip(car, t[1]), r0 * 0.5f) && Game.Near(g.Grip(car, t[0]), f0));
                    Run(5, "Drift"); Check("drift off: grip back", Game.Near(g.Grip(car, t[1]), r0));
                    int ghost0 = g.Int(car.Sim - 0x1bc);   // 4 already when FH2 Mods' Ghost cars option is on (it keeps the player ghosted)
                    Run(5, "Ghost"); Check("ghost on", g.Int(car.Sim - 0x1bc) == 4);
                    Run(5, "Ghost"); Run(50); Check($"ghost off: {g.Int(car.Sim - 0x1bc)} (was {ghost0} before)", g.Int(car.Sim - 0x1bc) == ghost0);
                    var p0 = g.Vec(car, Game.Position);
                    Run(1, "Jump"); Run(60); var p1 = g.Vec(car, Game.Position);
                    Check($"jump: rose {p1[1] - p0[1]:0.00} m after 0.3 s", p1[1] - p0[1] > 1);
                    Run(600); Check("landed", Math.Abs(g.Vec(car, Game.Position)[1] - p0[1]) < 0.5f);
                    Run(1, "SaveSpot"); Check($"saved spot {d.SpotList.Count}", d.SpotList.Count > 0);
                    var spot = d.SpotList[d.SelectedSpot]; spot[1] += 5;     // 5 m above where the car stands: it just drops back
                    var before = g.Vec(car, Game.Position);
                    Run(1, "Teleport"); var lifted = g.Vec(car, Game.Position);
                    Check($"teleport: {lifted[1] - before[1]:0.0} m up (5.4 asked)", Math.Abs(lifted[1] - before[1] - 5.4f) < 0.3f && Math.Abs(lifted[0] - before[0]) < 0.3f);
                    Run(600); Check("dropped back", Math.Abs(g.Vec(car, Game.Position)[1] - p0[1]) < 0.5f);
                    d.SpotList.RemoveAt(d.SelectedSpot);
                    var h0 = g.Vec(car, Game.Position)[1];
                    Run(1, "Flip"); var up = g.Rows(car, Game.Basis)[1];
                    Check($"flip upright: lifted {g.Vec(car, Game.Position)[1] - h0:0.0} m, up axis y {up[1]:0.00}", g.Vec(car, Game.Position)[1] - h0 > 1 && up[1] > 0.99f);
                    Run(600); Check("settled", Math.Abs(g.Vec(car, Game.Position)[1] - h0) < 0.5f);
                    bool kmh = st.Kmh; Run(1, "Units"); Check("units switch", st.Kmh != kmh); Run(1, "Units");
                    Run(5, "Boost"); Run(5, "Grip"); Run(5, "Limiter"); Run(5, "LowGravity");
                    Run(5, "AllOff"); d.Shutdown();
                    Check("all off: torque, grip, ghost back", Game.Near(g.Torque(car), stock) && Game.Near(g.Grip(car, t[1]), r0) && Game.Near(g.Grip(car, t[0]), f0) && g.Int(car.Sim - 0x1bc) == ghost0);
                    Console.WriteLine(fails == 0 ? "PASS" : fails + " failed");
                    return fails;
                }
            }
            using (var single = new Mutex(true, "FH2ModsOverlay", out bool first))
            {
                if (!first) { MessageBox.Show("FH2 Mods Overlay is already running. Press Insert in the game.", "FH2 Mods Overlay"); return 0; }
                Application.EnableVisualStyles();
                s = new Settings(Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "overlay.ini"));
                runs.Load(s.BestTimes);
                var engine = new Thread(Engine) { IsBackground = true, Priority = ThreadPriority.AboveNormal, Name = "effects" };
                engine.Start();

                var hud = new Hud(s, () => driving, runs) { GameWindow = () => game?.Window ?? IntPtr.Zero };
                var menu = new Menu(s, () => driving, runs, hud, a => requests.Enqueue(a)) { GameState = () => state };
                hud.MenuOpen = () => menu.Visible;
                var tray = new NotifyIcon { Text = "FH2 Mods Overlay (Insert in game)", Icon = SystemIcons.Application, Visible = true };
                tray.ContextMenuStrip = new ContextMenuStrip();
                tray.ContextMenuStrip.Items.Add("Show menu", null, (o, e) => menu.ShowOver(game?.Window ?? IntPtr.Zero));
                tray.ContextMenuStrip.Items.Add("All off", null, (o, e) => requests.Enqueue("AllOff"));
                tray.ContextMenuStrip.Items.Add("Exit", null, (o, e) => Application.Exit());
                tray.DoubleClick += (o, e) => menu.ShowOver(game?.Window ?? IntPtr.Zero);
                var ui = new System.Windows.Forms.Timer { Interval = 30 };
                ui.Tick += (o, e) =>
                {
                    if (menuToggle) { menuToggle = false; if (menu.Visible) menu.Hide(); else menu.ShowOver(game?.Window ?? IntPtr.Zero); }
                    if (requests.Contains("Exit")) Application.Exit();
                };
                ui.Start();
                tray.ShowBalloonTip(3000, "FH2 Mods Overlay", "Running. Press Insert in the game for the menu.", ToolTipIcon.None);
                Application.ApplicationExit += (o, e) => tray.Visible = false;
                Application.Run();

                stop = true; engine.Join(1000);
                s.BestTimes = runs.Save(); s.Save();
            }
            return 0;
        }

        static void Engine()
        {
            timeBeginPeriod(1);
            var clock = System.Diagnostics.Stopwatch.StartNew();
            double last = 0; long nextAttach = 0;
            while (!stop)
            {
                try
                {
                    if (game == null || game.Exited)
                    {
                        if (game != null) { game.Dispose(); game = null; driving = null; state = "WAITING FOR GAME"; }
                        if (Environment.TickCount - nextAttach > 0)
                        {
                            nextAttach = Environment.TickCount + 2000;
                            try { var g = Game.Attach(); if (g != null) { game = g; var d = new Driving(g, s); d.LoadSpots(); driving = d; } }
                            catch (Exception e) { state = e.Message.StartsWith("Unsupported") ? "UNSUPPORTED GAME" : "CAN'T CONNECT"; }
                        }
                        Keys(false);
                        Thread.Sleep(50); continue;
                    }
                    double now = clock.Elapsed.TotalSeconds; float dt = (float)Math.Min(0.1, now - last); last = now;
                    var input = Keys(game.Focused);
                    while (requests.TryDequeue(out string r))
                    {
                        if (r == "Exit") { requests.Enqueue(r); break; }
                        if (r == "DeleteSpot") { DeleteSpot(); continue; }
                        if ((r == "SaveSpot" || r == "Teleport") && !driving.Live) { pending = r; continue; }   // waits for the game to have focus again
                        input.Pressed.Add(r);
                    }
                    if (pending != null && driving.Live) { input.Pressed.Add(pending); pending = null; }
                    driving.Tick(input, dt);
                    state = !driving.InCar ? "IN MENUS" : driving.Live ? "LIVE" : "PAUSED";
                    runs.Tree = s.DragTree; runs.Pro = s.ProTree; runs.Kmh = s.Kmh;
                    bool better;
                    lock (runs) better = driving.InCar && runs.Step(driving.SpeedMs, dt);
                    if (better) { s.BestTimes = runs.Save(); s.Save(); }
                }
                catch (Exception e) { if (driving != null) driving.Error = e.Message; }
                Thread.Sleep(4);
            }
            try { driving?.Shutdown(); } catch { }
            timeEndPeriod(1);
        }
        static string pending;
        static bool lsWas;

        static void DeleteSpot()
        {
            var d = driving; if (d == null || d.SelectedSpot >= d.SpotList.Count) return;
            d.SpotList.RemoveAt(d.SelectedSpot); d.SelectedSpot = Math.Max(0, d.SelectedSpot - 1);
            s.Spots = string.Join(";", d.SpotList.Select(x => string.Join(",", x.Select(f => f.ToString("R", System.Globalization.CultureInfo.InvariantCulture)))));
            s.Save();
        }

        static bool Key(Keys k) => k != System.Windows.Forms.Keys.None && (GetAsyncKeyState((int)k) & 0x8000) != 0;

        // bound keys while the game is in front (not with Ctrl or Alt held: those combinations belong to FH2 Mods' world keys);
        // the menu key also works while the menu itself is in front
        static Input Keys(bool gameFocused)
        {
            var input = new Input();
            bool menuFocused = Form.ActiveForm is Menu;
            bool chord = Key(System.Windows.Forms.Keys.ControlKey) || Key(System.Windows.Forms.Keys.Menu);
            foreach (var b in s.Bind)
            {
                bool down = (gameFocused || menuFocused && b.Key == "Menu") && !chord && Key(b.Value);
                if (down) { input.Down.Add(b.Key); if (!wasDown.Contains(b.Key)) input.Pressed.Add(b.Key); }
            }
            wasDown.Clear(); wasDown.UnionWith(input.Down);
            if (input.Pressed.Remove("Menu")) menuToggle = true;
            if (!gameFocused) { input.Pressed.Clear(); input.Down.Clear(); return input; }

            // left stick from the first connected controller, plus the arrow keys
            for (int i = 0; i < 4; i++)
                if (XInputGetState(i, out Pad p) == 0)
                {
                    float Axis(short v) { float x = v / 32767f; return Math.Abs(x) < 0.2f ? 0 : x; }
                    input.StickX = Axis(p.LX); input.StickY = Axis(p.LY);
                    if ((p.Buttons & 0x0200) != 0) input.Down.Add("UfoUp");     // RB / LB climb and descend in UFO mode
                    if ((p.Buttons & 0x0100) != 0) input.Down.Add("UfoDown");
                    bool ls = (p.Buttons & 0x0040) != 0;   // left stick click: rocket, like its key
                    if (ls && !lsWas) input.Pressed.Add("Rocket");
                    lsWas = ls;
                    break;
                }
            if (Key(System.Windows.Forms.Keys.Left)) input.StickX = -1; if (Key(System.Windows.Forms.Keys.Right)) input.StickX = 1;
            if (Key(System.Windows.Forms.Keys.Up)) input.StickY = 1; if (Key(System.Windows.Forms.Keys.Down)) input.StickY = -1;
            return input;
        }
    }
}
