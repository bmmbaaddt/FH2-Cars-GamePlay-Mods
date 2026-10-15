# FH2 mod patcher. Rebuilds media zips from the untouched originals in backup_original\media,
# applying the values in settings.json, in the game's own zip layout:
# local headers without extras, central entries with extra 0x1123 = data offset,
# and media\zipmanifest.xml updated with each zip's new central directory position.
# -RoundTrip: rebuild unchanged and compare to the originals (proves the writer is byte-exact).
# -Restore: put the original files back.
param([string]$Settings = (Join-Path $PSScriptRoot 'settings.json'), [switch]$RoundTrip, [switch]$Restore)
$ErrorActionPreference = 'Stop'
# this folder sits next to azimuth.exe; the game is the folder under it with Mount\media
$root   = Split-Path $PSScriptRoot
$manifestFile = Get-ChildItem $root -Directory | ForEach-Object { Join-Path $_.FullName 'Mount\media\zipmanifest.xml' } | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $manifestFile) { throw "game not found: put the FH2 Mods folder next to azimuth.exe" }
$media  = Split-Path $manifestFile
$gameFolder = Split-Path -Leaf (Split-Path (Split-Path $media))
$backup = Join-Path $root 'backup_original\media'
if (Get-Process azimuth -ErrorAction SilentlyContinue | Where-Object { $_.Path -like "$root\*" }) { throw 'close the game first' }
$zips   = 'Camera.zip', 'GameTunableSettings.zip', 'EntityModel.zip'
New-Item -ItemType Directory -Force $backup | Out-Null
foreach ($f in $zips + 'zipmanifest.xml') {
  if (-not (Test-Path (Join-Path $backup $f))) { Copy-Item (Join-Path $media $f) (Join-Path $backup $f) }   # backup = untouched original
}
if ($Restore) {
  foreach ($f in $zips + 'zipmanifest.xml') { Copy-Item -Force (Join-Path $backup $f) (Join-Path $media $f) }
  'restored original files'; return
}

# ExtraOptions finds the game through [launcher] game_folder, which Azimuth 0.9.9 no longer writes (it may drop it again on a settings change).
$azIni = Join-Path $root 'data\azimuth.ini'
$azText = if (Test-Path $azIni) { [IO.File]::ReadAllText($azIni) } else { '' }
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
# everything built without errors: write it all at once
foreach ($zipName in $outputs.Keys) { [IO.File]::WriteAllBytes((Join-Path $media $zipName), $outputs[$zipName]) }
[IO.File]::WriteAllText((Join-Path $media 'zipmanifest.xml'), $manifest, (New-Object Text.UTF8Encoding($true)))
'applied'
