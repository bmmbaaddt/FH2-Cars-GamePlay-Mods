// Vector maths and the motion of every driving effect. World y is up; a body's orientation is three rows:
// right, up and forward. All functions are pure, so Motion.Test() checks them without the game.
using System;

namespace FH2Overlay
{
    static class V
    {
        public static float[] From(byte[] b, int at = 0)
        {
            var v = new[] { BitConverter.ToSingle(b, at), BitConverter.ToSingle(b, at + 4), BitConverter.ToSingle(b, at + 8) };
            if (Array.Exists(v, x => float.IsNaN(x) || float.IsInfinity(x))) throw new Exception("bad vector");
            return v;
        }
        public static byte[] Bytes(float[] v)
        {
            var b = new byte[12];
            for (int i = 0; i < 3; i++) Buffer.BlockCopy(BitConverter.GetBytes(v[i]), 0, b, i * 4, 4);
            return b;
        }
        public static float Dot(float[] a, float[] b) => a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
        public static float Len(float[] v) => (float)Math.Sqrt(Dot(v, v));
        public static float[] Add(float[] a, float[] b) => new[] { a[0] + b[0], a[1] + b[1], a[2] + b[2] };
        public static float[] Sub(float[] a, float[] b) => new[] { a[0] - b[0], a[1] - b[1], a[2] - b[2] };
        public static float[] Mul(float[] v, float s) => new[] { v[0] * s, v[1] * s, v[2] * s };
        public static float[] Cross(float[] a, float[] b) => new[] { a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };
        public static float[] Unit(float[] v) { float l = Len(v); return l < 1e-4f ? new float[3] : Mul(v, 1 / l); }
        public static float Clamp(float x, float lo, float hi) => float.IsNaN(x) ? 0 : Math.Max(lo, Math.Min(hi, x));
    }

    // speeds are kept in m/s (game) and mph (settings); shown in mph or km/h
    static class Units
    {
        public const float KmPerMile = 1.609344f;
        public static float Show(float ms, bool kmh) => kmh ? ms * 3.6f : ms / Motion.MsPerMph;
        public static string Name(bool kmh) => kmh ? "km/h" : "mph";
        public static string Speed(float ms, bool kmh) => Show(ms, kmh).ToString("0") + " " + Name(kmh);
        public static string Distance(float metres, bool kmh) => kmh ? (metres / 1000).ToString("0.0") + " km" : (metres / 1609.344f).ToString("0.0") + " mi";
    }

    static class Motion
    {
        public const float G = 9.81f, MsPerMph = 0.44704f;
        static readonly float[] Up = { 0, 1, 0 };
        const float StuntSpin = 5.712f;   // rad/s: one full turn in about 1.1 s

        // speed cap on the ground plane; falling and jumping are left alone
        public static float[] Limit(float[] v, float maxMs)
        {
            float flat = (float)Math.Sqrt(v[0] * v[0] + v[2] * v[2]);
            return flat <= maxMs ? v : new[] { v[0] * maxMs / flat, v[1], v[2] * maxMs / flat };
        }

        public static float[] Jump(float[] v, float metres) => new[] { v[0], Math.Max(v[1], (float)Math.Sqrt(2 * G * metres)), v[2] };

        // takes up to `delta` m/s of forward speed away, never reversing the car
        public static float[] Brake(float[] v, float[] forward, float delta)
        {
            float flat = (float)Math.Sqrt(v[0] * v[0] + v[2] * v[2]);
            if (flat < 0.5f || v[0] * forward[0] + v[2] * forward[2] <= 0) return v;
            float k = Math.Max(0, flat - delta) / flat;
            return new[] { v[0] * k, v[1], v[2] * k };
        }

        // low gravity: part of each tick's new downward speed is taken back (capped at 0.1 g per tick)
        public static float[] SlowFall(float[] v, float? previousVy, float gravityPercent)
        {
            if (previousVy == null || v[1] >= 0) return v;
            float gained = Math.Min(previousVy.Value - v[1], 0.1f * G);
            if (gained <= 0) return v;
            return new[] { v[0], Math.Min(0, v[1] + gained * (1 - gravityPercent / 100)), v[2] };
        }

        static float Pitch(float[][] rows) => (float)Math.Asin(V.Clamp(rows[2][1], -1, 1));

        // spin that brings the nose to `degrees` above level (pitch is about the car's right axis); null when overturned
        public static float[] Wheelie(float[][] rows, float[] spin, float degrees)
        {
            if (rows[1][1] < 0.5f) return null;
            float rate = V.Clamp(-3 * (degrees * (float)Math.PI / 180 - Pitch(rows)), -1.2f, 1.2f);
            return V.Add(spin, V.Mul(rows[0], rate - V.Dot(spin, rows[0])));
        }

        public enum Stunt { BarrelRoll, Spin360, Backflip }
        public static void DoStunt(float[][] rows, float[] v, Stunt s, out float[] velocity, out float[] spin)
        {
            velocity = (float[])v.Clone();
            velocity[1] = Math.Max(velocity[1], s == Stunt.BarrelRoll ? 6.5f : s == Stunt.Backflip ? 8f : 4.5f);
            spin = s == Stunt.BarrelRoll ? V.Mul(rows[2], StuntSpin)
                 : s == Stunt.Spin360 ? new[] { 0, StuntSpin, 0 }
                 : V.Mul(V.Unit(V.Cross(rows[2], rows[1])), StuntSpin);   // about the car's left: nose up and over
        }

        public static float[] Rocket(float[][] rows, float[] v, float ms) => V.Add(v, V.Mul(rows[2], ms));

        // in the air the stick sets pitch and roll rates; yaw is kept
        public static float[] AirControl(float[][] rows, float[] spin, float pitch, float roll, float rate)
        {
            var pitchAxis = V.Unit(V.Cross(rows[1], rows[2]));
            var rollAxis = V.Unit(V.Cross(rows[1], rows[0]));
            spin = V.Add(spin, V.Mul(pitchAxis, V.Clamp(pitch, -1, 1) * rate - V.Dot(spin, pitchAxis)));
            return V.Add(spin, V.Mul(rollAxis, V.Clamp(roll, -1, 1) * rate - V.Dot(spin, rollAxis)));
        }

        // UFO: hover, fly along the heading, climb, turn, and stay level
        public static void Fly(float[][] rows, float forward, float climb, float turn, float speed, float climbSpeed, float turnRate, out float[] velocity, out float[] spin)
        {
            var heading = V.Unit(new[] { rows[2][0], 0, rows[2][2] });
            var side = V.Unit(new[] { rows[0][0], 0, rows[0][2] });
            if (V.Len(heading) < 0.5f) heading = V.Unit(V.Cross(side, Up));
            velocity = V.Add(V.Mul(heading, V.Clamp(forward, -1, 1) * speed), new[] { 0, V.Clamp(climb, -1, 1) * climbSpeed, 0 });
            spin = V.Add(V.Mul(V.Unit(V.Cross(heading, side)), V.Clamp(turn, -1, 1) * turnRate), V.Mul(V.Cross(rows[1], Up), 4));
            if (V.Len(spin) > 8) spin = V.Mul(V.Unit(spin), 8);
        }

        public static void Test()
        {
            void Ok(bool c, string what) { if (!c) throw new Exception("motion test failed: " + what); }
            float[][] level = { new float[] { 1, 0, 0 }, new float[] { 0, 1, 0 }, new float[] { 0, 0, 1 } };
            var v = Limit(new float[] { 40, -3, 30 }, 26.8224f);
            Ok(Math.Abs(Math.Sqrt(v[0] * v[0] + v[2] * v[2]) - 26.8224) < 1e-3 && v[1] == -3, "limit keeps vertical, caps flat speed");
            Ok(Limit(new float[] { 1, 2, 3 }, 26)[0] == 1, "limit never speeds up");
            Ok(Math.Abs(Jump(new float[3], 3)[1] * Jump(new float[3], 3)[1] / (2 * G) - 3) < 1e-3, "jump height");
            Ok(Math.Abs(Brake(new float[] { 0, -1, 20 }, level[2], 2)[2] - 18) < 1e-4, "brake slows forward");
            Ok(Brake(new float[] { 0, 0, -10 }, level[2], 2)[2] == -10, "brake leaves reversing");
            Ok(Brake(new float[] { 0, 0, 1 }, level[2], 5)[2] == 0, "brake stops at zero");
            Ok(Math.Abs(SlowFall(new float[] { 3, -2, 4 }, -1.5f, 50)[1] + 1.75f) < 1e-4, "slow fall");
            Ok(SlowFall(new float[] { 0, 2, 0 }, 1, 50)[1] == 2, "slow fall leaves rising");
            var w = Wheelie(level, new float[] { 0, 0.3f, 0.2f }, 20);
            Ok(w[0] < 0 && w[1] == 0.3f && w[2] == 0.2f, "wheelie lifts the nose, keeps yaw and roll");
            Ok(Wheelie(new[] { level[0], new float[] { 0, -1, 0 }, level[2] }, new float[3], 20) == null, "no wheelie upside down");
            DoStunt(level, new float[] { 5, -1, 20 }, Stunt.Backflip, out var nv, out var ns);
            Ok(nv[1] == 8 && nv[2] == 20 && V.Dot(V.Cross(ns, level[2]), level[1]) > 0, "backflip lifts the nose over");
            DoStunt(level, new float[] { 5, -1, 20 }, Stunt.Spin360, out nv, out ns);
            Ok(ns[1] > 5 && ns[0] == 0, "360 about vertical");
            Ok(Rocket(level, new float[] { 5, -1, 20 }, 15)[2] == 35, "rocket along forward");
            var a = AirControl(level, new float[] { 0, 0.7f, 3 }, 1, 0, 2.5f);
            Ok(Math.Abs(a[1] - 0.7f) < 1e-4 && Math.Abs(V.Dot(a, level[2])) < 1e-4, "air control keeps yaw, zero roll");
            Fly(level, 0, 0, 0, 20, 10, 1.5f, out var fv, out var fs);
            Ok(V.Len(fv) == 0 && V.Len(fs) < 1e-5, "UFO hovers");
            Fly(level, 1, -0.5f, 0, 20, 10, 1.5f, out fv, out fs);
            Ok(Math.Abs(fv[2] - 20) < 1e-4 && Math.Abs(fv[1] + 5) < 1e-4, "UFO forward and climb");
            Fly(level, 0, 0, 1, 20, 10, 1.5f, out fv, out fs);
            Ok(V.Dot(V.Cross(fs, level[2]), level[0]) > 0, "UFO turns right");
        }
    }
}
