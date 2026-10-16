# FH2 mod patcher. Rebuilds media zips from the untouched originals in backup_original\media,
# applying the values in settings.json, in the game's own zip layout:
# local headers without extras, central entries with extra 0x1123 = data offset,
# and media\zipmanifest.xml updated with each zip's new central directory position.
# -RoundTrip: rebuild unchanged and compare to the originals (proves the writer is byte-exact).
# -Restore: put the original files back.
param([string]$Settings = (Join-Path $PSScriptRoot 'settings.json'), [switch]$RoundTrip, [switch]$Restore)
$ErrorActionPreference = 'Stop'
# this folder sits next to azimuth.exe; the game is wherever Azimuth's folder= points (any drive), else a folder beside it with Mount\media
$root   = Split-Path $PSScriptRoot
$azIni = Join-Path $root 'data\azimuth.ini'
$azText = if (Test-Path $azIni) { [IO.File]::ReadAllText($azIni) } else { '' }
$candidates = @([regex]::Matches($azText, '(?m)^(?:folder|game_folder)=(.+?)\s*$') | ForEach-Object { $_.Groups[1].Value.Trim('"') }) + @(Get-ChildItem $root -Directory | ForEach-Object FullName)
$manifestFile = $candidates | ForEach-Object { Join-Path ([IO.Path]::Combine($root, $_)) 'Mount\media\zipmanifest.xml' } | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $manifestFile) { throw "game not found: put the FH2 Mods folder next to azimuth.exe, and add the game in Azimuth and start it once" }
$media  = Split-Path $manifestFile
$gameFolder = Split-Path (Split-Path $media)   # full path; ExtraOptions accepts absolute folders
$backup = Join-Path $root 'backup_original\media'
if (Get-Process azimuth -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$root\*" }) { throw 'close the game first' }
$zips   = 'Camera.zip', 'GameTunableSettings.zip', 'EntityModel.zip', 'stateflow.zip'
$loose  = 'physics\PhysicsSettings.ini', 'physics\surfaceTypes.xml'   # plain files under media, patched in place
# breakable objects' weights, per map (Storm Island's map is in its DLC folder)
$tracks = @('tracks\anthem\physicsdefinitions.bin', 'tracks\homespace02\physicsdefinitions.bin') +
          @(Get-ChildItem (Join-Path (Split-Path $media) 'DLC\*\media\tracks\*\physicsdefinitions.bin') -ErrorAction SilentlyContinue | ForEach-Object { '..\' + $_.FullName.Substring((Split-Path $media).Length + 1) })
New-Item -ItemType Directory -Force $backup | Out-Null
foreach ($f in $zips + $loose + $tracks + 'zipmanifest.xml') {
  $orig = Join-Path $backup $f
  if (-not (Test-Path $orig)) { New-Item -ItemType Directory -Force (Split-Path $orig) | Out-Null; Copy-Item (Join-Path $media $f) $orig }   # backup = untouched original
}
# ExtraOptions with this tool's additions (ghost cars, keyboard keys) replaces the installed ExtraOptions DLL:
# data\bin\azimuth-tls.dll, or azimuth-tls.dll.original when the LivingCockpit loader sits in front of it.
$eoIni    = Join-Path $root 'data\bin\FH2_ExtraOptions.ini'
$eoFork   = Join-Path $PSScriptRoot 'ExtraOptions\azimuth-tls.dll'
$eoBackup = Join-Path $root 'backup_original\ExtraOptions\azimuth-tls.dll'
$eoTarget = 'azimuth-tls.dll.original', 'azimuth-tls.dll' | ForEach-Object { Join-Path $root "data\bin\$_" } |
            Where-Object { (Test-Path $_) -and [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($_)).Contains('FH2_ExtraOptions') } | Select-Object -First 1
# the stock ExtraOptions refuses to load with settings it doesn't know, so these sections go when it comes back
function StripForkSettings($text) { [regex]::Replace($text, '(?ms)^\[(Keyboard|Cockpit|Traffic)\]\r?\n.*?(?=^\[|\z)', '').TrimEnd() + "`n" }
if ($Restore) {
  foreach ($f in $zips + $loose + $tracks + 'zipmanifest.xml') { Copy-Item -Force (Join-Path $backup $f) (Join-Path $media $f) }
  if ($eoTarget -and (Test-Path $eoBackup)) {
    Copy-Item -Force $eoBackup $eoTarget
    if (Test-Path $eoIni) { [IO.File]::WriteAllText($eoIni, (StripForkSettings ([IO.File]::ReadAllText($eoIni))), (New-Object Text.UTF8Encoding($false))) }
    '  ExtraOptions: original DLL restored'
  }
  'restored original files'; return
}
if ($eoTarget -and (Test-Path $eoFork) -and (Get-FileHash $eoTarget).Hash -ne (Get-FileHash $eoFork).Hash) {
  if (-not (Test-Path $eoBackup)) { New-Item -ItemType Directory -Force (Split-Path $eoBackup) | Out-Null; Copy-Item $eoTarget $eoBackup }
  Copy-Item -Force $eoFork $eoTarget
  "  ExtraOptions: installed the FH2 Mods build ($(Split-Path -Leaf $eoTarget))"
}
$eoActive = $eoTarget -and (Test-Path $eoFork) -and (Get-FileHash $eoTarget).Hash -eq (Get-FileHash $eoFork).Hash
if (-not $eoTarget) { '  ExtraOptions not found in data\bin: ghost cars and keyboard keys need it (install ExtraOptions, then Apply again)' }

# ExtraOptions finds the game through [launcher] game_folder, which Azimuth 0.9.9 no longer writes (it may drop it again on a settings change).
if ($azText -match '(?m)^\[launcher\]' -and $azText -notmatch '(?m)^game_folder=') { [IO.File]::WriteAllText($azIni, ($azText -replace '(?m)^\[launcher\]\r?\n', "[launcher]`r`ngame_folder=$gameFolder`r`n"), [Text.Encoding]::Unicode); '  azimuth.ini: game_folder added back for ExtraOptions' }

$s = Get-Content $Settings -Raw | ConvertFrom-Json
$inv = [Globalization.CultureInfo]::InvariantCulture
function Num($v) { ([double]$v).ToString('0.######', $inv) }

# Multiplies min/max of every <Density> in the cartype="traffic" list by $factor.
function ScaleTraffic([double]$factor) {
  $densityEval = { param($m) $m.Groups[1].Value + (Num ([double]$m.Groups[2].Value * $factor)) + '"' }.GetNewClosure()
  $listEval = { param($m) [regex]::Replace($m.Value, '(\b(?:min|max)=")([\d.]+)"', [Text.RegularExpressions.MatchEvaluator]$densityEval) }.GetNewClosure()
  $patch = ,@('(?s)<DensityList cartype="traffic">.*?</DensityList>', $listEval)
  return ,$patch   # keep the one-patch list from being unrolled by the pipeline
}

$freeRoam = [double]$s.FreeRoamTrafficPercent / 100
$chainTime = if ($s.SkillChainSeconds -gt 0) { "<property id=""MaxTimeBetweenSkills"" value=""$(Num $s.SkillChainSeconds)""/>" } else { '' }   # 0 = the game's own
$patches = @{
  'Camera.zip' = @{
    'CameraSettings.ini' = @(
      ,@('(?m)^Driver\\FOV 48\b', "Driver\FOV $(Num $s.CockpitFOV)")
      ,@('(?m)^Driver\\DefaultOffsetFromSteeringWheelZ 0\b', "Driver\DefaultOffsetFromSteeringWheelZ $(Num $s.SeatOffsetZ)")
      ,@('(?m)^Driver\\DefaultOffsetFromSteeringWheelY 0\b', "Driver\DefaultOffsetFromSteeringWheelY $(Num $s.SeatOffsetY)")
    )
  }
  'GameTunableSettings.zip' = @{
    'PlayerXPLevels.xml' = @(
      ,@('CreditRewardPerLevel="0"', "CreditRewardPerLevel=""$([long]$s.CreditsPerLevel)""")
    )
    'Skills.xml' = @(
      ,@('Fame="([\d.]+)"', { param($m) 'Fame="{0}"' -f (Num ([double]$m.Groups[1].Value * $s.SkillPointsMultiplier)) }.GetNewClosure())
    )
    'SkillChainXPRewards.xml' = @(
      ,@('XPAwarded="(\d+)"', { param($m) 'XPAwarded="{0}"' -f [long]([double]$m.Groups[1].Value * $s.SkillXPMultiplier) }.GetNewClosure())
    )
  }
  'stateflow.zip' = @{}
  'EntityModel.zip' = @{
    'Templates/templates_races.xml'           = ,@('<property id="CreditsPerKm" value="1000" />', "<property id=""CreditsPerKm"" value=""$([long]$s.CreditsPerKm)"" />")
    'Templates/templates_bucket_list.xml'     = ,@('<property id="CreditsPerKm" value="1000" />', "<property id=""CreditsPerKm"" value=""$([long]$s.CreditsPerKm)"" />")
    'Templates/templates_race_encounters.xml' = ,@('<property id="CreditsPerKm" value="1000" />', "<property id=""CreditsPerKm"" value=""$([long]$s.CreditsPerKm)"" />")
    'Templates/templates_global.xml' = @(
      ,@('<property id="MaxMultiplier" value="5.0"/>', "<property id=""MaxMultiplier"" value=""$(Num $s.MaxSkillMultiplier)""/>$chainTime")
    )
    'AI/Densities.xml' = ScaleTraffic $freeRoam
    # more traffic needs a higher cap on live traffic cars (18 by default)
    'AI/CarLoadingSettings.xml' = ,@('MaxActiveTraffic="18"', "MaxActiveTraffic=""$([Math]::Max(18, [int][Math]::Round(18 * $freeRoam)))""")
  }
}
if (-not $s.RaceTraffic) {
  $patches['EntityModel.zip']['AI/DensitiesRace.xml'] = ScaleTraffic 0
  $patches['GameTunableSettings.zip']['CareerRaceModes.xml'] = ,@('HasTraffic="true"', 'HasTraffic="false"')
}
# impacts above these speeds (mph) count as a crash and cancel the skill chain
if ($s.CrashesKeepSkillChain) {
  $patches['GameTunableSettings.zip']['Skills.xml'] += ,@('WorldCollisionThreshold="12.000000" CarCollisionThreshold="12.000000" SmashableCollisionTolerance="22.000000"',
    'WorldCollisionThreshold="9999.000000" CarCollisionThreshold="9999.000000" SmashableCollisionTolerance="9999.000000"')
}
if (-not $s.RoadTripTraffic) { $patches['EntityModel.zip']['AI/DensitiesRoadTrip.xml'] = ScaleTraffic 0 }
# Ghost traffic is done by ExtraOptions ([Traffic] GhostCars, written below): editing the ai_ow preset had no effect.
# Ctrl+key time and weather controls in free roam, rebuilt from the E3 2014 demo's keyboard shortcuts
# (that demo's template and script ship with the game but are never loaded outside the demo)
if ($s.WorldControls) {
  $patches['stateflow.zip']['e3_shortcuts.xml'] = ,@('</stategroup>\s*$', @'
  <!-- FH2 Mods: more time presets and clear weather -->
  <state id="fh2mods_time_sunrise">
    <behavior id="GameControlWrapper" />
    <node state="global.clear_target_time_of_day" />
    <node state="global.set_time_of_day" hours="06" minutes="30" targeted="true" speed="1500" />
  </state>
  <state id="fh2mods_time_midday">
    <behavior id="GameControlWrapper" />
    <node state="global.clear_target_time_of_day" />
    <node state="global.set_time_of_day" hours="12" minutes="00" targeted="true" speed="1500" />
  </state>
  <state id="fh2mods_weather_clear">
    <node state="goto_weather_dry_step_1" />
    <node state="global.wait_for_time" time="10.0" />
    <node state="goto_weather_dry_step_2" />
  </state>
  <state id="fh2mods_time_stop"><node state="global.stop_time" /></state>
  <state id="fh2mods_time_start"><node state="global.start_time" /></state>
  <state id="fh2mods_traffic_off">
    <node state="global.ai_spawning_mode_festival" />
    <node state="global.ai_despawn_traffic" fadeout="true" />
  </state>
  <state id="fh2mods_traffic_on"><node state="global.ai_spawning_mode_both" /></state>
  <state id="fh2mods_repair"><node state="global.reset_car_damage" /></state>
  <state id="fh2mods_snap_to_road">
    <behavior id="GameControlWrapper" />
    <node state="global.snap_player_car_to_road" />
  </state>
</stategroup>
'@)
  $weather = foreach ($w in @(('weather_control_to_rain_step_1', 1.0, 0.0, 0.0, 20), ('weather_control_to_rain_step_2', 1.0, 0.5, 0.0, 15),
                             ('weather_control_to_rain_step_3', 1.0, 0.5, 1.0, 30), ('weather_control_to_dry_step_1', 0.5, 0.0, 0.8, 10),
                             ('weather_control_to_dry_step_2', 0.0, 0.0, 0.0, 15))) {
    "    <component type=""TriggerWeatherComponent"" id=""$($w[0])""><property id=""ControlAtmosphericWetness"" value=""true""/><property id=""DesiredAtmosphericWetness"" value=""$($w[1])""/><property id=""ControlPrecipitationLevel"" value=""true""/><property id=""DesiredPrecipitationLevel"" value=""$($w[2])""/><property id=""ControlEnvironmentalWetness"" value=""true""/><property id=""DesiredEnvironmentalWetness"" value=""$($w[3])""/><property id=""TransitionTime"" value=""$($w[4])""/></component>"
  }
  # key, state machine, flow, flow on the second press (toggle keys)
  $keys = foreach ($k in @(('t', 'tod', 'goto_time_of_day_midnight'), ('y', 'tod', 'fh2mods_time_sunrise'), ('u', 'tod', 'fh2mods_time_midday'),
                          ('i', 'tod', 'goto_time_of_day_5_ish'), ('p', 'clock', 'fh2mods_time_stop', 'fh2mods_time_start'),
                          ('w', 'weather', 'goto_weather_rain'), ('r', 'weather', 'fh2mods_weather_clear'),
                          ('g', 'traffic', 'fh2mods_traffic_off', 'fh2mods_traffic_on'), ('n', 'car', 'fh2mods_repair'), ('b', 'car', 'fh2mods_snap_to_road'))) {
    $type = if ($k.Count -gt 3) { 'KeyboardShortcutToggleComponent' } else { 'KeyboardShortcutComponent' }
    $toggle = if ($k.Count -gt 3) { "<property id=""StateFlowToggle"" value=""e3_keyboard_shortcuts.$($k[3])""/>" } else { '' }
    "    <component type=""$type"" id=""fh2mods_key_$($k[0])""><property id=""Key"" value=""$($k[0])""/><property id=""Modifiers"" value=""CONTROL""/><property id=""StateMachineID"" value=""state_machine_$($k[1])""/><property id=""StateFlow"" value=""e3_keyboard_shortcuts.$($k[2])""/>$toggle</component>"
  }
  $machines = foreach ($m in 'tod', 'clock', 'weather', 'traffic', 'car') { "    <component type=""GameStateMachineComponent"" id=""state_machine_$m""><property id=""TargetFlow"" value=""""/></component>" }
  $patches['EntityModel.zip']['Templates/templates_e3.xml'] = ,@('</entity_templates>\s*$', "  <entity_template name=""fh2mods_world_controls"">`r`n$($keys -join "`r`n")`r`n$($weather -join "`r`n")`r`n$($machines -join "`r`n")`r`n  </entity_template>`r`n</entity_templates>`r`n")
  foreach ($f in 'entities_offline_freeroam.xml', 'entities_offline_freeroam_island.xml') {
    $patches['EntityModel.zip'][$f] = ,@('<entity id="micro_save_continuous">', "<entity id=""fh2mods_world_controls"" template=""fh2mods_world_controls""/>`r`n`r`n  <entity id=""micro_save_continuous"">")
  }
}
$loosePatches = @{ 'physics\PhysicsSettings.ini' = @(); 'physics\surfaceTypes.xml' = @() }
# every car but yours stays visible but can't be hit (traffic, drivatars, race AI); only the FH2 Mods ExtraOptions knows this setting
if ($eoActive -and (Test-Path $eoIni)) {
  $eo = [IO.File]::ReadAllText($eoIni) -replace '(?ms)^\[Cockpit\]\r?\n.*?(?=^\[|\z)', ''   # steering wheel setting from an earlier version
  $ghost = "GhostCars=$([int][bool]$s.GhostTraffic)"
  $eo = if ($eo -match '(?m)^GhostCars=') { $eo -replace '(?m)^GhostCars=\d*', $ghost }
        else { $eo.TrimEnd() + "`n`n[Traffic]`n; drive through every other car; they stay visible`n$ghost`n" }
  [IO.File]::WriteAllText($eoIni, $eo, (New-Object Text.UTF8Encoding($false)))
  "  FH2_ExtraOptions.ini: $ghost"
} elseif ($s.GhostTraffic) { '  Ghost cars skipped: it needs ExtraOptions in data\bin' }
# crashes and breakables stop turning the car: hit forces push it but no longer spin, flip or tumble it,
# including the game's extra random spin on big impacts (HackLargeImpactLowAngVelCollTorqueScale)
if ($s.NoCrashSpin) {
  foreach ($k in 'CarObjSmashableCollTorqueScale(Yaw|Pitch|Roll)', 'CarObjObjCollTorqueScale(Yaw|Pitch|Roll)', 'CarObjWorldCollTorqueScale(Yaw|Pitch|Roll)',
                 'HumanCarAICarCollYawTorqueScale0', 'HackLargeImpactLowAngVelCollTorqueScale\\(Yaw|Pitch|Roll)TorqueScale(CarSpace|CollSpace)',
                 'HackLargeImpactLowAngVelCollTorqueScale\\(YVelRandImpulseMax|CollSpacePitchRandImpulseMax|CollSpaceRollRandImpulseMax|MaxAngVelAddedFromTorqueScales)') {
    $loosePatches['physics\PhysicsSettings.ini'] += ,@("(?m)^($k) [\d.]+(?=\r?$)", { param($m) "$($m.Groups[1].Value) 0" })
  }
}
# the car body slides along walls, guard rails and tyre walls instead of grabbing them (tyres and the road are separate)
if ($s.SlideOffWalls) {
  foreach ($c in 'HardWorld', 'GuardRail', 'TireWall') {
    $loosePatches['physics\surfaceTypes.xml'] += ,@("(<Node Category0=""$c"" Category1=""Car"" FrictionCoefficient="")[\d.]+""", { param($m) "$($m.Groups[1].Value)0.050000""" })
  }
}

# PowerShell 5.1 reads 0xFFFFFFFF as int32 -1, so the CRC uses [long] arithmetic.
$crcTable = [long[]]::new(256)
for ($n = 0; $n -lt 256; $n++) { [long]$c = $n; for ($k = 0; $k -lt 8; $k++) { if ($c -band 1) { $c = 3988292384L -bxor ($c -shr 1) } else { $c = $c -shr 1 } }; $crcTable[$n] = $c }
function Crc32([byte[]]$d) { [long]$c = 4294967295L; foreach ($x in $d) { $c = $crcTable[($c -bxor $x) -band 255] -bxor ($c -shr 8) }; return [uint32]($c -bxor 4294967295L) }
function U16($b, $o) { [BitConverter]::ToUInt16($b, $o) }
function U32($b, $o) { [BitConverter]::ToUInt32($b, $o) }

$manifest = [IO.File]::ReadAllText((Join-Path $backup 'zipmanifest.xml'))
$outputs = @{}

foreach ($zipName in $zips) {
  $b = [IO.File]::ReadAllBytes((Join-Path $backup $zipName))
  $e = $b.Length - 22; while ((U32 $b $e) -ne 0x06054b50) { $e-- }
  $count = U16 $b ($e + 10); $cd = U32 $b ($e + 16)
  $out = New-Object IO.MemoryStream; $bw = New-Object IO.BinaryWriter($out)
  $central = New-Object IO.MemoryStream; $cw = New-Object IO.BinaryWriter($central)
  $i = $cd
  for ($k = 0; $k -lt $count; $k++) {
    $nl = U16 $b ($i + 28); $el = U16 $b ($i + 30); $cl = U16 $b ($i + 32)
    $name = [Text.Encoding]::ASCII.GetString($b, $i + 46, $nl)
    $method = U16 $b ($i + 10); $time = U16 $b ($i + 12); $date = U16 $b ($i + 14)
    $crc = U32 $b ($i + 16); $csize = U32 $b ($i + 20); $usize = U32 $b ($i + 24)
    $attrI = U16 $b ($i + 36); $attrE = U32 $b ($i + 38); $lho = U32 $b ($i + 42)
    $dataOff = $lho + 30 + (U16 $b ($lho + 26)) + (U16 $b ($lho + 28))
    $comp = [byte[]]::new($csize); [Array]::Copy($b, $dataOff, $comp, 0, $csize)

    # files\<zip>\<name> replaces that file whole (e.g. a camera mod's CameraPhysics.xml); patches still apply on top
    $override = Join-Path $PSScriptRoot "files\$zipName\$name"
    $hasOverride = -not $RoundTrip -and (Test-Path -LiteralPath $override)
    if (-not $RoundTrip -and ($hasOverride -or $patches[$zipName].ContainsKey($name))) {
      if ($hasOverride) { $text = [IO.File]::ReadAllText($override); "  $zipName/${name}: replaced from files\$zipName" }
      else {
        $ds = New-Object IO.Compression.DeflateStream((New-Object IO.MemoryStream(,$comp)), [IO.Compression.CompressionMode]::Decompress)
        $ms = New-Object IO.MemoryStream; $ds.CopyTo($ms); $ds.Dispose()
        $text = [Text.Encoding]::UTF8.GetString($ms.ToArray())
      }
      foreach ($p in $patches[$zipName][$name]) {
        $hits = [regex]::Matches($text, $p[0]).Count
        if (-not $hits) { throw "pattern not found in ${name}: $($p[0])" }
        if ($p[1] -is [scriptblock]) { $text = [regex]::Replace($text, $p[0], [Text.RegularExpressions.MatchEvaluator]$p[1]) }
        else { $text = [regex]::Replace($text, $p[0], $p[1].Replace('$', '$$')) }
        "  $zipName/${name}: $hits change(s)"
      }
      $raw = (New-Object Text.UTF8Encoding($false)).GetBytes($text)
      $cms = New-Object IO.MemoryStream
      $dz = New-Object IO.Compression.DeflateStream($cms, [IO.Compression.CompressionLevel]::Optimal, $true); $dz.Write($raw, 0, $raw.Length); $dz.Dispose()
      $comp = $cms.ToArray(); $crc = Crc32 $raw; $usize = $raw.Length; $csize = $comp.Length; $method = 8
    }

    $nameBytes = [Text.Encoding]::ASCII.GetBytes($name)
    $newLho = [uint32]$out.Position
    $bw.Write([uint32]0x04034b50); $bw.Write([uint16]10); $bw.Write([uint16]0); $bw.Write([uint16]$method)
    $bw.Write([uint16]$time); $bw.Write([uint16]$date); $bw.Write([uint32]$crc); $bw.Write([uint32]$csize); $bw.Write([uint32]$usize)
    $bw.Write([uint16]$nameBytes.Length); $bw.Write([uint16]0); $bw.Write($nameBytes)
    $newData = [uint32]$out.Position
    $bw.Write($comp)
    $cw.Write([uint32]0x02014b50); $cw.Write([uint16]10); $cw.Write([uint16]10); $cw.Write([uint16]0); $cw.Write([uint16]$method)
    $cw.Write([uint16]$time); $cw.Write([uint16]$date); $cw.Write([uint32]$crc); $cw.Write([uint32]$csize); $cw.Write([uint32]$usize)
    $cw.Write([uint16]$nameBytes.Length); $cw.Write([uint16]8); $cw.Write([uint16]0); $cw.Write([uint16]0)
    $cw.Write([uint16]$attrI); $cw.Write([uint32]$attrE); $cw.Write([uint32]$newLho); $cw.Write($nameBytes)
    $cw.Write([uint16]0x1123); $cw.Write([uint16]4); $cw.Write([uint32]$newData)
    $i += 46 + $nl + $el + $cl
  }
  $bw.Flush(); $cw.Flush()
  $newCd = [uint32]$out.Position; $cdBytes = $central.ToArray(); $bw.Write($cdBytes)
  $bw.Write([uint32]0x06054b50); $bw.Write([uint16]0); $bw.Write([uint16]0); $bw.Write([uint16]$count); $bw.Write([uint16]$count)
  $bw.Write([uint32]$cdBytes.Length); $bw.Write([uint32]$newCd); $bw.Write([uint16]0); $bw.Flush()

  if ($RoundTrip) {
    $rebuilt = $out.ToArray()
    "$zipName round-trip identical: $(($rebuilt.Length -eq $b.Length) -and -not (Compare-Object $rebuilt $b -SyncWindow 0))"
    continue
  }
  $outputs[$zipName] = $out.ToArray()
  $rx = '(path="' + [regex]::Escape('game:\media\' + $zipName.ToLower()) + '"[^>]*?dirstart=")\d+(" dirsize=")\d+'
  if ($manifest -notmatch $rx) { throw "zipmanifest entry not found for $zipName" }
  $manifest = [regex]::Replace($manifest, $rx, "`${1}$newCd`${2}$($cdBytes.Length + 22)")
}
if ($RoundTrip) { return }
foreach ($f in $loose) {
  $text = [IO.File]::ReadAllText((Join-Path $backup $f))
  foreach ($p in $loosePatches[$f]) {
    $hits = [regex]::Matches($text, $p[0]).Count
    if (-not $hits) { throw "pattern not found in ${f}: $($p[0])" }
    if ($p[1] -is [scriptblock]) { $text = [regex]::Replace($text, $p[0], [Text.RegularExpressions.MatchEvaluator]$p[1]) }
    else { $text = [regex]::Replace($text, $p[0], $p[1].Replace('$', '$$')) }
    "  ${f}: $hits change(s)"
  }
  $outputs[$f] = [Text.Encoding]::ASCII.GetBytes($text)
}
# breakables weigh 1/100 of normal (a 100 kg fence part becomes 1 kg), so hitting them barely slows the car.
# Each rigid body record: u64 20, then inverse mass, inverse inertia 3x3, inertia 3x3 (records are not aligned).
foreach ($f in $tracks) {
  $d = [IO.File]::ReadAllBytes((Join-Path $backup $f))
  if ($s.LightBreakables) {
    $n = 0; $i = [Array]::IndexOf($d, [byte]0x14)
    while ($i -ge 0 -and $i -le $d.Length - 84) {
      $im = [BitConverter]::ToSingle($d, $i + 8); $ii = [BitConverter]::ToSingle($d, $i + 12); $in = [BitConverter]::ToSingle($d, $i + 48)
      if ((U32 $d $i) -eq 0x14 -and (U32 $d ($i + 4)) -eq 0 -and $im -gt 0 -and $im -le 100 -and $ii -gt 0 -and $in -gt 0 -and [Math]::Abs($ii * $in - 1) -lt 0.2) {
        for ($k = 0; $k -lt 19; $k++) {
          $o = $i + 8 + 4 * $k; $v = [BitConverter]::ToSingle($d, $o)
          $v = if ($k -lt 10) { $v * 100 } else { $v / 100 }   # inverse mass and inverse inertia up, inertia down
          [Array]::Copy([BitConverter]::GetBytes([single]$v), 0, $d, $o, 4)
        }
        $n++; $i += 84
      } else { $i++ }
      $i = [Array]::IndexOf($d, [byte]0x14, $i)
    }
    "  ${f}: $n breakables made lighter"
  }
  $outputs[$f] = $d
}
# everything built without errors: write it all at once
foreach ($zipName in $outputs.Keys) { [IO.File]::WriteAllBytes((Join-Path $media $zipName), $outputs[$zipName]) }
[IO.File]::WriteAllText((Join-Path $media 'zipmanifest.xml'), $manifest, (New-Object Text.UTF8Encoding($true)))
'applied'
