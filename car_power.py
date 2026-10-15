# Per-car power, aero, grip, brakes and turbo from cars.json, written into the game database.
# Always rebuilds from the untouched original in backup_original\media, so cars removed from cars.json go back to stock.
#   python car_power.py            apply cars.json
#   python car_power.py --list     print every drivable car as JSON (for the settings window)
#   python car_power.py --restore  put the original database back
import glob, json, os, shutil, sqlite3, struct, subprocess, sys, tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)   # this folder sits next to azimuth.exe
found = glob.glob(os.path.join(ROOT, '*', 'Mount', 'media', 'stripped', 'gamedbRC.slt'))
if not found: sys.exit('game not found: put the FH2 Mods folder next to azimuth.exe')
DB = found[0]
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
                     "where c.MediaName not like 'PG\\_%' escape '\\' and c.MediaName not like 'NUL\\_%' escape '\\' order by m.IconPathBase, c.MediaName").fetchall()
    print(json.dumps([{'id': i, 'name': f"{make} {media.split('_')[1]} ({year})"} for i, make, media, year in rows]))
    sys.exit()

shutil.copy2(ORIG, DB)
if '--restore' in sys.argv: sys.exit('restored the original game database')

def installed_engines():
    """car id -> engine id actually fitted in the newest save (engine swaps differ from stock)."""
    dbs = glob.glob(os.path.join(SAVES, '**', 'revisions', '*', 'data', 'PlayerDatabase'), recursive=True)
    if not dbs: return {}
    blob = open(max(dbs, key=os.path.getmtime), 'rb').read()
    tmp = os.path.join(tempfile.mkdtemp(), 'save.sqlite')
    open(tmp, 'wb').write(blob[8:8 + struct.unpack('<I', blob[4:8])[0]])
    s = sqlite3.connect(tmp)
    rows = s.execute('select CarId, Engine from Career_Garage').fetchall()
    s.close(); shutil.rmtree(os.path.dirname(tmp))
    return {car: db.execute('select EngineID from List_UpgradeEngine where Id=?', (part,)).fetchone()[0] for car, part in rows if part != -1}

cars = {int(k): v for k, v in json.load(open(CARS_JSON)).items()}
db = sqlite3.connect(DB)
engines = installed_engines()
tire_cols = [r[1] for r in db.execute('pragma table_info(List_TireCompound)')]
boosted = set()
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
    print(cid, m, 'tire' if tire else 'no road tire found')
db.commit()
assert db.execute('pragma integrity_check').fetchone()[0] == 'ok'
db.close()
print(f'applied to {len(cars)} car(s)')
