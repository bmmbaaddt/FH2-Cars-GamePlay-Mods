# Per-car power, aero, grip, brakes and turbo from cars.json, written into the game database.
# Always rebuilds from the untouched original in backup_original\media, so cars removed from cars.json go back to stock.
#   python car_power.py            apply cars.json
#   python car_power.py --list     print every drivable car as JSON (for the settings window)
#   python car_power.py --restore  put the original database back
import glob, json, os, re, shutil, sqlite3, struct, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)   # this folder sits next to azimuth.exe

def find_game_db():
    # the game is wherever Azimuth's folder= points (any drive), else a folder beside azimuth.exe
    folders = []
    try:
        raw = open(os.path.join(ROOT, 'data', 'azimuth.ini'), 'rb').read()
        text = raw.decode('utf-16') if raw[:2] in (b'\xff\xfe', b'\xfe\xff') else raw.decode('utf-8', 'ignore')
        folders = [f.strip('"') for f in re.findall(r'(?m)^(?:folder|game_folder)=(.+?)\s*$', text)]
    except OSError:
        pass
    for folder in folders + glob.glob(os.path.join(ROOT, '*')):
        db = os.path.join(ROOT, folder, 'Mount', 'media', 'stripped', 'gamedbRC.slt')   # join keeps absolute folders as they are
        if os.path.exists(db): return db

DB = find_game_db()
if not DB: sys.exit('game not found: put the FH2 Mods folder next to azimuth.exe, and add the game in Azimuth and start it once')
ORIG = os.path.join(ROOT, 'backup_original', 'media', 'gamedbRC.slt')
SAVES = os.path.join(ROOT, 'data', 'titles')   # Azimuth 0.9.9 keeps saves per game: data\titles\<id>\saves
CARS_JSON = os.path.join(HERE, 'cars.json')
TURBO_TABLES = ['List_UpgradeEngineTurboSingle', 'List_UpgradeEngineTurboTwin', 'List_UpgradeEngineTurboQuad']
NOT_ROAD_TIRES = ('rally', 'offroad', 'drift', 'snow', 'drag')
FIRST_NEW_TIRE = 100   # cloned tire compounds get ids from here, one per car

if '--list' not in sys.argv:
    running = subprocess.run(['powershell', '-NoProfile', '-Command', '(Get-Process azimuth -ErrorAction SilentlyContinue).Path'], capture_output=True, text=True).stdout
    if any(os.path.normcase(p).startswith(os.path.normcase(ROOT + os.sep)) for p in running.splitlines()): sys.exit('close the game first')
if not os.path.exists(ORIG): os.makedirs(os.path.dirname(ORIG), exist_ok=True); shutil.copy2(DB, ORIG)

if '--list' in sys.argv:
    g = sqlite3.connect(f'file:{ORIG}?mode=ro', uri=True)
    rows = g.execute("select c.Id, m.IconPathBase, c.MediaName, c.Year from Data_Car c join List_CarMake m on m.ID=c.MakeID "
                     "where c.MediaName not like 'NUL\\_%' escape '\\' order by m.IconPathBase, c.MediaName").fetchall()   # PG_ = the drivable bus and box truck
    print(json.dumps([{'id': i, 'name': f"{'Traffic' if make == 'PG' else make} {media.split('_')[1]} ({year})"} for i, make, media, year in rows]))
    sys.exit()

shutil.copy2(ORIG, DB)
if '--restore' in sys.argv: sys.exit('restored the original game database')

def installed_parts():
    """car id -> engine id, drivetrain id, transmission row actually fitted in the newest save (swaps differ from stock)."""
    dbs = glob.glob(os.path.join(SAVES, '**', 'revisions', '*', 'data', 'PlayerDatabase'), recursive=True)
    if not dbs: return {}, {}, {}
    blob = open(max(dbs, key=os.path.getmtime), 'rb').read()
    tmp = os.path.join(tempfile.mkdtemp(), 'save.sqlite')
    open(tmp, 'wb').write(blob[8:8 + struct.unpack('<I', blob[4:8])[0]])
    s = sqlite3.connect(tmp)
    rows = s.execute('select CarId, Engine, Drivetrain, Transmission from Career_Garage').fetchall()
    s.close(); shutil.rmtree(os.path.dirname(tmp))
    return ({car: db.execute('select EngineID from List_UpgradeEngine where Id=?', (e,)).fetchone()[0] for car, e, d, g in rows if e != -1},
            {car: db.execute('select DrivetrainID from List_UpgradeDrivetrain where Id=?', (d,)).fetchone()[0] for car, e, d, g in rows if d != -1},
            {car: g for car, e, d, g in rows if g != -1})

cars = {int(k): v for k, v in json.load(open(CARS_JSON)).items()}
db = sqlite3.connect(DB)
engines, drivetrains, transmissions = installed_parts()
tire_cols = [r[1] for r in db.execute('pragma table_info(List_TireCompound)')]
boosted, revved, geared, regeared = set(), set(), set(), set()
for n, (cid, m) in enumerate(cars.items()):
    db.execute('update Data_Car set GameTorqueScale=GameTorqueScale*?, BodyAeroLongitudinalDrag=BodyAeroLongitudinalDrag*?, '
               'BodyAeroForwardDownforceFront=BodyAeroForwardDownforceFront*?, BodyAeroForwardDownforceRear=BodyAeroForwardDownforceRear*? where Id=?',
               (m['power'], m['drag'], m['downforce'], m['downforce'], cid))
    # grip: the car's best road tire gets its own copy of its compound with scaled friction
    tire = next((r for r in db.execute('select Id, TireCompoundID, TireModelName from List_UpgradeTireCompound where Ordinal=? order by Level desc', (cid,))
                 if r[2].lower() not in NOT_ROAD_TIRES), None)
    if tire and m['grip'] != 1:
        row = dict(zip(tire_cols, db.execute('select * from List_TireCompound where TireCompoundID=?', (tire[1],)).fetchone()))
        row.update(TireCompoundID=FIRST_NEW_TIRE + n, DisplayName=f'{FIRST_NEW_TIRE + n}_Custom_{cid}')
        for c in ('TorqueFreeLatFrictionScale', 'TorqueFreeLongFrictionScaleBrake', 'TorqueFreeLongFrictionScaleAccel0', 'TorqueFreeLongFrictionScaleAccel1'): row[c] *= m['grip']
        db.execute(f'insert into List_TireCompound ({",".join(tire_cols)}) values ({",".join("?" * len(tire_cols))})', [row[c] for c in tire_cols])
        db.execute('update List_UpgradeTireCompound set TireCompoundID=? where Id=?', (FIRST_NEW_TIRE + n, tire[0]))
    db.execute('update List_UpgradeBrakes set GameFrictionScaleBraking=GameFrictionScaleBraking*? where Ordinal=? and Level=(select max(Level) from List_UpgradeBrakes where Ordinal=?)',
               (m['brakes'], cid, cid))
    # turbo: more peak boost, full boost sooner, no torque loss before boost (an engine shared by two cars is boosted once)
    engine = engines.get(cid) or db.execute('select EngineID from List_UpgradeEngine where Ordinal=? and IsStock=1', (cid,)).fetchone()[0]
    if m['turbo'] != 1 and engine not in boosted:
        for t in TURBO_TABLES:
            db.execute(f'update {t} set MaxScale=MaxScale*?, PowerMaxScale=PowerMaxScale*0.7, MinScale=max(MinScale, 1.0) where EngineID=?', (m['turbo'], engine))
        boosted.add(engine)
    # rev limit: the redline and the torque curve's rpm range stretch together, so the engine pulls to the new limit
    if m.get('redline', 1) != 1 and engine not in revved:
        db.execute('update List_UpgradeEngineCamshaft set RedlineRPM=RedlineRPM*?, TorqueCurveMaxRPM=TorqueCurveMaxRPM*? where EngineID=?', (m['redline'], m['redline'], engine))
        revved.add(engine)
    # gearing: a shorter final drive makes every gear taller (higher top speed, slower acceleration) on every
    # transmission of the fitted drivetrain; in-game gear tuning still works on top
    drivetrain = drivetrains.get(cid) or db.execute('select DrivetrainID from List_UpgradeDrivetrain where Ordinal=? and IsStock=1', (cid,)).fetchone()[0]
    if m.get('gearing', 1) != 1 and drivetrain not in geared:
        db.execute('update List_UpgradeDrivetrainTransmission set FinalDriveRatio=FinalDriveRatio/? where DrivetrainID=?', (m['gearing'], drivetrain))
        geared.add(drivetrain)
    # gears: the fitted gearbox gets this many forward gears, keeping 1st and the stock step between gears
    # (so extra gears are taller than the old top gear); the game supports up to 8
    box = transmissions.get(cid) or db.execute('select Id from List_UpgradeDrivetrainTransmission where DrivetrainID=? and IsStock=1', (drivetrain,)).fetchone()[0]
    if m.get('gears', 1) > 1 and box not in regeared:   # 1 = stock gearbox
        count, first = db.execute('select NumGears - 1, GearRatio1 from List_UpgradeDrivetrainTransmission where Id=?', (box,)).fetchone()
        top = db.execute(f'select GearRatio{count} from List_UpgradeDrivetrainTransmission where Id=?', (box,)).fetchone()[0]
        n = min(int(m['gears']), 8)
        ratios = [first * (top / first) ** (k / (count - 1)) for k in range(n)] + [-1.0] * (8 - n)
        db.execute(f'update List_UpgradeDrivetrainTransmission set NumGears=?, {", ".join(f"GearRatio{k + 1}=?" for k in range(8))} where Id=?', (n + 1, *ratios, box))
        regeared.add(box)
    print(cid, m, 'tire' if tire else 'no road tire found')
db.commit()
assert db.execute('pragma integrity_check').fetchone()[0] == 'ok'
db.close()
print(f'applied to {len(cars)} car(s)')
