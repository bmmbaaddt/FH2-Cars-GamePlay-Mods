// The on-screen gauges: two small click-through, see-through windows over the game's picture (per-pixel alpha, so
// edges stay smooth): the cluster (speed, rev bar, nitrous, readouts) and the status and timer card. In move mode (from the
// menu) they take the mouse: drag one to move it, scroll on it to resize everything.
using System;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Drawing.Text;
using System.Linq;
using System.Runtime.InteropServices;
using System.Windows.Forms;

namespace FH2Overlay
{
    sealed class Layer : Form
    {
        public bool Moving;
        public Action<int, int> Dragged;   // new top-left, in screen pixels
        public Action<int> Scrolled;
        Point grab; bool dragging;

        public Layer()
        {
            FormBorderStyle = FormBorderStyle.None; ShowInTaskbar = false; StartPosition = FormStartPosition.Manual;
            MouseDown += (o, e) => { dragging = true; grab = e.Location; };
            MouseUp += (o, e) => dragging = false;
            MouseMove += (o, e) => { if (dragging) Dragged?.Invoke(Left + e.X - grab.X, Top + e.Y - grab.Y); };
            MouseWheel += (o, e) => Scrolled?.Invoke(Math.Sign(e.Delta));
        }
        protected override CreateParams CreateParams
        {
            get { var p = base.CreateParams; p.ExStyle |= 0x80000 | 0x80 | 0x8 | 0x8000000 | 0x20; return p; }   // layered, tool, topmost, no-activate, click-through
        }
        protected override bool ShowWithoutActivation => true;
        public void SetMoving(bool on)
        {
            Moving = on;
            int ex = GetWindowLong(Handle, -20);
            SetWindowLong(Handle, -20, on ? ex & ~0x20 : ex | 0x20);
        }

        [StructLayout(LayoutKind.Sequential)] struct PT { public int X, Y; }
        [StructLayout(LayoutKind.Sequential)] struct SZ { public int W, H; }
        [StructLayout(LayoutKind.Sequential, Pack = 1)] struct BLEND { public byte Op, Flags, Alpha, Format; }
        [DllImport("user32")] static extern int GetWindowLong(IntPtr w, int i);
        [DllImport("user32")] static extern int SetWindowLong(IntPtr w, int i, int v);
        [DllImport("user32")] static extern IntPtr GetDC(IntPtr w);
        [DllImport("user32")] static extern int ReleaseDC(IntPtr w, IntPtr dc);
        [DllImport("user32")] static extern bool UpdateLayeredWindow(IntPtr w, IntPtr dst, ref PT pos, ref SZ size, IntPtr src, ref PT srcPos, int key, ref BLEND blend, int flags);
        [DllImport("gdi32")] static extern IntPtr CreateCompatibleDC(IntPtr dc);
        [DllImport("gdi32")] static extern bool DeleteDC(IntPtr dc);
        [DllImport("gdi32")] static extern IntPtr SelectObject(IntPtr dc, IntPtr o);
        [DllImport("gdi32")] static extern bool DeleteObject(IntPtr o);

        // draws `paint` into a w x h picture shown at screen x, y
        public void Render(int x, int y, int w, int h, Action<Graphics> paint)
        {
            if (w < 1 || h < 1) { if (Visible) Hide(); return; }
            if (!Visible) Show();
            using (var bmp = new Bitmap(w, h, PixelFormat.Format32bppPArgb))
            {
                using (var g = Graphics.FromImage(bmp))
                {
                    g.SmoothingMode = SmoothingMode.AntiAlias; g.TextRenderingHint = TextRenderingHint.AntiAliasGridFit;
                    if (Moving) using (var b = new SolidBrush(Color.FromArgb(70, 255, 255, 255))) g.FillRectangle(b, 0, 0, w, h);
                    paint(g);
                }
                IntPtr screen = GetDC(IntPtr.Zero), mem = CreateCompatibleDC(screen), hb = bmp.GetHbitmap(Color.FromArgb(0)), old = SelectObject(mem, hb);
                var pos = new PT { X = x, Y = y }; var size = new SZ { W = w, H = h }; var zero = new PT();
                var blend = new BLEND { Op = 0, Alpha = 255, Format = 1 };
                UpdateLayeredWindow(Handle, screen, ref pos, ref size, mem, ref zero, 0, ref blend, 2);
                SelectObject(mem, old); DeleteObject(hb); DeleteDC(mem); ReleaseDC(IntPtr.Zero, screen);
            }
        }
    }

    sealed class Hud
    {
        readonly Settings s;
        readonly Func<Driving> driving;
        readonly Runs runs;
        readonly Layer speedo = new Layer(), card = new Layer();
        public Func<IntPtr> GameWindow = () => IntPtr.Zero;
        public Func<bool> MenuOpen = () => false;
        public bool Moving { get; private set; }
        Point origin;   // the game picture's top-left on screen

        [StructLayout(LayoutKind.Sequential)] struct RECT { public int L, T, R, B; }
        [StructLayout(LayoutKind.Sequential)] struct PT { public int X, Y; }
        [DllImport("user32")] static extern bool GetClientRect(IntPtr w, out RECT r);
        [DllImport("user32")] static extern bool ClientToScreen(IntPtr w, ref PT p);
        [DllImport("user32")] static extern bool IsIconic(IntPtr w);
        [DllImport("user32")] static extern IntPtr GetForegroundWindow();

        public Hud(Settings s, Func<Driving> driving, Runs runs)
        {
            this.s = s; this.driving = driving; this.runs = runs;
            speedo.Dragged = (x, y) => { s.SpeedoX = x - origin.X; s.SpeedoY = y - origin.Y; };
            card.Dragged = (x, y) => { s.CardX = x - origin.X; s.CardY = y - origin.Y; };
            speedo.Scrolled = card.Scrolled = d => s.HudScale = V.Clamp(s.HudScale + d * 0.1f, 0.5f, 3);
            var t = new Timer { Interval = 33 }; t.Tick += (o, e) => Frame(); t.Start();
        }

        public void SetMoving(bool on) { Moving = on; speedo.SetMoving(on); card.SetMoving(on); if (!on) s.Save(); }

        void Frame()
        {
            var game = GameWindow();
            bool show = game != IntPtr.Zero && !IsIconic(game) && (GetForegroundWindow() == game || MenuOpen() || Moving)
                        && GetClientRect(game, out RECT r) && r.R >= 200 && r.B >= 150;
            if (!show) { speedo.Hide(); card.Hide(); return; }
            GetClientRect(game, out RECT c);
            var at = new PT(); ClientToScreen(game, ref at); origin = new Point(at.X, at.Y);
            int w = c.R, h = c.B;
            var d = driving();
            float k = s.HudScale * Math.Max(0.6f, h / 1080f);

            // the cluster: bottom centre unless moved
            int cw = (int)(400 * k), ch = (int)((s.ShowSpeedo ? 112 : 0) * k + (s.ShowNitrous ? 18 * k : 0) + (s.ShowStats ? 24 * k : 0) + 10 * k);
            bool showCluster = (s.ShowSpeedo || s.ShowNitrous || s.ShowStats) && (Moving || d != null && d.InCar);
            int sx = s.SpeedoX >= 0 ? s.SpeedoX : (w - cw) / 2, sy = s.SpeedoY >= 0 ? s.SpeedoY : h - ch - (int)(28 * k);
            sx = Math.Max(0, Math.Min(w - cw, sx)); sy = Math.Max(0, Math.Min(h - ch, sy));
            if (showCluster) speedo.Render(origin.X + sx, origin.Y + sy, cw, ch, g => Cluster(g, d, cw, ch, k));
            else speedo.Hide();

            // the card: top right unless moved
            List<(string text, Color color, float size)> lines;
            lock (runs) lines = Lines(d);
            int tree = s.ShowTimer && runs.Tree ? (int)(34 * k) : 0;
            int kw = (int)(320 * k), kh = (int)(16 * k + tree + lines.Sum(l => (l.size + 7) * k));
            int kx = s.CardX >= 0 ? s.CardX : w - kw - (int)(30 * k), ky = s.CardY >= 0 ? s.CardY : (int)(30 * k);
            kx = Math.Max(0, Math.Min(w - kw, kx)); ky = Math.Max(0, Math.Min(h - kh, ky));
            if (lines.Count > 0 || tree > 0) card.Render(origin.X + kx, origin.Y + ky, kw, kh, g =>
            {
                using (var p = Round(new Rectangle(0, 0, kw - 1, kh - 1), 6 * k)) using (var b = new SolidBrush(Panel)) g.FillPath(b, p);
                using (var b = new SolidBrush(Accent)) g.FillRectangle(b, 8 * k, 0, kw - 16 * k, 2 * k);
                float y = 9 * k;
                if (tree > 0) { Tree(g, 12 * k, y, k); y += tree; }
                foreach (var (text, color, size) in lines)
                    using (var f = new Font(size >= 13 ? "Bahnschrift SemiBold" : "Bahnschrift", size * k, GraphicsUnit.Pixel)) using (var b = new SolidBrush(color))
                    {
                        g.DrawString(text, f, b, new RectangleF(12 * k, y, kw - 20 * k, (size + 7) * k), new StringFormat { Trimming = StringTrimming.EllipsisCharacter, FormatFlags = StringFormatFlags.NoWrap });
                        y += (size + 7) * k;
                    }
            });
            else card.Hide();
        }

        List<(string text, Color color, float size)> Lines(Driving d)
        {
            var lines = new List<(string text, Color color, float size)>();
            if (s.ShowStatus && d != null)
            {
                if (!string.IsNullOrEmpty(d.Status) && d.Status != "paused") lines.Add((d.Status.ToUpperInvariant(), Accent, 11));
                if (!string.IsNullOrEmpty(d.Error)) lines.Add((d.Error, Color.FromArgb(255, 255, 170, 90), 10));
            }
            if (s.ShowTimer && (runs.Running || runs.Last.Count > 0))
            {
                lines.Add(((runs.Running ? "" : "LAST RUN   ") + $"{runs.Elapsed:0.00} s   top {(s.Kmh ? runs.TopMph * Units.KmPerMile : runs.TopMph):0} {Units.Name(s.Kmh)}", Ink, 13));
                foreach (var name in runs.Names)
                    if (runs.Last.TryGetValue(name, out float t))
                    {
                        bool best = runs.Best.TryGetValue(name, out float b) && b == t;
                        lines.Add(($"{name}   {t:0.000} s" + (runs.Best.ContainsKey(name) && !best ? $"   best {b:0.000}" : best ? "   \u2605 best" : ""), best ? Accent : Dim, 10));
                    }
            }
            if (s.ShowTimer && !float.IsNaN(runs.Reaction))
                lines.Add((runs.Lights == -1 ? $"FOUL START  {-runs.Reaction:0.000} s early" : $"reaction {runs.Reaction:0.000} s", runs.Lights == -1 ? Color.FromArgb(255, 255, 170, 90) : Ink, 10));
            if (s.ShowTimer && runs.Message != "") lines.Add((runs.Message, Accent, 10));
            if (Moving && lines.Count == 0) lines.Add(("status and timer card", Ink, 11));
            return lines;
        }

        Color Accent { get { try { return ColorTranslator.FromHtml(s.Accent); } catch { return Color.DeepSkyBlue; } } }
        static readonly Color Panel = Color.FromArgb(190, 10, 16, 24), Ink = Color.FromArgb(240, 244, 248), Dim = Color.FromArgb(170, 150, 165, 180),
                              Off = Color.FromArgb(55, 255, 255, 255), Amber = Color.FromArgb(255, 255, 190, 60);

        // wide digital panel: rev bar across the top, speed on the left, rpm and cruise or drift on the right,
        // nitrous strip and readouts below
        void Cluster(Graphics g, Driving d, int w, int h, float k)
        {
            using (var p = Round(new Rectangle(0, 0, w - 1, h - 1), 6 * k)) using (var b = new SolidBrush(Panel)) g.FillPath(b, p);
            using (var b = new SolidBrush(Accent)) g.FillRectangle(b, 8 * k, h - 2 * k, w - 16 * k, 2 * k);
            float y = 8 * k;
            if (s.ShowSpeedo)
            {
                float frac = d != null && d.RedlineRpm > 0 ? Math.Min(1, d.Rpm / d.RedlineRpm) : 0;
                bool shift = s.ShowShiftLight && frac > 0.92f;
                const int n = 30;
                float seg = (w - 24 * k) / n;
                for (int i = 0; i < n; i++)
                {
                    bool lit = (i + 0.5f) / n <= frac;
                    Color col = !lit ? Off : shift ? (Environment.TickCount % 200 < 100 ? Accent : Ink) : i >= n * 0.85f ? Amber : Accent;
                    using (var b = new SolidBrush(col)) g.FillRectangle(b, 12 * k + i * seg, y, seg - 2 * k, 10 * k);
                }
                y += 16 * k;
                string speed = d == null ? "0" : Units.Show(d.SpeedMs, s.Kmh).ToString("0");
                using (var f = new Font("Bahnschrift SemiBold", 62 * k, GraphicsUnit.Pixel)) using (var b = new SolidBrush(Ink))
                using (var u = new Font("Bahnschrift", 14 * k, GraphicsUnit.Pixel)) using (var ub = new SolidBrush(Accent))
                {
                    var sz = g.MeasureString(speed, f);
                    g.DrawString(speed, f, b, 16 * k, y - 4 * k);
                    g.DrawString(Units.Name(s.Kmh).ToUpperInvariant(), u, ub, 16 * k + sz.Width - 6 * k, y + 52 * k);
                }
                using (var f = new Font("Bahnschrift", 13 * k, GraphicsUnit.Pixel)) using (var big = new Font("Bahnschrift SemiBold", 24 * k, GraphicsUnit.Pixel))
                using (var dim = new SolidBrush(Dim)) using (var ink = new SolidBrush(Ink)) using (var acc = new SolidBrush(Accent)) using (var amb = new SolidBrush(Amber))
                {
                    float rx = w * 0.56f;
                    g.DrawString("RPM", f, dim, rx, y + 4 * k);
                    g.DrawString(d == null ? "-" : d.Rpm.ToString("0"), big, ink, rx, y + 20 * k);
                    if (d != null && d.Cruise) g.DrawString("CRUISE " + Units.Speed(d.CruiseMs, s.Kmh), f, acc, rx, y + 56 * k);
                    else if (d != null && d.DriftChain > 0) g.DrawString($"DRIFT  {d.DriftChain:#,0}", f, amb, rx, y + 56 * k);
                    else if (d != null && d.NitrousOn) g.DrawString("NITROUS", f, acc, rx, y + 56 * k);
                }
                y += 88 * k;
            }
            if (s.ShowNitrous)
            {
                float fill = d?.Tank ?? 1; bool firing = d?.NitrousOn ?? false;
                var r = new RectangleF(12 * k, y, w - 24 * k, 8 * k);
                using (var b = new SolidBrush(Off)) g.FillRectangle(b, r);
                using (var b = new SolidBrush(firing ? Ink : Color.FromArgb(220, 120, 140, 255))) g.FillRectangle(b, r.X, r.Y, r.Width * fill, r.Height);
                y += 18 * k;
            }
            if (s.ShowStats && d != null)
                using (var f = new Font("Bahnschrift", 12 * k, GraphicsUnit.Pixel)) using (var b = new SolidBrush(Dim))
                {
                    string dir = "N NE E SE S SW W NW".Split(' ')[(int)((d.Heading + 22.5f) % 360 / 45)];
                    string text = $"{dir} {d.Heading:000}\u00b0    G {d.GLong:+0.0;-0.0} / {Math.Abs(d.GLat):0.0}    drift {d.DriftAngle:0}\u00b0  best {d.DriftBest:#,0}    {Units.Distance(d.TripMetres, s.Kmh)}    top {Units.Speed(d.TopMs, s.Kmh)}";
                    g.DrawString(text, f, b, new RectangleF(12 * k, y, w - 20 * k, 20 * k), new StringFormat { Trimming = StringTrimming.EllipsisCharacter, FormatFlags = StringFormatFlags.NoWrap });
                }
        }

        void Tree(Graphics g, float x, float y, float k)
        {
            for (int i = 0; i < 4; i++)
            {
                bool lit = runs.Lights == -1 ? i == 3 : i < 3 ? runs.Lights > i : runs.Lights == 4;
                Color on = runs.Lights == -1 && i == 3 ? Color.FromArgb(255, 255, 80, 60) : i < 3 ? Amber : Color.FromArgb(255, 40, 230, 120);
                using (var b = new SolidBrush(lit ? on : Off)) g.FillRectangle(b, x + i * 34 * k, y + 2 * k, 28 * k, 20 * k);
            }
        }

        static GraphicsPath Round(Rectangle r, float radius)
        {
            var p = new GraphicsPath(); float d = Math.Max(1, radius * 2);
            p.AddArc(r.X, r.Y, d, d, 180, 90); p.AddArc(r.Right - d, r.Y, d, d, 270, 90);
            p.AddArc(r.Right - d, r.Bottom - d, d, d, 0, 90); p.AddArc(r.X, r.Bottom - d, d, d, 90, 90);
            p.CloseFigure(); return p;
        }
    }
}
