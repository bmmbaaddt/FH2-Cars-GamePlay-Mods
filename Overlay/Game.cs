// Reads and writes the running game (FH2 1.0.0.64 hosted by Azimuth; its image is found by the PE timestamp). Every write first checks that
// the player car is still the same object, so a car change, a loading screen or a menu never gets a stale write.
using System;
using System.Diagnostics;
using System.Runtime.InteropServices;

namespace FH2Overlay
{
    sealed class Car
    {
        public long Address, Sim, Owner;   // the car object, its rigid body (CCarDynamics) and its owner
        public int Model;                   // Data_Car id
        public float StockTorque;           // the torque scale the game set for this car
        public bool Same(Car o) => o != null && o.Address == Address && o.Sim == Sim && o.Model == Model;
    }

    sealed class Game : IDisposable
    {
        const uint Timestamp = 0x565501ed;   // FH2 1.0.0.64
        readonly long Image;
        long CarList => Image + 0x34aba60;         // -> +0x48 -> +0x98 -> car pointer vector at +0x458/+0x460
        long CarVtable => Image + 0x2c5cb80;
        long SimVtable => Image + 0x2eb5e18;
        long InputManager => Image + 0x365d7c8;   // the same controller manager the FH2 Mods ExtraOptions reads
        long InputVtable => Image + 0x2e7a2e8;
        long BackfireVtable => Image + 0x2e6b080;

        // car object
        const int CarModel = 0x118, CarStockTorque = 0x135c, CarSim = 0x4c08, CarOwner = 0x4d00, CarKind = 0x4d08, CarActive = 0x4d30, CarHidden = 0x530a;
        const int CarBackfire = 0x4e20, BackfireMode = 0x4e28, BackfireRequest = 0x4e2c;
        // rigid body: position, orientation (right, up, forward rows of 16 bytes), velocity, spin, angular momentum, inertia
        public const int Position = -0x390, Basis = -0x380, Velocity = -0x350, Spin = -0x340, Momentum = -0x330, Inertia = -0x320;
        const int Collision = -0x1bc;   // 2 = collides, 4 = ghost
        const int EngineSpeed = 0x148, Redline = 0x1c8, TorqueScale = 0xc4c, BrakeInput = 0xdfc, BrakeForce = 0xe20, Speed = 0xe28;
        const int TyreParams = 0xbc0, Tyres = 0x1640, TyreStride = 0x4b0, TyreLateralGrip = 12;
        static readonly int[] PoseCopies = { -0x390, 0xd30, 0xef0 };   // the body keeps its pose three times; teleport moves all

        // code the offsets above were read from; a different build is refused instead of written to
        static readonly (long rva, string bytes)[] Build = {
            (0x21e0c0, "48 8b 49 48 48 85 c9"), (0xf605c0, "48 8d 81 00 12 00 00 c3"), (0x12c9a54, "c5 72 59 b3 4c 0c 00 00"),
            (0x12e07d0, "c5 fa 11 89 4c 0c 00 00 c3"), (0x12c8158, "c5 fa 10 81 48 01 00 00"), (0x12e86a9, "4b 8b 84 01 40 16 00 00"),
            (0x12c65c8, "8b 83 fc 0d 00 00"), (0x12ddf45, "44 89 83 fc 0d 00 00"), (0x12e23be, "8b 87 20 0e 00 00"),
            (0x12e3cf4, "c5 f8 29 81 b0 fc ff ff"), (0x12e3d06, "c5 fa 11 89 28 0e 00 00"), (0x12dd924, "c5 f8 29 81 c0 fc ff ff"),
            (0x12dd92c, "48 81 c1 d0 fc ff ff"), (0x12d7270, "c5 e0 59 87 c0 fc ff ff"), (0x1257884, "c4 c1 7a 10 60 04") };
        static readonly (long rva, string bytes)[] BackfireCode = { (0xf7b54b, "39 af 2c 4e 00 00"), (0xf7d8ea, "83 79 08 00 0f 86"), (0xf7d8f7, "44 89 71 0c") };

        [DllImport("kernel32", SetLastError = true)] static extern IntPtr OpenProcess(uint access, bool inherit, int pid);
        [DllImport("kernel32", SetLastError = true)] static extern bool ReadProcessMemory(IntPtr p, IntPtr a, byte[] b, UIntPtr n, out UIntPtr done);
        [DllImport("kernel32", SetLastError = true)] static extern bool WriteProcessMemory(IntPtr p, IntPtr a, byte[] b, UIntPtr n, out UIntPtr done);
        [DllImport("kernel32")] static extern bool CloseHandle(IntPtr h);
        [StructLayout(LayoutKind.Sequential)] struct Region { public long Base, AllocBase; public uint AllocProtect; public uint Pad; public long Size; public uint State, Protect, Type, Pad2; }
        [DllImport("kernel32")] static extern IntPtr VirtualQueryEx(IntPtr p, IntPtr a, out Region r, IntPtr size);
        [DllImport("user32")] static extern IntPtr GetForegroundWindow();
        [DllImport("user32")] static extern uint GetWindowThreadProcessId(IntPtr w, out uint pid);

        readonly Process process;
        IntPtr handle;
        public readonly bool Backfires;
        public int Pid => process.Id;

        Game(Process p, IntPtr h, long image)
        {
            process = p; handle = h; Image = image;
            foreach (var (rva, bytes) in Build)
                if (!Matches(rva, bytes)) { Dispose(); throw new Exception("Unsupported game build: FH2 1.0.0.64 on Azimuth is needed."); }
            Backfires = Array.TrueForAll(BackfireCode, c => Matches(c.rva, c.bytes));
        }

        // null while the game is not running or still at Azimuth's launcher (its image not loaded yet)
        public static Game Attach()
        {
            var all = Process.GetProcessesByName("azimuth");
            if (all.Length == 0) return null;
            var h = OpenProcess(0x0438, false, all[0].Id);   // query, read, write, operation
            if (h == IntPtr.Zero)
                throw new Exception(Marshal.GetLastWin32Error() == 5 ? "Run this the same way as the game (both as admin, or neither)." : "Could not open the game.");
            long image = FindImage(h);
            if (image == 0) { CloseHandle(h); return null; }
            return new Game(all[0], h, image);
        }

        static long FindImage(IntPtr h)
        {
            long a = 0;
            while (a < 1L << 47 && VirtualQueryEx(h, new IntPtr(a), out Region r, (IntPtr)Marshal.SizeOf<Region>()) != IntPtr.Zero && r.Size > 0)
            {
                if (r.Base == r.AllocBase && r.State == 0x1000)
                {
                    var head = new byte[0x200];
                    if (ReadProcessMemory(h, new IntPtr(r.Base), head, (UIntPtr)0x200, out UIntPtr n) && head[0] == 'M' && head[1] == 'Z')
                    {
                        int pe = BitConverter.ToInt32(head, 0x3c);
                        if (pe > 0 && pe < 0x1f0 && BitConverter.ToUInt32(head, pe + 8) == Timestamp) return r.Base;
                    }
                }
                a = r.Base + r.Size;
            }
            return 0;
        }

        public bool Exited { get { try { return process.HasExited; } catch { return true; } } }
        public IntPtr Window { get { process.Refresh(); return process.MainWindowHandle; } }
        public bool Focused { get { GetWindowThreadProcessId(GetForegroundWindow(), out uint pid); return pid == (uint)process.Id; } }

        bool Matches(long rva, string hex)
        {
            try
            {
                var want = Array.ConvertAll(hex.Split(' '), h => Convert.ToByte(h, 16));
                var got = Read(Image + rva, want.Length);
                for (int i = 0; i < want.Length; i++) if (got[i] != want[i]) return false;
                return true;
            }
            catch { return false; }
        }

        public byte[] Read(long address, int count)
        {
            var b = new byte[count];
            if (!ReadProcessMemory(handle, new IntPtr(address), b, (UIntPtr)(ulong)count, out UIntPtr n) || (int)n != count)
                throw new Exception("read failed");
            return b;
        }
        public long Ptr(long a) => BitConverter.ToInt64(Read(a, 8), 0);
        public int Int(long a) => BitConverter.ToInt32(Read(a, 4), 0);
        public float Float(long a) => BitConverter.ToSingle(Read(a, 4), 0);
        void Write(long address, byte[] b)
        {
            if (!WriteProcessMemory(handle, new IntPtr(address), b, (UIntPtr)(ulong)b.Length, out UIntPtr n) || (int)n != b.Length)
                throw new Exception("write failed");
        }
        void WriteFloat(long a, float v) { if (float.IsNaN(v) || float.IsInfinity(v)) throw new Exception("bad value"); Write(a, BitConverter.GetBytes(v)); }

        // the cars of the world: the player's is the one owned locally, active and shown, with a valid body
        long[] Cars()
        {
            long root = Ptr(CarList); if (root == 0) return new long[0];
            long world = Ptr(root + 0x48); if (world == 0) return new long[0];
            long list = Ptr(world + 0x98); if (list == 0) return new long[0];
            long begin = Ptr(list + 0x458), end = Ptr(list + 0x460);
            if (end < begin || (end - begin) % 8 != 0 || (end - begin) / 8 > 128) return new long[0];
            var cars = new long[(end - begin) / 8];
            var raw = Read(begin, cars.Length * 8);
            for (int i = 0; i < cars.Length; i++) cars[i] = BitConverter.ToInt64(raw, i * 8);
            return cars;
        }

        public Car Player()
        {
            try
            {
                Car found = null;
                foreach (long c in Cars())
                {
                    if (c == 0 || Ptr(c) != CarVtable || Int(c + CarKind) != 0 || Int(c + CarActive) != 1 || Read(c + CarHidden, 1)[0] != 0) continue;
                    long owner = Ptr(c + CarOwner), sim = Ptr(c + CarSim);
                    if (owner == 0 || sim == 0 || Ptr(sim) != SimVtable) continue;
                    float stock = Float(c + CarStockTorque); int model = Int(c + CarModel);
                    if (!(stock > 0 && stock <= 100) || model <= 0) return null;
                    if (found != null) return null;   // two candidates: not sure which is ours, so none
                    found = new Car { Address = c, Sim = sim, Owner = owner, Model = model, StockTorque = stock };
                }
                return found;
            }
            catch { return null; }
        }

        public bool Valid(Car car)
        {
            try
            {
                return car != null && Ptr(car.Address) == CarVtable && Ptr(car.Address + CarSim) == car.Sim && Ptr(car.Sim) == SimVtable &&
                       Ptr(car.Address + CarOwner) == car.Owner && Int(car.Address + CarModel) == car.Model && Near(Float(car.Address + CarStockTorque), car.StockTorque);
            }
            catch { return false; }
        }
        void Check(Car car) { if (!Valid(car)) throw new Exception("the car changed"); }
        public static bool Near(float a, float b) => Math.Abs(a - b) <= Math.Max(1e-6f, Math.Abs(b) * 1e-6f);

        // driving, not in a menu, map or pause: the game's window is in front and its input goes to the car
        public bool Driving()
        {
            try
            {
                if (!Focused) return false;
                long root = Ptr(CarList); if (root == 0) return false;
                long world = Ptr(root + 0x48); if (world == 0 || Read(world + 0x28, 1)[0] != 0) return false;
                long pad = Pad(); if (pad == 0) return false;
                long ring = Ptr(pad + 0x40), size = Ptr(pad + 0x48), first = Ptr(pad + 0x50), count = Ptr(pad + 0x58);
                if (ring == 0 || size <= 0 || size > 4096 || (size & (size - 1)) != 0 || first < 0 || count <= 0 || count > size * 2) return false;
                long block = Ptr(ring + ((first / 2) & (size - 1)) * 8);
                return block != 0 && (Int(block + first % 2 * 8 + 4) & 1) != 0;   // the top input context is the car's
            }
            catch { return false; }
        }
        long Pad()
        {
            long m = Ptr(InputManager); if (m == 0 || Ptr(m) != InputVtable) return 0;
            long pad = Ptr(m + 0xab28), first = m + 0x10;
            if (pad < first || pad >= first + 0x9800 || (pad - first) % 0x1300 != 0 || Read(pad + 4, 1)[0] != 1) return 0;
            return pad;
        }
        // throttle and brake as the game sees them, from any controller or the keyboard
        public float Throttle() { try { long p = Pad(); return p == 0 ? 0 : Read(p + 0x71, 1)[0] / 255f; } catch { return 0; } }
        public float BrakePedal() { try { long p = Pad(); return p == 0 ? 0 : Read(p + 0x70, 1)[0] / 255f; } catch { return 0; } }

        public float SpeedMs(Car car) { Check(car); float v = Float(car.Sim + Speed); if (!(v >= 0 && v < 1500)) throw new Exception("bad speed"); return v; }
        public float Rpm(Car car) { Check(car); float r = Float(car.Sim + EngineSpeed) * 60 / (2 * (float)Math.PI); if (!(r >= 0 && r < 40000)) throw new Exception("bad rpm"); return r; }
        public float RedlineRpm(Car car) => Float(car.Sim + Redline) * 60 / (2 * (float)Math.PI);
        public void SetRpm(Car car, float rpm) { Check(car); if (!(rpm >= 500 && rpm <= 20000)) throw new Exception("bad rpm"); WriteFloat(car.Sim + EngineSpeed, rpm * 2 * (float)Math.PI / 60); }

        public float Torque(Car car) { Check(car); return Float(car.Sim + TorqueScale); }
        public void SetTorque(Car car, float scale) { Check(car); if (!(scale > 0 && scale <= 300)) throw new Exception("bad torque"); WriteFloat(car.Sim + TorqueScale, scale); }

        public float[] Brakes(Car car) { Check(car); return new[] { Float(car.Sim + BrakeInput), Float(car.Sim + BrakeForce) }; }
        public void SetBrakes(Car car, float input, float force)
        {
            Check(car);
            if (!(input >= 0 && input <= 1 && force >= 0 && force <= 1)) throw new Exception("bad brake");
            WriteFloat(car.Sim + BrakeInput, input); WriteFloat(car.Sim + BrakeForce, force);
        }

        public float[] Vec(Car car, int at) { Check(car); return V.From(Read(car.Sim + at, 12)); }
        public float[][] Rows(Car car, int at) { Check(car); var b = Read(car.Sim + at, 48); return new[] { V.From(b, 0), V.From(b, 16), V.From(b, 32) }; }
        public void SetVelocity(Car car, float[] v)
        {
            Check(car);
            if (V.Len(v) > 500) throw new Exception("speed out of range");
            Write(car.Sim + Velocity, V.Bytes(v)); WriteFloat(car.Sim + Speed, V.Len(v));
        }
        public void SetSpin(Car car, float[] w)
        {
            Check(car);
            if (V.Len(w) > 100) throw new Exception("spin out of range");
            var inertia = Rows(car, Inertia);   // the body integrates angular momentum, so it is set to match
            var l = new float[3];
            for (int i = 0; i < 3; i++) for (int j = 0; j < 3; j++) l[j] += w[i] * inertia[i][j];
            Write(car.Sim + Spin, V.Bytes(w)); Write(car.Sim + Momentum, V.Bytes(l));
        }

        public void Teleport(Car car, float[] pos, byte[] basis)
        {
            Check(car);
            if (pos.Length != 3 || Array.Exists(pos, x => float.IsNaN(x) || Math.Abs(x) > 50000)) throw new Exception("bad spot");
            var here = Vec(car, Position); var turn = Rows(car, Basis);
            var turned = new bool[PoseCopies.Length];   // a copy gets the new direction only where it holds the current one
            for (int i = 0; i < PoseCopies.Length; i++)
            {
                int at = PoseCopies[i];
                if (V.Len(V.Sub(V.From(Read(car.Sim + at, 12)), here)) > 0.05f) throw new Exception("car position layout not recognised");
                var b = Read(car.Sim + at + 16, 48);
                turned[i] = V.Len(V.Sub(V.From(b, 0), turn[0])) < 0.02f && V.Len(V.Sub(V.From(b, 16), turn[1])) < 0.02f && V.Len(V.Sub(V.From(b, 32), turn[2])) < 0.02f;
            }
            if (!turned[0]) throw new Exception("car direction layout not recognised");
            SetVelocity(car, new float[3]); SetSpin(car, new float[3]);
            var p = new[] { pos[0], pos[1] + 0.4f, pos[2] };
            for (int i = 0; i < PoseCopies.Length; i++) { if (turned[i]) Write(car.Sim + PoseCopies[i] + 16, basis); Write(car.Sim + PoseCopies[i], V.Bytes(p)); }
        }
        public byte[] BasisBytes(Car car) { Check(car); return Read(car.Sim + Basis, 48); }

        public bool SetGhost(Car car, bool on)
        {
            Check(car);
            int now = Int(car.Sim + Collision);
            if (now != 2 && now != 4) return false;
            if (now != (on ? 4 : 2)) Write(car.Sim + Collision, BitConverter.GetBytes(on ? 4 : 2));
            return true;
        }

        // exhaust flames: mode 0 lets the game pop on request; request 2 = pop now
        public int BackfireModeOf(Car car)
        {
            if (!Backfires || !Valid(car) || Ptr(car.Address + CarBackfire) != BackfireVtable) return -1;
            int m = Int(car.Address + BackfireMode);
            return m >= 0 && m <= 3 ? m : -1;
        }
        public void SetBackfireMode(Car car, int mode) { if (BackfireModeOf(car) < 0) throw new Exception("no backfire"); Write(car.Address + BackfireMode, BitConverter.GetBytes(mode)); }
        public void Pop(Car car, bool on)
        {
            if (BackfireModeOf(car) != 0) return;
            int r = Int(car.Address + BackfireRequest);
            if ((r == 0 || r == 2) && r != (on ? 2 : 0)) Write(car.Address + BackfireRequest, BitConverter.GetBytes(on ? 2 : 0));
        }

        // lateral grip of the front and rear tyres; refused when another car shares the same tyre data
        public long[] TyresOf(Car car)
        {
            Check(car);
            long parms = Ptr(car.Sim + TyreParams), front = Ptr(car.Sim + Tyres), rear = Ptr(car.Sim + Tyres + 2 * TyreStride);
            if (parms <= 0 || !(front == parms + 0x960 || front == parms + 0x12c0) || rear != front + 0x147c) throw new Exception("tyre layout not recognised");
            for (int i = 0; i < 4; i++) if (Ptr(car.Sim + Tyres + i * TyreStride) != (i < 2 ? front : rear)) throw new Exception("tyre layout not recognised");
            foreach (long t in new[] { front, rear })
            {
                float a = Float(t), b = Float(t + 8);
                if (!(a >= 80 && a <= 600 && b >= 8 && b <= 40)) throw new Exception("tyre data not recognised");
            }
            foreach (long other in Cars())
            {
                if (other == 0 || other == car.Address || Ptr(other) != CarVtable) continue;
                long s = Ptr(other + CarSim);
                if (s == 0 || Ptr(s) != SimVtable) continue;
                for (int i = 0; i < 4; i++) { long t = Ptr(s + Tyres + i * TyreStride); if (t == front || t == rear) throw new Exception("tyres shared with another car"); }
            }
            return new[] { front, rear };
        }
        public float Grip(Car car, long tyre) { Check(car); float g = Float(tyre + TyreLateralGrip); if (!(g >= 0.05f && g <= 40)) throw new Exception("bad grip"); return g; }
        public void SetGrip(Car car, long tyre, float g) { Check(car); if (!(g >= 0.05f && g <= 40)) throw new Exception("bad grip"); WriteFloat(tyre + TyreLateralGrip, g); }

        public void Dispose() { if (handle != IntPtr.Zero) { CloseHandle(handle); handle = IntPtr.Zero; } }
    }
}
