# Per-car power, aero, grip, brakes and turbo from cars.json, written into the game database.
# Always rebuilds from the untouched original in backup_original\media, so cars removed from cars.json go back to stock.
#   python car_power.py            apply cars.json
#   python car_power.py --list     print every drivable car as JSON (for the settings window)
#   python car_power.py --restore  put the original database back
import glob, hashlib, json, os, re, shutil, sqlite3, struct, subprocess, sys, tempfile

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
        appx = os.path.join(ROOT, folder, 'Mount', 'AppxManifest.xml')   # only FH2 itself (Anthem), not FH2 Presents Fast & Furious (Spire)
        # the game's manifest, not its database: a car mod may have moved the database into mediaoverride
        if os.path.exists(os.path.join(os.path.dirname(os.path.dirname(db)), 'zipmanifest.xml')) and (not os.path.exists(appx) or re.search(r'<Identity[^>]*Name="Anthem"', open(appx, encoding='utf-8', errors='ignore').read())): return db

DB = find_game_db()
if not DB: sys.exit('game not found: put the FH2 Mods folder next to azimuth.exe, and add the game in Azimuth and start it once')
ORIG = os.path.join(ROOT, 'backup_original', 'media', 'gamedbRC.slt')
SAVES = os.path.join(ROOT, 'data', 'titles')   # Azimuth 0.9.9 keeps saves per game: data\titles\<id>\saves
CARS_JSON = os.path.join(HERE, 'cars.json')
TURBO_TABLES = ['List_UpgradeEngineTurboSingle', 'List_UpgradeEngineTurboTwin', 'List_UpgradeEngineTurboQuad']
NOT_ROAD_TIRES = ('rally', 'offroad', 'drift', 'snow', 'drag')
FIRST_NEW_TIRE = 100   # cloned tire compounds get ids from here, one per car

def save_query(sql):
    """rows of a query on the newest save's database ([] with no save)."""
    dbs = glob.glob(os.path.join(SAVES, '**', 'revisions', '*', 'data', 'PlayerDatabase'), recursive=True)
    if not dbs: return []
    blob = open(max(dbs, key=os.path.getmtime), 'rb').read()
    tmp = os.path.join(tempfile.mkdtemp(), 'save.sqlite')
    open(tmp, 'wb').write(blob[8:8 + struct.unpack('<I', blob[4:8])[0]])
    s = sqlite3.connect(tmp)
    rows = s.execute(sql).fetchall()
    s.close(); shutil.rmtree(os.path.dirname(tmp))
    return rows

def save_garage():
    """(CarId, Engine, Drivetrain, Transmission) of every car in the newest save's garage."""
    return save_query('select CarId, Engine, Drivetrain, Transmission from Career_Garage')

if '--random' in sys.argv:   # a random car from the garage, for the window to show; reads the save only
    import random
    rows = save_garage()
    if not rows: sys.exit('no save found: play the game once first')
    print(random.choice(rows)[0]); sys.exit()

if '--list' not in sys.argv:
    running = subprocess.run(['powershell', '-NoProfile', '-Command', '(Get-Process azimuth -ErrorAction SilentlyContinue).Path'], capture_output=True, text=True).stdout
    if any(os.path.normcase(p).startswith(os.path.normcase(ROOT + os.sep)) for p in running.splitlines()): sys.exit('close the game first')
if os.path.exists(DB) and not os.path.exists(ORIG): os.makedirs(os.path.dirname(ORIG), exist_ok=True); shutil.copy2(DB, ORIG)

# A car mod in ExtraOptions' override folders (mediaoverride > mediapc > media) ships its own database, which the game
# reads instead: car mods go into that copy, rebuilt from a backup of it. When the copy is neither that backup nor what
# this script last wrote, the mod was installed or updated: the backup takes the mod's changes.
WRITTEN = None
def sha(path): return hashlib.sha256(open(path, 'rb').read()).hexdigest()

def merge_changes(base, before, after):
    """Adds to base what changed from before to after. Car importers (like the FM5 importer) add their cars to the
    database the game reads, which already holds our car mods: only the importer's own rows reach the backup, so
    our mods are not applied twice."""
    tmp = base + '.new'; shutil.copy2(base, tmp)
    g = sqlite3.connect(tmp)
    g.execute('attach ? as b', (before,)); g.execute('attach ? as a', (after,))
    for (t,) in g.execute("select name from a.sqlite_master where type='table'").fetchall():
        if not g.execute("select 1 from main.sqlite_master where type='table' and name=?", (t,)).fetchone():
            g.execute(g.execute('select sql from a.sqlite_master where name=?', (t,)).fetchone()[0])
            g.execute(f'insert into main."{t}" select * from a."{t}"')
        elif not g.execute("select 1 from b.sqlite_master where type='table' and name=?", (t,)).fetchone():
            g.execute(f'insert or replace into main."{t}" select * from a."{t}"')
        else:
            same = ' and '.join(f'x."{c}" is "{t}"."{c}"' for _, c, *_ in g.execute(f'pragma a.table_info("{t}")'))
            g.execute(f'delete from main."{t}" where exists (select 1 from (select * from b."{t}" except select * from a."{t}") x where {same})')
            g.execute(f'insert or replace into main."{t}" select * from a."{t}" except select * from b."{t}"')
    g.commit()
    assert g.execute('pragma integrity_check').fetchone()[0] == 'ok'
    g.close(); os.replace(tmp, base)

def replaced(before, after):
    """a car pack swapped in its own database (cars we wrote are gone), rather than an importer adding to ours"""
    g = sqlite3.connect(after); g.execute('attach ? as b', (before,))
    gone = g.execute('select 1 from b.Data_Car where Id not in (select Id from main.Data_Car) limit 1').fetchone()
    g.close(); return bool(gone)

for folder in ('mediaoverride', 'mediapc'):
    override = os.path.join(os.path.dirname(os.path.dirname(os.path.dirname(DB))), folder, 'stripped', 'gamedbRC.slt')
    if os.path.exists(override):
        if '--list' not in sys.argv and os.path.exists(ORIG): shutil.copy2(ORIG, DB)   # the masked media copy goes back to stock
        DB, ORIG, WRITTEN = override, os.path.join(ROOT, 'backup_original', folder, 'gamedbRC.slt'), os.path.join(ROOT, 'backup_original', folder, 'gamedbRC.written')
        current = sha(DB)
        if not os.path.exists(ORIG):
            os.makedirs(os.path.dirname(ORIG), exist_ok=True); shutil.copy2(DB, ORIG)
        elif current not in (sha(ORIG), open(WRITTEN).read() if os.path.exists(WRITTEN) else ''):
            # ponytail: a row the mod changed that our car mods also changed keeps both changes; rare, a full re-import fixes it
            if os.path.exists(WRITTEN + '.slt') and not replaced(WRITTEN + '.slt', DB): merge_changes(ORIG, WRITTEN + '.slt', DB)
            else: shutil.copy2(DB, ORIG)
        break

if not os.path.exists(ORIG): sys.exit(r'game database missing: put the original Mount\media\stripped\gamedbRC.slt back from your game download')

if '--list' in sys.argv:
    g = sqlite3.connect(f'file:{ORIG}?mode=ro', uri=True)
    rows = g.execute("select c.Id, m.IconPathBase, c.MediaName, c.Year from Data_Car c join List_CarMake m on m.ID=c.MakeID "
                     "where c.MediaName not like 'NUL\\_%' escape '\\' order by m.IconPathBase, c.MediaName").fetchall()   # PG_ = the drivable bus and box truck
    print(json.dumps([{'id': i, 'name': f"{'Traffic' if make == 'PG' else make} {media.split('_')[1]} ({year})"} for i, make, media, year in rows]))
    sys.exit()

if '--restore' in sys.argv: shutil.copy2(ORIG, DB); sys.exit('restored the original game database')
# built in a work copy that replaces the game's only when everything worked: a crash halfway must not leave a half-built
# database, which the next run would take for a newly installed car mod and merge into the backup
PREV = DB if os.path.exists(DB) else ORIG   # what the game reads now, for the owned-parts check
WORK = DB + '.work'; shutil.copy2(ORIG, WORK)

def val(sql, *args):
    """first column of the first row, or None when the database has no such row (a car pack was swapped)."""
    row = db.execute(sql, args).fetchone()
    return row and row[0]

def installed_parts():
    """car id -> engine id, drivetrain id, transmission row actually fitted in the newest save (swaps differ from stock)."""
    rows = save_garage()
    return ({car: val('select EngineID from List_UpgradeEngine where Id=?', e) for car, e, d, g in rows if e != -1},
            {car: val('select DrivetrainID from List_UpgradeDrivetrain where Id=?', d) for car, e, d, g in rows if d != -1},
            {car: g for car, e, d, g in rows if g != -1})

cars = {int(k): v for k, v in json.load(open(CARS_JSON)).items()}
db = sqlite3.connect(WORK)
db.execute('attach ? as prev', (PREV,))   # for the owned-parts check: only parts this rebuild takes away count, not ones a removed car pack took
engines, drivetrains, transmissions = installed_parts()
tire_cols = [r[1] for r in db.execute('pragma table_info(List_TireCompound)')]
boosted, revved, geared, regeared = set(), set(), set(), set()
for n, (cid, m) in enumerate(cars.items()):
    if val('select Id from Data_Car where Id=?', cid) is None:
        print(cid, 'skipped: not in this game database (car pack removed?)'); continue
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
    engine = engines.get(cid) or val('select EngineID from List_UpgradeEngine where Ordinal=? and IsStock=1', cid)
    if m['turbo'] != 1 and engine is not None and engine not in boosted:
        for t in TURBO_TABLES:
            db.execute(f'update {t} set MaxScale=MaxScale*?, PowerMaxScale=PowerMaxScale*0.7, MinScale=max(MinScale, 1.0) where EngineID=?', (m['turbo'], engine))
        boosted.add(engine)
    # rev limit: the redline and the torque curve's rpm range stretch together, so the engine pulls to the new limit
    redline = min(m.get('redline', 1), 2)   # the game crashes above x2
    if redline != 1 and engine is not None and engine not in revved:
        db.execute('update List_UpgradeEngineCamshaft set RedlineRPM=RedlineRPM*?, TorqueCurveMaxRPM=TorqueCurveMaxRPM*? where EngineID=?', (redline, redline, engine))
        revved.add(engine)
    # gearing: a shorter final drive makes every gear taller (higher top speed, slower acceleration) on every
    # transmission of the fitted drivetrain; in-game gear tuning still works on top
    drivetrain = drivetrains.get(cid) or val('select DrivetrainID from List_UpgradeDrivetrain where Ordinal=? and IsStock=1', cid)
    if m.get('gearing', 1) != 1 and drivetrain is not None and drivetrain not in geared:
        db.execute('update List_UpgradeDrivetrainTransmission set FinalDriveRatio=FinalDriveRatio/? where DrivetrainID=?', (m['gearing'], drivetrain))
        geared.add(drivetrain)
    # gears: the fitted gearbox gets this many forward gears, keeping 1st and the stock step between gears
    # (so extra gears are taller than the old top gear); the game supports up to 8
    box = transmissions.get(cid) or val('select Id from List_UpgradeDrivetrainTransmission where DrivetrainID=? and IsStock=1', drivetrain)
    if m.get('gears', 1) > 1 and val('select Id from List_UpgradeDrivetrainTransmission where Id=?', box) is not None and box not in regeared:   # 1 = stock gearbox
        count, first = db.execute('select NumGears - 1, GearRatio1 from List_UpgradeDrivetrainTransmission where Id=?', (box,)).fetchone()
        top = db.execute(f'select GearRatio{count} from List_UpgradeDrivetrainTransmission where Id=?', (box,)).fetchone()[0]
        n = min(int(m['gears']), 8)
        ratios = [first * (top / first) ** (k / (count - 1)) for k in range(n)] + [-1.0] * (8 - n)
        db.execute(f'update List_UpgradeDrivetrainTransmission set NumGears=?, {", ".join(f"GearRatio{k + 1}=?" for k in range(8))} where Id=?', (n + 1, *ratios, box))
        regeared.add(box)
    print(cid, m, 'tire' if tire else 'no road tire found')
# Packs (settings.json): extra parts for every car, rebuilt from the original like the car mods, so switching one off takes it out again
PART_ENUMS = {'Valves': 13, 'Displacement': 14, 'PistonsCompression': 15, 'FuelSystem': 16, 'Ignition': 17, 'Exhaust': 18, 'Intake': 19,
              'Flywheel': 20, 'Manifold': 21, 'RestrictorPlate': 22, 'OilCooling': 23, 'Intercooler': 29}
settings = json.load(open(os.path.join(HERE, 'settings.json'), encoding='utf-8-sig'))
def insert_row(table, r):
    db.execute(f'insert or ignore into "{table}" ({",".join(r)}) values ({",".join("?" * len(r))})', list(r.values()))
def menu_entry(enum, level, name, description):
    """the upgrade shop lists a part level only when Upgrades has a row for it"""
    cols = [c[1] for c in db.execute('pragma table_info(Upgrades)')]
    for (type_id,) in db.execute('select id from UpgradeTypes where PartEnum=?', (enum,)).fetchall():
        top = db.execute('select * from Upgrades where TypeId=? order by Level desc limit 1', (type_id,)).fetchone()
        if top and val('select 1 from Upgrades where TypeId=? and Level=?', type_id, level) is None:
            insert_row('Upgrades', dict(zip(cols, top), id=val('select max(id) + 1 from Upgrades'), Level=level, Name=name, Description=description))
packs = []
if settings.get('PackEngineParts', 0) > 0:   # a level 4 of every engine part: the best one again, with this much more torque
    more = 1 + settings['PackEngineParts'] / 100
    for part, enum in PART_ENUMS.items():
        t = f'List_UpgradeEngine{part}'
        cols = [c[1] for c in db.execute(f'pragma table_info("{t}")')]
        for row in db.execute(f'select * from "{t}" t where Level=(select max(Level) from "{t}" where EngineID=t.EngineID) and Level < 4 group by EngineID').fetchall():
            r = dict(zip(cols, row))
            r.update(Id=r['EngineID'] * 1000 + 4, Level=4, IsStock=0, ManufacturerID=255, Price=1000)
            if 'TorqueScale' in r: r['TorqueScale'] *= more
            if part == 'Displacement': r['Disp'] = round(r['Disp'] * 1.1)
            if part == 'Flywheel': r['MomentInertia'] = max(0.001, r['MomentInertia'] * 0.65)
            if part == 'Intercooler': r['MaxScaleScale'] *= 1.1
            insert_row(t, r)
        menu_entry(enum, 4, f'Race+ {part}', 'Extra FH2 Mods engine part; the original upgrades are kept.')
    packs.append(f'engine parts +{settings["PackEngineParts"]:g}%')
if settings.get('PackEngineSwaps'):   # every engine fits every car: new List_UpgradeEngine levels after the car's own
    db.execute('create index if not exists FH2Mods_Swaps on List_UpgradeEngine(Ordinal, EngineID)')
    db.execute("""with car as (select u.Ordinal, max(u.Level) top,
                                      (select e.[EngineMass-kg] from List_UpgradeEngine s join Data_Engine e on e.EngineID=s.EngineID where s.Ordinal=u.Ordinal and s.IsStock=1) mass
                               from List_UpgradeEngine u join Data_Car c on c.Id=u.Ordinal and c.IsDrivable=1
                               group by u.Ordinal having sum(u.IsStock=1) = 1),
                  missing as (select car.Ordinal, car.top, car.mass, e.EngineID, e.[EngineMass-kg] m,
                                     row_number() over (partition by car.Ordinal order by e.EngineID) rn
                              from car cross join Data_Engine e
                              where not exists (select 1 from List_UpgradeEngine x where x.Ordinal=car.Ordinal and x.EngineID=e.EngineID))
                  insert into List_UpgradeEngine (Id, Ordinal, Level, EngineID, IsStock, ManufacturerID, Price, MassDiff, WeightDistDiff, DragScale, WindInstabilityScale)
                  select Ordinal * 1000 + top + rn, Ordinal, top + rn, EngineID, 0, 255, 1000, m - mass, 0, 1, 1 from missing where top + rn < 1000""")
    packs.append('engine swaps')
if settings.get('PackTrafficCars'):   # traffic and AI-only cars in the Autoshow, in their make's usual bucket
    db.execute("""insert into Data_Car_Buckets (CarId, CarBucket, BucketHero)
        select c.Id, coalesce((select b.CarBucket from Data_Car_Buckets b join Data_Car m on m.Id=b.CarId where m.MakeID=c.MakeID group by b.CarBucket order by count(*) desc limit 1),
                              (select CarBucket from Data_Car_Buckets group by CarBucket order by count(*) desc limit 1)), 0
        from Data_Car c join TrafficCars t on t.CarId=c.Id
        where not exists (select 1 from Data_Car_Buckets b where b.CarId=c.Id)
          and exists (select 1 from List_UpgradeEngine e where e.Ordinal=c.Id and e.IsStock=1)""")
    # the Autoshow lists the game's Drivable_Data_Car view, which drops every car in TrafficCars; traffic still spawns
    # them from AI\CarLoadingSettings.xml, so taking them out of the table only makes them buyable
    db.execute('delete from TrafficCars where CarId in (select CarId from Data_Car_Buckets)')
    packs.append('traffic cars for sale')
if settings.get('PackFreeCars'):
    db.execute('update Data_Car set BaseCost=1')
    packs.append('free cars')
# a part the garage owns or has fitted must still exist, or the save would point at nothing
tables = {'Engine': 'List_UpgradeEngine', **{p: f'List_UpgradeEngine{p}' for p in PART_ENUMS}}
by_enum = {0: 'List_UpgradeEngine', **{e: f'List_UpgradeEngine{p}' for p, e in PART_ENUMS.items()}}
used = [(t, car, pid) for c, t in tables.items() for car, pid in save_query(f'select CarId, "{c}" from Career_Garage where "{c}" != -1')]
used += [(by_enum[e], car, pid) for car, e, pid in save_query('select g.CarId, p.PartEnum, p.PartId from Career_PurchasedParts p join Career_Garage g on g.Id=p.GarageId') if e in by_enum]
gone = sorted({(t, car, pid) for t, car, pid in used if val(f'select 1 from "{t}" where Id=?', pid) is None and val(f'select 1 from prev."{t}" where Id=?', pid) is not None})
if gone:
    db.close(); os.remove(WORK)
    sys.exit('your garage uses parts from a pack you switched off (' + ', '.join(f'car {c} {t[18:] or "engine"} {p}' for t, c, p in gone[:5]) +
             '): switch the pack back on, or fit the stock part in game first')
print('packs:', ', '.join(packs) or 'none')
# a car pack swap can drop cars the save owns; the window shows only the last line, so the warning goes there
lost = sorted({c for (c,) in save_query('select CarId from Career_Garage')} - {c for (c,) in db.execute('select Id from Data_Car')})
lost = f'; WARNING: your garage has {len(lost)} car(s) this car pack does not have (ids {", ".join(map(str, lost[:5]))}): put the old pack back or sell them first' if lost else ''
# the Skill Legend perk's x6 is a fixed cap that replaces apply.ps1's MaxMultiplier, so it stays one above that
top = settings.get('MaxSkillMultiplier', 5)
db.execute("update Perks set Modifier=? where Name='Skill Legend'", (max(6.0, top + 1.0),))
db.commit()
assert db.execute('pragma integrity_check').fetchone()[0] == 'ok'
db.close()
os.replace(WORK, DB)
if WRITTEN: open(WRITTEN, 'w').write(sha(DB)); shutil.copy2(DB, WRITTEN + '.slt')   # the copy lets a later mod update be merged
print(f'applied to {len(cars)} car(s)' + lost)
