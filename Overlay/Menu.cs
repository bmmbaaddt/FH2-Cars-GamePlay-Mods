// The menu (Insert): tabs across the top, cards of settings below. Switches act at once; numbers are used from the
// next tick; speeds show in mph or km/h (the switch in the header); everything is saved when the menu closes.
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace FH2Overlay
{
    sealed class Menu : Form
    {
        static readonly Color Back = Color.FromArgb(15, 20, 28), Bar = Color.FromArgb(10, 14, 20), CardBack = Color.FromArgb(24, 31, 42),
                              Field = Color.FromArgb(32, 41, 55), Ink = Color.FromArgb(232, 238, 245), Dim = Color.FromArgb(135, 150, 168);
        Color Accent { get { try { return ColorTranslator.FromHtml(s.Accent); } catch { return Color.DeepSkyBlue; } } }

        readonly Settings s;
        readonly Func<Driving> driving;
        readonly Runs runs;
        readonly Hud hud;
        readonly Action<string> request;   // a one-shot action for the engine thread (teleport, all off...)
        readonly FlowLayoutPanel tabs = new FlowLayoutPanel { Dock = DockStyle.Top, Height = 40, Padding = new Padding(12, 6, 0, 0) };
        readonly FlowLayoutPanel page = new FlowLayoutPanel { Dock = DockStyle.Fill, AutoScroll = true, Padding = new Padding(12, 10, 0, 10) };
        readonly Label pill = new Label { AutoSize = true, Font = new Font("Bahnschrift SemiBold", 9), Padding = new Padding(8, 3, 8, 3) };
        readonly Button units = new Button { Width = 110, Height = 26, FlatStyle = FlatStyle.Flat, Font = new Font("Bahnschrift SemiBold", 9), Cursor = Cursors.Hand };
        readonly ToolTip tips = new ToolTip { InitialDelay = 300, AutoPopDelay = 15000 };
        readonly Dictionary<string, Button> tabButtons = new Dictionary<string, Button>();
        readonly Dictionary<string, Action> builders = new Dictionary<string, Action>();
        readonly List<Action> refreshers = new List<Action>();
        Panel card; int cardY;
        string current, capture;
        public Func<string> GameState = () => "WAITING FOR GAME";
        Point grab;

        public Menu(Settings s, Func<Driving> driving, Runs runs, Hud hud, Action<string> request)
        {
            this.s = s; this.driving = driving; this.runs = runs; this.hud = hud; this.request = request;
            Text = "FH2 Mods Overlay"; FormBorderStyle = FormBorderStyle.None; TopMost = true; ShowInTaskbar = false;
            BackColor = Back; ForeColor = Ink; Font = new Font("Segoe UI", 9.5f); Size = new Size(760, 560); StartPosition = FormStartPosition.Manual;
            Opacity = s.MenuOpacity / 100.0; KeyPreview = true;

            var head = new Panel { Dock = DockStyle.Top, Height = 50, BackColor = Bar };
            var title = new Label { Text = "FH2 MODS", AutoSize = true, Font = new Font("Bahnschrift SemiBold", 15), ForeColor = Ink, Location = new Point(16, 12) };
            var sub = new Label { Text = "OVERLAY", AutoSize = true, Font = new Font("Bahnschrift", 10), ForeColor = Accent, Location = new Point(122, 18) };
            pill.Anchor = units.Anchor = AnchorStyles.Top | AnchorStyles.Right;
            units.Location = new Point(Width - 160, 12); units.FlatAppearance.BorderColor = Accent;
            units.Click += (o, e) => { s.Kmh = !s.Kmh; s.Save(); Open(current); };
            tips.SetToolTip(units, "Speeds in the menu, gauges and run timers. [" + KeyName(s.Bind["Units"]) + "] in game.");
            var close = new Label { Text = "✕", AutoSize = true, Font = new Font("Segoe UI", 12), ForeColor = Dim, Cursor = Cursors.Hand, Location = new Point(Width - 40, 13), Anchor = AnchorStyles.Top | AnchorStyles.Right };
            close.Click += (o, e) => Hide();
            head.Controls.AddRange(new Control[] { title, sub, pill, units, close });
            foreach (Control c in new Control[] { head, title, sub })
            {
                c.MouseDown += (o, e) => grab = e.Location;
                c.MouseMove += (o, e) => { if (e.Button == MouseButtons.Left) Location = new Point(Left + e.X - grab.X, Top + e.Y - grab.Y); };
            }
            var foot = new Panel { Dock = DockStyle.Bottom, Height = 44, BackColor = Bar };
            var allOff = Btn("ALL OFF", () => request("AllOff")); allOff.Location = new Point(14, 8); allOff.ForeColor = Accent; allOff.FlatAppearance.BorderColor = Accent;
            var hint = new Label { AutoSize = true, ForeColor = Dim, Location = new Point(110, 14), Text = $"[{KeyName(s.Bind["Menu"])}] shows / hides this menu.  Effects pause while it is in front: click the game to drive." };
            foot.Controls.AddRange(new Control[] { allOff, hint });
            tabs.BackColor = Bar;
            Controls.Add(page); Controls.Add(tabs); Controls.Add(foot); Controls.Add(head);

            Tab("Driving", DrivingTab); Tab("Stunts & air", StuntsTab); Tab("Modes", ModesTab); Tab("Timers", TimersTab); Tab("Teleport", TeleportTab);
            Tab("Gauges", GaugesTab); Tab("Keys", KeysTab); Tab("About", AboutTab);
            Open("Driving");

            var t = new Timer { Interval = 200 }; t.Tick += (o, e) => { if (Visible) Tick(); }; t.Start();
            VisibleChanged += (o, e) => { if (!Visible) { s.Save(); if (hud.Moving) hud.SetMoving(false); } };
            KeyDown += (o, e) => { if (capture == null && e.KeyCode == s.Bind["Menu"]) Hide(); };
        }

        void Tick()
        {
            string st = GameState();
            pill.Text = st;
            pill.BackColor = st == "LIVE" ? Accent : st == "PAUSED" ? Color.FromArgb(255, 190, 60) : Field;
            pill.ForeColor = st == "LIVE" || st == "PAUSED" ? Bar : Ink;
            pill.Location = new Point(units.Left - pill.Width - 10, 15);
            units.Text = s.Kmh ? "KM/H   → mph" : "MPH   → km/h";
            units.ForeColor = Ink;
            var d = driving();
            foreach (var kv in tabButtons) kv.Value.Text = kv.Key + (TabActive(kv.Key, d) ? "  •" : "");
            foreach (var r in refreshers) r();
        }
        bool TabActive(string name, Driving d)
        {
            if (d == null) return false;
            if (name == "Driving") return d.Boost || d.NitrousOn || d.BurnoutActive || d.LaunchActive || d.Drift || d.Grip || d.Limiter || d.Cruise;
            if (name == "Stunts & air") return d.Wheelie || d.Air || d.LowGravity || d.Ufo || d.Ghost;
            if (name == "Modes") return d.Modes.Mode != Modes.Kind.Off;
            return false;
        }

        void Tab(string name, Action build)
        {
            var b = new Button { Text = name, AutoSize = true, Height = 30, FlatStyle = FlatStyle.Flat, BackColor = Bar, ForeColor = Dim, Cursor = Cursors.Hand, Font = new Font("Bahnschrift", 10), Margin = new Padding(0, 0, 4, 0) };
            b.FlatAppearance.BorderSize = 0; b.FlatAppearance.MouseOverBackColor = CardBack;
            b.Click += (o, e) => Open(name);
            tabButtons[name] = b; builders[name] = build; tabs.Controls.Add(b);
        }
        void Open(string name)
        {
            current = name; capture = null;
            foreach (var kv in tabButtons) { bool on = kv.Key == name; kv.Value.ForeColor = on ? Ink : Dim; kv.Value.BackColor = on ? CardBack : Bar; }
            page.SuspendLayout(); page.Controls.Clear(); refreshers.Clear(); card = null;
            builders[name]();
            page.ResumeLayout();
            Tick();
        }

        // ---- building blocks: cards in two columns, rows inside a card
        void Card(string title, bool wide = false)
        {
            card = new Panel { Width = wide ? 716 : 352, Height = 40, BackColor = CardBack, Margin = new Padding(0, 0, 12, 12) };
            card.Paint += (o, e) => { using (var b = new SolidBrush(Accent)) e.Graphics.FillRectangle(b, 0, 0, 3, 24); };
            card.Controls.Add(new Label { Text = title.ToUpperInvariant(), AutoSize = true, Font = new Font("Bahnschrift SemiBold", 10), ForeColor = Ink, Location = new Point(12, 5) });
            cardY = 32; page.Controls.Add(card);
        }
        void Put(Control c, int height = 0)
        {
            c.Location = new Point(12, cardY); card.Controls.Add(c);
            cardY += (height > 0 ? height : c.Height) + 4; card.Height = cardY + 8;
        }
        void Note(string text)
        {
            int width = card.Width - 24;
            var l = new Label { Text = text, AutoSize = false, Width = width, ForeColor = Dim };
            l.Height = TextRenderer.MeasureText(text, l.Font, new Size(width, 0), TextFormatFlags.WordBreak).Height + 2;
            Put(l);
        }
        Panel Line(string label, string help)
        {
            var p = new Panel { Width = card.Width - 24, Height = 28 };
            var l = new Label { Text = label, AutoSize = false, Width = p.Width - 100, Height = 28, TextAlign = ContentAlignment.MiddleLeft };
            if (help != null) { tips.SetToolTip(l, help); l.Text += "  ⓘ"; }
            p.Controls.Add(l); Put(p); return p;
        }
        void Switch(string label, string help, Func<bool> get, Action<bool> set, string key = null)
        {
            var p = Line(label + (key != null && s.Bind[key] != Keys.None ? $"   [{KeyName(s.Bind[key])}]" : ""), help);
            var b = new Button { Width = 64, Height = 22, Location = new Point(p.Width - 66, 3), FlatStyle = FlatStyle.Flat, Cursor = Cursors.Hand, Font = new Font("Bahnschrift SemiBold", 8.5f) };
            void Show() { bool v = get(); b.Text = v ? "ON" : "OFF"; b.BackColor = v ? Accent : Field; b.ForeColor = v ? Bar : Dim; b.FlatAppearance.BorderColor = v ? Accent : Field; }
            b.Click += (o, e) => { set(!get()); Show(); };
            Show(); refreshers.Add(Show); p.Controls.Add(b);
        }
        void Live(string label, string help, string key, Func<Driving, bool> get, Action<Driving, bool> set) =>
            Switch(label, help, () => { var d = driving(); return d != null && get(d); }, v => { var d = driving(); if (d != null) set(d, v); }, key);
        void Setting(string label, string help, string field) =>
            Switch(label, help, () => (bool)typeof(Settings).GetField(field).GetValue(s), v => typeof(Settings).GetField(field).SetValue(s, v));

        // a number field; speed fields are kept in mph and shown in the chosen unit
        void Number(string label, string help, string field, decimal min, decimal max, decimal step, int decimals = 0, bool speed = false)
        {
            var f = typeof(Settings).GetField(field);
            decimal k = speed && s.Kmh ? (decimal)Units.KmPerMile : 1;
            var p = Line(label + (speed ? $"  ({Units.Name(s.Kmh)})" : ""), help);
            var n = new NumericUpDown { Minimum = Math.Round(min * k), Maximum = Math.Round(max * k), Increment = step, DecimalPlaces = decimals, Width = 82, Location = new Point(p.Width - 84, 3), BackColor = Field, ForeColor = Ink, BorderStyle = BorderStyle.None, TextAlign = HorizontalAlignment.Right, Font = new Font("Bahnschrift", 10) };
            decimal Shown() => Math.Max(n.Minimum, Math.Min(n.Maximum, Math.Round(Convert.ToDecimal(f.GetValue(s)) * k, decimals)));
            n.Value = Shown();
            n.ValueChanged += (o, e) => { if (Shown() != n.Value) f.SetValue(s, Convert.ChangeType(n.Value / k, f.FieldType)); };
            refreshers.Add(() => { if (!n.Focused && Shown() != n.Value) n.Value = Shown(); });   // F7 / F8 change boost while open
            p.Controls.Add(n);
        }
        Button Btn(string text, Action click)
        {
            var b = new Button { Text = text, AutoSize = true, Height = 28, FlatStyle = FlatStyle.Flat, BackColor = Field, ForeColor = Ink, Cursor = Cursors.Hand, Font = new Font("Bahnschrift", 9.5f), Margin = new Padding(0, 0, 6, 0) };
            b.FlatAppearance.BorderColor = Field;
            b.Click += (o, e) => click(); return b;
        }
        void Buttons(params Button[] bs)
        {
            var row = new FlowLayoutPanel { Width = card.Width - 24, Height = 32, BackColor = CardBack };
            row.Controls.AddRange(bs); Put(row);
        }
        static string KeyName(Keys k) => k >= Keys.D0 && k <= Keys.D9 ? ((int)k - (int)Keys.D0).ToString() : k == Keys.Next ? "PgDn" : k == Keys.Prior ? "PgUp" : k == Keys.Back ? "Backspace" : k.ToString();
        string K(string action) => s.Bind[action] == Keys.None ? "no key" : KeyName(s.Bind[action]);

        // ---- tabs
        void DrivingTab()
        {
            Card("Boost");
            Live("Boost", "More engine torque. F7 / F8 change it while driving.", "Boost", d => d.Boost, (d, v) => d.Boost = v);
            Number("Boost  (x torque)", null, "Boost", 1.05m, 20, 0.1m, 2);
            Setting("Pops and bangs", "Exhaust flames while boost or nitrous push the engine (cars that have them).", "Flames");
            Setting("Anti-lag", "Pops for 1.5 s after lifting off near the redline.", "AntiLag");
            Card("Nitrous");
            Note($"Hold [{K("Nitrous")}] with the throttle down. Refills a second after you let go.");
            Number("Power  (% more torque)", null, "NitrousPower", 5, 1000, 5);
            Number("Tank  (seconds)", null, "NitrousSeconds", 1, 60, 1);
            Setting("Infinite nitrous", "The tank never empties.", "InfiniteNitrous");
            Card("Launch");
            Note($"Standing [{K("StandingLaunch")}]: stop, press, floor it. Rolling [{K("RollingLaunch")}]: held at the rolling speed. Wheelie [{K("WheelieLaunch")}]. Burnout [{K("Burnout")}]: stopped. Press again to cancel.");
            Number("Launch rpm  (% of redline)", null, "LaunchRpmPercent", 30, 100, 5);
            Number("Hold  (seconds)", "How long the revs or speed are held before the launch.", "LaunchHoldSeconds", 0.5m, 15, 0.5m, 1);
            Number("Launch power  (% more torque)", null, "LaunchPower", 0, 500, 5);
            Number("Launch power lasts  (s)", null, "LaunchPowerSeconds", 1, 30, 1);
            Number("Rolling launch speed", null, "RollingMph", 10, 200, 5, 0, true);
            Number("Burnout  (seconds)", null, "BurnoutSeconds", 1, 30, 1);
            Card("Handling");
            Live("Drift", "Less rear grip: the tail slides out easily.", "Drift", d => d.Drift, (d, v) => d.Drift = v);
            Number("Drift rear grip  (x)", null, "DriftRearGrip", 0.1m, 3, 0.05m, 2);
            Live("More grip", "More grip on every tyre.", "Grip", d => d.Grip, (d, v) => d.Grip = v);
            Number("More grip  (x)", null, "GripBoost", 1.05m, 10, 0.05m, 2);
            Setting("Super brakes", "A hard brake stops much faster (never pushes backwards).", "SuperBrakes");
            Number("Super brakes  (extra g)", null, "SuperBrakeG", 0.5m, 10, 0.5m, 1);
            Card("Speed");
            Live("Cruise control", "Holds the speed you had when you switched it on while you are off the throttle. The brake cancels it.", "Cruise", d => d.Cruise, (d, v) => { d.Cruise = v; d.CruiseMs = d.SpeedMs; });
            Live("Speed limiter", null, "Limiter", d => d.Limiter, (d, v) => d.Limiter = v);
            Number("Limit", null, "LimiterMph", 5, 300, 5, 0, true);
        }
        void StuntsTab()
        {
            Card("Stunts");
            Note($"At speed: [{K("BarrelRoll")}] barrel roll, [{K("Spin360")}] 360, [{K("Backflip")}] backflip, [{K("Rocket")}] or LS rocket. Any time: [{K("Jump")}] jump, [{K("Stop")}] instant stop, [{K("Flip")}] flip upright.");
            Number("Jump height  (m)", null, "JumpMetres", 1, 20, 0.5m, 1);
            Number("Rocket adds", null, "RocketMph", 5, 130, 5, 0, true);
            Live("Wheelie mode", "Keeps the nose up at the wheelie angle.", "Wheelie", d => d.Wheelie, (d, v) => d.Wheelie = v);
            Number("Wheelie angle  (degrees)", "Also used by the wheelie launch.", "WheelieDegrees", 5, 40, 1);
            Card("Air");
            Live("Air control", "In the air, the left stick or the arrow keys pitch and roll the car.", "AirControl", d => d.Air, (d, v) => d.Air = v);
            Number("Air control rate  (rad/s)", null, "AirRate", 0.5m, 8, 0.5m, 1);
            Live("Low gravity", "Falls slower.", "LowGravity", d => d.LowGravity, (d, v) => d.LowGravity = v);
            Number("Gravity  (%)", null, "GravityPercent", 10, 90, 5);
            Card("UFO");
            Live("UFO mode", $"Fly: throttle and brake forward and back, steer to turn, [{K("UfoUp")}] / RB up, [{K("UfoDown")}] / LB down.", "Ufo", d => d.Ufo, (d, v) => d.Ufo = v);
            Number("Speed", "Up to 1000 mph (1609 km/h).", "UfoMph", 10, 1000, 10, 0, true);
            Number("Climb", null, "UfoClimbMph", 5, 300, 5, 0, true);
            Card("Ghost");
            Live("Ghost mode", "Drive through traffic and other cars. Turn it off before races. (FH2 Mods' Ghost cars option keeps you ghosted anyway.)", "Ghost", d => d.Ghost, (d, v) => d.Ghost = v);
        }
        void ModesTab()
        {
            Card("Game modes", true);
            Note($"[{K("GameMode")}] in game goes to the next mode: off, speed bomb, score attack, pursuit. Or start one here; it begins when you are back in the game. All off stops it.");
            var now = new Label { AutoSize = true, ForeColor = Ink, Font = new Font("Bahnschrift SemiBold", 10) };
            refreshers.Add(() => { var d = driving(); now.Text = d == null ? "game not connected" : d.Modes.Mode == Modes.Kind.Off ? (d.Modes.Result == "" ? "no mode running" : "last: " + d.Modes.Result) : "running: " + Modes.Names[(int)d.Modes.Mode]; });
            refreshers.Last()(); Put(now, 24);
            Buttons(Btn("Speed bomb", () => request("ModeBomb")), Btn("Score attack", () => request("ModeScore")), Btn("Pursuit", () => request("ModePursuit")), Btn("Stop", () => request("ModeStop")));
            Card("Speed bomb");
            Note("Reach the speed to arm it, then never drop below it for more than half a second. It gets 5 mph faster every minute.");
            Number("Bomb speed", null, "BombMph", 20, 250, 5, 0, true);
            Card("Score attack");
            Note("Points for speed over 60 mph, drifts, air and near misses. New skills raise the multiplier (up to x5); the chain banks after 3 s without one. A crash loses the chain.");
            Number("Length  (seconds)", null, "ScoreSeconds", 30, 900, 30);
            Card("Pursuit");
            Note("Cars of the world (drivatars first) are dropped on the road right behind you and chase you along the roads you drove. Stop next to one for 2.5 s and you are busted; 3 hits and you are wrecked. Stay 250 m away for 8 s to escape: then they come faster, and more of them (up to 3). Ghosting is off during a pursuit. Free roam with traffic on. Experimental.");
            Number("Chaser top speed", null, "ChaserMph", 50, 400, 10, 0, true);
            Number("Chaser height  (m)", "Drivatar chasers are held on the road you drove. Raise this if they sink into it, lower it if they float. Changes at once.", "ChaserHeight", -1, 1, 0.05m, 2);
            Card("Best");
            var best = new Label { AutoSize = true, ForeColor = Ink, Font = new Font("Consolas", 9.5f) };
            refreshers.Add(() => best.Text = $"speed bomb    {s.BombBest:0.0} s\nscore attack  {s.ScoreBest:#,0}\npursuit       {s.PursuitBest:0.0} s");
            refreshers.Last()(); Put(best, 56);
            Buttons(Btn("Reset", () => { s.BombBest = s.ScoreBest = s.PursuitBest = 0; s.Save(); }));
        }
        void TimersTab()
        {
            Card("Run timers");
            Note($"Stop, then go: the clock starts as the car moves. The runs follow the unit switch ({(s.Kmh ? "0-100 to 0-400 km/h, 100 to 2000 m" : "0-60 to 0-200 mph, 1/8 to 1 mile")}).");
            Setting("Timer card", null, "ShowTimer");
            var best = new Label { AutoSize = true, ForeColor = Ink, Font = new Font("Consolas", 9.5f) };
            refreshers.Add(() => { lock (runs) { runs.Kmh = s.Kmh; best.Text = string.Join("\n", runs.Names.Select(n => $"{n,-11} {(runs.Best.TryGetValue(n, out float t) ? t.ToString("0.000") + " s" : "-")}")); } });
            refreshers.Last()(); Put(best, 130);
            Buttons(Btn("Reset best times", () => { lock (runs) runs.Best.Clear(); s.BestTimes = ""; s.Save(); }));
            Card("Drag");
            Setting("Light tree", "Stop for a second: three ambers, then green. Leaving before green is a foul start.", "DragTree");
            Setting("Pro tree", "All ambers together, green 0.4 s later.", "ProTree");
            var last = new Label { AutoSize = true, ForeColor = Ink, Font = new Font("Consolas", 9) };
            refreshers.Add(() => { lock (runs) last.Text = runs.History.Count == 0 ? "no runs yet" : string.Join("\n", runs.History); });
            refreshers.Last()(); Put(last, 160);
        }
        void TeleportTab()
        {
            Card("Teleport", true);
            Note($"[{K("SaveSpot")}] saves where you are (up to 9), [{K("Teleport")}] goes to the selected spot. Free roam only. Save and Go here happen when you are back in the game.");
            var list = new ListBox { Width = card.Width - 24, Height = 190, BackColor = Field, ForeColor = Ink, BorderStyle = BorderStyle.None, Font = new Font("Consolas", 10) };
            void Fill()
            {
                var d = driving(); if (d == null) return;
                var items = d.SpotList.Select((p, i) => $"Spot {i + 1}    x {p[0],8:0}    z {p[2],8:0}    height {p[1],5:0} m").ToArray();
                if (!items.SequenceEqual(list.Items.Cast<string>())) { list.Items.Clear(); list.Items.AddRange(items); }
                if (d.SelectedSpot < list.Items.Count && list.SelectedIndex != d.SelectedSpot) list.SelectedIndex = d.SelectedSpot;
            }
            list.SelectedIndexChanged += (o, e) => { var d = driving(); if (d != null && list.SelectedIndex >= 0) d.SelectedSpot = list.SelectedIndex; };
            Fill(); refreshers.Add(Fill); Put(list);
            Buttons(Btn("Save spot here", () => request("SaveSpot")), Btn("Go", () => request("Teleport")), Btn("Delete", () => request("DeleteSpot")));
        }
        void GaugesTab()
        {
            Card("On screen");
            Setting("Speed and rev panel", null, "ShowSpeedo");
            Setting("Shift flash", "The rev bar flashes near the redline.", "ShowShiftLight");
            Setting("Nitrous strip", null, "ShowNitrous");
            Setting("Readouts", "Compass, g-force, drift angle and best drift, trip distance, top speed.", "ShowStats");
            Setting("Status card", "What is switched on, and any problem.", "ShowStatus");
            Card("Layout");
            Number("Size  (x)", null, "HudScale", 0.5m, 3, 0.1m, 1);
            Switch("Move gauges", "Drag a gauge to move it, scroll on it to resize. Switch off when done.", () => hud.Moving, v => hud.SetMoving(v));
            Buttons(Btn("Default positions", () => { s.SpeedoX = s.SpeedoY = s.CardX = s.CardY = -1; s.HudScale = 1; }));
            Card("Colours");
            Buttons(Btn("Accent colour", () => { using (var c = new ColorDialog { Color = Accent, FullOpen = true }) if (c.ShowDialog(this) == DialogResult.OK) { s.Accent = ColorTranslator.ToHtml(c.Color); Open("Gauges"); } }),
                    Btn("Default", () => { s.Accent = "#22D3EE"; Open("Gauges"); }));
            Number("Menu opacity  (%)", null, "MenuOpacity", 50, 100, 5);
            refreshers.Add(() => Opacity = s.MenuOpacity / 100.0);
        }
        void KeysTab()
        {
            Card("Keys", true);
            Note("Click a key, then press the new one (Esc = none). Keys act only while the game is in front, and not with Ctrl or Alt held, so FH2 Mods' Ctrl keys stay free.");
            int half = (card.Width - 36) / 2, i = 0, top = cardY;
            foreach (var (name, _, help) in Actions.All)
            {
                var p = new Panel { Width = half, Height = 28, Location = new Point(12 + (i % 2) * (half + 12), top + (i / 2) * 30) };
                p.Controls.Add(new Label { Text = char.ToUpper(help[0]) + help.Substring(1), AutoSize = false, Width = half - 100, Height = 28, TextAlign = ContentAlignment.MiddleLeft });
                var b = new Button { Width = 94, Height = 22, Location = new Point(half - 96, 3), FlatStyle = FlatStyle.Flat, BackColor = Field, ForeColor = Ink, Cursor = Cursors.Hand, Font = new Font("Bahnschrift", 9) };
                b.FlatAppearance.BorderColor = Field;
                void Show() { b.Text = capture == name ? "press a key" : K(name); b.FlatAppearance.BorderColor = capture == name ? Accent : Field; }
                b.Click += (o, e) => { capture = name; foreach (var r in refreshers) r(); };
                b.PreviewKeyDown += (o, e) => e.IsInputKey = true;
                b.KeyDown += (o, e) =>
                {
                    if (capture != name) return;
                    e.Handled = e.SuppressKeyPress = true;
                    var k = e.KeyCode;
                    if (k == Keys.ControlKey || k == Keys.ShiftKey || k == Keys.Menu) return;
                    if (k != Keys.Escape) foreach (var other in s.Bind.Keys.ToList()) if (s.Bind[other] == k) s.Bind[other] = Keys.None;   // one key, one action
                    s.Bind[name] = k == Keys.Escape ? Keys.None : k; capture = null; s.Save();
                    foreach (var r in refreshers) r();
                };
                Show(); refreshers.Add(Show); p.Controls.Add(b); card.Controls.Add(p);
                i++;
            }
            cardY = top + (i + 1) / 2 * 30 + 6; card.Height = cardY + 8;
            Buttons(Btn("Default keys", () => { foreach (var a in Actions.All) s.Bind[a.name] = a.key; s.Save(); Open("Keys"); }));
        }
        void AboutTab()
        {
            Card("FH2 Mods Overlay", true);
            Note("Driving effects for Forza Horizon 2 1.0.0.64 on Azimuth, part of FH2 Mods. They change only your own car, only while the overlay runs, and are put back when you pause, open the map or a menu, change car, press All off, or close it.");
            Note("Single-player only: do not use it in online sessions with other players.");
            Note("Packs (Race+ engine parts, engine swaps, traffic cars for sale, free cars) are in the FH2 Mods window: they change the game files, so they need Apply and a game restart.");
            Note("The game must run in windowed or borderless mode for the menu and gauges to show over it.");
            Buttons(Btn("Close the overlay", () => request("Exit")));
        }

        public void ShowOver(IntPtr game)
        {
            if (game != IntPtr.Zero && GetWindowRect(game, out RECT r)) Location = new Point(r.L + Math.Max(20, (r.R - r.L - Width) / 2), r.T + Math.Max(40, (r.B - r.T - Height) / 2));
            Show(); Activate(); Open(current ?? "Driving");
        }
        [StructLayout(LayoutKind.Sequential)] struct RECT { public int L, T, R, B; }
        [DllImport("user32")] static extern bool GetWindowRect(IntPtr w, out RECT r);
    }
}
