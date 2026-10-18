# Read-only check for "Apply failed": compares this PC's game files and the tool's backups with fingerprints of the
# genuine FH2 1.0.0.64 files, and lists folders, versions and settings. Changes nothing. Writes diagnose.txt here.
Add-Type -AssemblyName System.IO.Compression.FileSystem
$report = New-Object Collections.Generic.List[string]
function Say($text, $color = 'Gray') { $report.Add($text); Write-Host $text -ForegroundColor $color }
$sha = [Security.Cryptography.SHA256]::Create()
function Hex($bytes) { -join ($bytes | ForEach-Object { $_.ToString('x2') }) }
function FileHash($path) { $f = [IO.File]::OpenRead($path); try { Hex $sha.ComputeHash($f) } finally { $f.Dispose() } }
$known = ConvertFrom-Json @'
{"exe":"3ec04c6163fb5614e3d351ba0570d39b846041a257715d6340eceb03ccb7f0a3","files":{"media/Camera.zip":{"sha":"b059c1c56a0738fa3d54bb0f5b0cad510874c21072dca8c86bde2c9bd5da230e","entries":{"321Cams.xml":"cceab123474f6418","bokehtypes.xml":"5c9dcd3a4d163c6f","BuyCar.xml":"4e38f11bfa80f3a5","CameraGroups.xml":"d9104cdc63226e3a","CameraLenses.xml":"db9a01eaa8505b64","CameraPhysics.xml":"284b2380a3563923","CameraPhysicsSansEffects.xml":"cd338bd7d33cdec2","CameraSettings.ini":"730701ef97101257","CareerMenu.xml":"3bb90986cf89b435","CarRelativeCams.xml":"f3b9091311e28be3","carrevealcams.xml":"5ca57abec7f8ffdc","ColorSelectCams.xml":"00e62c9603a7aad6","HomeSpaceCams.xml":"af50dddb1b0d05e7","homespacecams_small.xml":"6886c9d2fda5d9ae","IE_Autovista.xml":"8c511aace92bcabf","ShowroomPhotomode.ini":"de11823a390f515b","UICarTargetCameraPresets.xml":"922b5a7600757f9b","UIFlowCams.xml":"4a5396027b8caac0","WalkaroundCamera.ini":"b829e8b9aaf7cff5"}},"media/GameTunableSettings.zip":{"sha":"4af33e849cc4a06d0b70a5b0eb585786de96621422b02c3ce32d25cd57bb4327","entries":{"321Tuning.xml":"82bf8633e6429dbc","ANNARecommendations.xml":"7b6c0cb77849d95e","AudioPresets.xml":"a684e28722f0f683","Bonuses.xml":"601c89ca2e7b5995","CareerPlayerLevels.xml":"ca518b2c88a33a1c","CareerRaceModes.xml":"97448afc29aa10f5","CastellettoToNiceIETime.xml":"338fb5281032a154","CastellettoToNiceIEWeather.xml":"f05f2ad58ec93176","ChampionshipGridOrdering.xml":"aaadbf09c3f13de5","collidablepresets.xml":"0f9dcc9bd30f3187","DestinationCreatorWeightings.xml":"2ed4a2aa5a6791c2","DestinationStats.xml":"4f5c976b02294751","E3TimeSettings.xml":"172e8facd0d900b1","E3WeatherSettings.xml":"11e44a52d30bd831","EventSkillsXPThresholds.xml":"b04f53b16a922247","GameOptionPresets.xml":"b2dda0bdc102e7c1","GameOptionPresets_FreePlay.xml":"8f351b651c1b3c09","GameTunableSettings.ini":"789e27aeadf9b126","HeroDriveTimeSettings.xml":"09e28f06b414a007","HeroDriveWeatherSettings.xml":"fd1b5a694e804066","LiveryStressOptions/MaxLayers_Deep.xml":"1cc87bcec37d070f","LiveryStressOptions/MaxLayers_HighVertCount.xml":"b9ebd8be3c396c3d","LiveryStressOptions/MaxLayers_MfrDecalsOnly.xml":"a269ce27ad75787b","LiveryStressOptions/MaxLayers_Shallow.xml":"d4f4235af49cac8c","LiveryStressOptions/MaxLayers_VinylsOnly.xml":"4d0eb5b93e6437e0","NiceToSisteronIETime.xml":"266305b3764245c3","NiceToSisteronIEWeather.xml":"f05f2ad58ec93176","PhotoChallengeRanks.xml":"5261cd6d9afc4aba","PlayerXPLevels.xml":"22abe701dcdae782","RaceFeats.xml":"55875028dd12ec1a","RewindBonuses.xml":"155951b8b581b986","SkillChainXPRewards.xml":"385b12e84141d51d","Skills.xml":"618a482588b12017","TheIsandIEWeather.xml":"b981b6e058b9b0d5","TheIslandIETime.xml":"fa6eba9b356303ad"}},"media/EntityModel.zip":{"sha":"ebc3182654d899ec46ecbebeb3d02ddc51276b2fca3f7af07b82f7511cf330fc","entries":{"AI/AITimes.xml":"4621223174a84288","AI/CarLoadingSettings.xml":"95e3be0ed6efeb70","AI/Densities.xml":"2289579e243bdd2a","AI/DensitiesIEDriveToFestival.xml":"7bfe096598ce2b3f","AI/DensitiesRace.xml":"7b3b41fdf7ec49b9","AI/DensitiesRoadTrip.xml":"12e4dd62c9c80372","AI/Difficulty.xml":"6e91f833959688f7","AI/RacingAI.xml":"515b1055aa32a2eb","barn_finds.xml":"ae0892d1f66a8f87","bucketlist_slots.xml":"12230791310b6031","car_meets.xml":"f056cc95d8848403","car_meets_island.xml":"d75d6311405ba726","destination_hubs.xml":"5db44f3238833079","discount_boards.xml":"de077008bc9fefad","discount_boards_hidden.xml":"97d55a0bbd342e82","discount_boards_island.xml":"0e7951d2c2c886c8","discount_boards_island_hidden.xml":"81c36f5852ee1878","entities_global.xml":"175d257b683d80bc","entities_initial_experience_island.xml":"5006a8fd16421235","entities_initial_experience_island_championship.xml":"8e1256163b342219","entities_offline_freeroam.xml":"dd921a5c3736644b","entities_offline_freeroam_island.xml":"b41ecb82ba8e5a07","entities_route_boot.xml":"2c33c89164955c4d","entities_route_boot_to_car_meet.xml":"d986f34d96015e4a","entities_route_bucket_list.xml":"127498a6355c6570","entities_route_bucket_list_island.xml":"6d9302779c909e45","entities_route_offline_car_meet_island.xml":"fc71a652261ca65f","entities_route_offline_car_meet_ita_coast.xml":"0e4856a8afb48301","entities_route_offline_car_meet_montellino.xml":"4877c6cbd5dfda3e","entities_route_offline_car_meet_nice.xml":"b8ed07644d9bd177","entities_route_offline_car_meet_saintmartin.xml":"ed00b2e1dd92ec87","entities_route_offline_car_meet_sangimignano.xml":"ee88d711c15e5ddf","entities_route_offline_car_meet_sisteron.xml":"926da48c3566ac99","entities_route_offline_freeroam.xml":"97c52ef1a19bbb34","entities_route_offline_freeroam_e3.xml":"3da35c6982a54510","entities_route_offline_freeroam_first_boot.xml":"88e36f44c37c9ba8","entities_route_offline_freeroam_island.xml":"f1ef6e5d96ba0c69","entities_route_offline_freeroam_island_first_boot.xml":"1432dfe1192e50e0","entities_route_online_car_meet_island.xml":"a28517d6336f4532","entities_route_online_car_meet_ita_coast.xml":"fd506386c5ebc5cd","entities_route_online_car_meet_montellino.xml":"21d132a6c67912a3","entities_route_online_car_meet_nice.xml":"b10dac2c83fbcd52","entities_route_online_car_meet_saintmartin.xml":"4687101f70ab3f99","entities_route_online_car_meet_sangimignano.xml":"73a3ea2d808892ee","entities_route_online_car_meet_sisteron.xml":"01221a8faa1a9393","entities_route_online_freeroam.xml":"a5094026dda0da9d","entities_route_online_freeroam_island.xml":"723500dca345299e","entities_route_race.xml":"3efa9db7b74a97b0","entities_route_road_trip.xml":"72623a4c7e9347ba","entities_route_showcase.xml":"967ca4aef47a2147","gameplay_triggers_route_2251.xml":"38a1de3ac434e5ae","gameplay_triggers_route_2646.xml":"ea43ae78afbe17a4","gameplay_triggers_route_3333.xml":"daff2e59991ea21e","gameplay_triggers_route_9001.xml":"de4a029e54d9fe7b","gameplay_triggers_route_9002.xml":"de4a029e54d9fe7b","gameplay_triggers_route_9003.xml":"f4cef1e72848e362","gameplay_triggers_route_9012.xml":"756810f9a5c90131","gameplay_triggers_route_9013.xml":"ea43ae78afbe17a4","gameplay_triggers_route_9014.xml":"ea43ae78afbe17a4","initial_experience.xml":"569381dd36cd8e5f","load_event_from_cmdline.xml":"0405472c7a6744bf","load_replay_from_cmdline.xml":"f677472e66cdcf52","load_route_from_cmdline.xml":"679277825c4e4808","load_route_from_cmdline_autodrive_ai_times.xml":"b7b02e2c3b2f8477","modules.xml":"530006f65d9bc440","Network/network_carmeet.xml":"63953e0c54c31c55","Network/network_freeroam.xml":"4fefeb4fc7d72209","Network/network_matchmaking_carmeet.xml":"b445ed7f55299186","Network/network_matchmaking_carmeet_directip.xml":"5f111471f632ee9f","Network/network_matchmaking_cleanup.xml":"67be1ca28092e2cb","Network/network_matchmaking_freeroam.xml":"8ffb63cafc1fd6e3","Network/network_matchmaking_freeroam_directip.xml":"df138ca6da24bae2","Network/network_matchmaking_freeroam_from_car_meet.xml":"5683489ec71fe406","Network/network_matchmaking_game_party_game_session_ready.xml":"54d26257d76252c2","Network/network_matchmaking_roadtrip.xml":"04f7546232d75bec","Network/network_matchmaking_roadtrip_directip.xml":"21331d112e06090a","Network/network_matchmaking_roadtrip_from_car_meet.xml":"5683489ec71fe406","Network/network_online_road_trip_activations.xml":"258f7c8ed1824398","Network/network_roadtrip.xml":"baa101d073f41e95","paintshop_route.xml":"b980e5298b5921e6","route_join_locations.xml":"8c0a786a6bc23e10","route_join_locations_route_3333.xml":"5cccfb5ec31cb6d5","route_join_locations_route_9012.xml":"306d2adb992d6ec7","route_join_locations_route_9013.xml":"3486de383ad35a69","route_join_locations_route_9014.xml":"3486de383ad35a69","speed_cameras.xml":"82483baa7c9cae70","speed_cameras_island.xml":"8392a463039578d5","Templates/route_join_templates.xml":"d254aa80fc967535","Templates/speed_camera_templates.xml":"9a1de06ac2003e81","Templates/templates_barn_find.xml":"f7c1d0f5a239fd65","Templates/templates_bucket_list.xml":"26fd6f3731361f0d","Templates/templates_cars.xml":"f6733846d19baf76","Templates/templates_car_meets.xml":"b55c990cff4fff20","Templates/templates_car_meets_activation.xml":"fe6c96f2faa00c6e","Templates/templates_car_meets_slot.xml":"c35867044adfd35e","Templates/templates_discount_boards.xml":"e56ea44614966038","Templates/templates_e3.xml":"c4d08c1915bf359b","Templates/templates_gameplay_triggers.xml":"72cd1468a8cd871c","Templates/templates_global.xml":"fc6f21ef85c41dac","Templates/templates_locations.xml":"7349e38d268d9b68","Templates/templates_network.xml":"ae8da12ff5f63988","Templates/templates_playground_games.xml":"e705391a88415dd8","Templates/templates_races.xml":"8bb22363ac77a7fe","Templates/templates_race_encounters.xml":"c4a40088b5fe3f72","Templates/template_files.xml":"05102c558140e8e3","Templates/time_of_day_templates.xml":"0e42ba113e269f9b","Templates/use_case_templates.xml":"4bbdc839639e9c80","workshop_route.xml":"b8791af646b69c27"}},"media/stateflow.zip":{"sha":"e63694b98d0b8859b19f7673d1cbc4d034f32fc7c394fa95be42e1087418effd","entries":{"anna_flow.xml":"ecfa55c1d592f2e6","autoshow.xml":"d3dd0eda37d1bb05","barn_finds.xml":"eff2918ec703bf52","boot.xml":"b0f32ebf4f71c4f9","bucket_list_activation.xml":"dff9412521766f16","bucket_list_activity.xml":"380d8dd21f39e45f","career_roadtrip_event.xml":"9bc227d027282275","career_roadtrip_event_activation.xml":"d2d4fdccc0431d18","career_roadtrip_event_e3.xml":"ee474f1c91b8ca84","career_states.xml":"9266099b80c9f4c9","career_swap.xml":"53a702d51e657a92","car_meet.xml":"ef9bc829559fbc3a","car_meet_slot.xml":"11157ddd294a42f5","car_reload.xml":"41f08965dc9b9e01","club.xml":"628b99b49c7cad6a","cutscene_testing.xml":"2878f558feeb0782","debug_car_select_flow.xml":"0249c83527c37b92","destination_hub.xml":"c243fa7c99d3e62f","e3_car_select.xml":"c33388ad94a7b343","e3_shortcuts.xml":"bedce38f9d42381c","fast_travel.xml":"582d544ce13a9bf9","free_roam_start.xml":"b5b0dda19bf00abc","free_roam_start_e3.xml":"864431b135881810","free_roam_start_ie.xml":"3e95cb94591b6a3b","free_roam_start_ie_island.xml":"af2c0f531d0a2476","garage.xml":"1f9d03f821fec753","global.xml":"9153876e5ff24a2a","hand_of_god.xml":"80c0e2005f067eea","install_complete.xml":"56ac4794e4a24131","in_game_ui.xml":"6859b43739d71e1f","island_ie.xml":"d367154bbbce892c","load_route_from_cmdline.xml":"e07e9aab1f41277e","load_route_from_cmdline_autodrive_ai_times.xml":"c4871036e9754bc7","network.xml":"d3f20dc8eb1cf852","network_car_meet_states.xml":"cd68d56b3a264c14","network_freeroam_states.xml":"b94f0b069e0bc132","network_matchmaking.xml":"42c35b094a004865","network_party_states.xml":"712326f4daa608fe","network_roadtrip_states.xml":"fbeb3c631a3d9281","paintshop.xml":"32f0597f6bf8ff63","panama_ie.xml":"6f77ca783e95cacc","pause.xml":"f82dae09ca6ee527","player_house.xml":"6115f54b593e8bf6","post_race_flow.xml":"98ce5fcd764d8feb","race_debug_activation.xml":"39783305e0ea3639","race_encounters.xml":"442859b12fdf9652","race_flow.xml":"3e677ae2aa8ca669","race_flow_e3.xml":"53b1b774421a2cae","race_trigger_activation.xml":"609c967f708414a6","rich_presence.xml":"951c9b72eeb2b721","rivals_everywhere.xml":"04eb3af7a195e30a","route_switch.xml":"b7b4662fab09f333","skill_tree.xml":"e83b3f26a1bd505d","speed_cameras_common.xml":"34e7972c7c4d2afd","speed_camera_leaderboard_trigger.xml":"8068655b0dd8e5bd","speed_traps.xml":"7227a92e6d07becf","speed_zones.xml":"3ca8daeacde6057b","TestFlow.xml":"b9db7db86f2b6f7b","TestFlow2.xml":"102957dbd2097e07","tombola.xml":"2e892429817df532","track_swap_flow.xml":"4a4eaf3de2972aab","tutorial_flow.xml":"8e337127a61bd87e","ui_screens.xml":"3be9a14787447469","workshop.xml":"563d25078166fd9e"}},"media/physics/PhysicsSettings.ini":{"sha":"f299c188991af11e0b404ae9808ac3e32b0f1cdc19c753a90be20ba20a24e169"},"media/physics/surfaceTypes.xml":{"sha":"01ffe197676fac08cd288c54da3ae97f369ee766bd3549659eeb10d391ce0748"},"media/tracks/anthem/physicsdefinitions.bin":{"sha":"8847d7c0b8c9be7da39d3799f30713111bf6c7149079dd4539449b0b57408a1b"},"media/tracks/homespace02/physicsdefinitions.bin":{"sha":"1474a898b04cc694f004edd5395916a09e3f74f80c3dd4c946ae3782f8ecba70"},"media/zipmanifest.xml":{"sha":"773e6ae5d5de2eab8776c757098768918d6ad5c187633b555bc5d5c567ebaec4"},"media/gamedbRC.slt":{"sha":"4d50013dc7705de05a074e4421679e16a3a1eda0816841861b0b7139ecf45517"},"DLC/StormIsland_1.0.0.36_neutral__8wekyb3d8bbwe/media/tracks/test03/physicsdefinitions.bin":{"sha":"cfee8b7b7ffbde14a0ed180a498e52ad2a6f0a61e55e54fb4a05f6a2fa05e2b5"}}}
'@

# a file is "genuine" (untouched), "CHANGED" (with the zip entries that differ) or "missing"
function Check($path, $want) {
  if (-not (Test-Path -LiteralPath $path)) { return 'missing' }
  $when = (Get-Item -LiteralPath $path).LastWriteTime.ToString('yyyy-MM-dd HH:mm')
  if ((FileHash $path) -eq $want.sha) { return "genuine   ($when)" }
  $detail = ''
  if ($want.entries) {
    try {
      $zip = [IO.Compression.ZipFile]::OpenRead($path)
      $changed = foreach ($e in $zip.Entries) {
        if ($e.FullName.EndsWith('/')) { continue }
        $s = $e.Open(); $h = (Hex $sha.ComputeHash($s)).Substring(0, 16); $s.Dispose()
        $orig = $want.entries.($e.FullName)
        if (-not $orig) { "$($e.FullName) (added)" } elseif ($orig -ne $h) { $e.FullName }
      }
      $zip.Dispose()
      $detail = if ($changed) { "; changed inside: $($changed -join ', ')" } else { '; same contents, repacked' }
    } catch { $detail = "; cannot open as zip: $($_.Exception.Message)" }
  }
  "CHANGED   ($when)$detail"
}

Say "FH2 Mods diagnose   $(Get-Date -Format 'yyyy-MM-dd HH:mm')" 'White'
Say "Windows $([Environment]::OSVersion.Version), PowerShell $($PSVersionTable.PSVersion), language $((Get-Culture).Name)"
$py = Get-Command python -ErrorAction SilentlyContinue
$pyVersion = if ($py) { try { (& python --version 2>&1) -join ' ' } catch { "$_" } } else { 'not found' }
Say "Python: $pyVersion   $(if ($py) { $py.Source })"

$root = Split-Path $PSScriptRoot
Say ''
Say "Tool folder:    $PSScriptRoot"
Say "Azimuth folder: $root"
$exe = Join-Path $root 'azimuth.exe'
Say "azimuth.exe:    $(if (Test-Path $exe) { 'found, version ' + (Get-Item $exe).VersionInfo.FileVersion } else { 'NOT FOUND (the FH2 Mods folder must sit next to azimuth.exe)' })"
$azIni = Join-Path $root 'data\azimuth.ini'
$azText = if (Test-Path $azIni) { [IO.File]::ReadAllText($azIni) } else { '' }
$folders = @([regex]::Matches($azText, '(?m)^(?:folder|game_folder)=(.+?)\s*$') | ForEach-Object { $_.Groups[1].Value.Trim('"') })
Say "azimuth.ini:    $(if ($azText) { 'game folder(s): ' + ($folders -join ' | ') } else { 'NOT FOUND' })"
Say 'Top of Azimuth folder:'
Get-ChildItem $root | Sort-Object LastWriteTime | ForEach-Object { Say ('   {0:yyyy-MM-dd HH:mm}  {1}{2}' -f $_.LastWriteTime, $_.Name, $(if ($_.PSIsContainer) { '\' })) }

# the game, found the same way as apply.ps1
$candidates = $folders + @(Get-ChildItem $root -Directory | ForEach-Object FullName)
function IsFH2($manifest) { $appx = Join-Path (Split-Path (Split-Path $manifest)) 'AppxManifest.xml'; (Test-Path -LiteralPath $manifest) -and (-not (Test-Path -LiteralPath $appx) -or [IO.File]::ReadAllText($appx) -match '<Identity[^>]*Name="Anthem"') }
Say 'Each azimuth.ini folder:'
foreach ($f in $folders | Select-Object -Unique) {
  $dir = [IO.Path]::Combine($root, $f); $mount = Join-Path $dir 'Mount'; $appx = Join-Path $mount 'AppxManifest.xml'
  $id = if (Test-Path -LiteralPath $appx) { [regex]::Match([IO.File]::ReadAllText($appx), '<Identity[^>]*Name="([^"]+)"[^>]*Version="([^"]+)"').Groups[1..2].Value -join ' ' } else { 'no Mount\AppxManifest.xml' }
  Say "   $f  ->  $(if (-not (Test-Path -LiteralPath $dir)) { 'FOLDER MISSING' } elseif (-not (Test-Path -LiteralPath $mount)) { 'no Mount folder; contains: ' + ((Get-ChildItem -LiteralPath $dir | Select-Object -First 12 | ForEach-Object Name) -join ', ') } else { "$id, Mount\media\zipmanifest.xml $(if (Test-Path -LiteralPath "$mount\media\zipmanifest.xml") { 'found' } else { 'MISSING' })" })"
}
$manifestFile = $candidates | ForEach-Object { Join-Path ([IO.Path]::Combine($root, $_)) 'Mount\media\zipmanifest.xml' } | Where-Object { IsFH2 $_ } | Select-Object -First 1
Say ''
if (-not $manifestFile) { Say 'GAME NOT FOUND: no FH2 (Anthem) Mount\media\zipmanifest.xml in the azimuth.ini folders or beside azimuth.exe' 'Red' }
else {
  $media = Split-Path $manifestFile; $mount = Split-Path $media
  Say "Game folder:    $(Split-Path $mount)"
  $gameExe = Join-Path $mount 'forza_durango_release_final.exe'
  $appx = Join-Path $mount 'AppxManifest.xml'
  $version = if (Test-Path $appx) { [regex]::Match([IO.File]::ReadAllText($appx), '<Identity[^>]*Version="([^"]+)"').Groups[1].Value }
  Say "Game version:   $version (Azimuth needs 1.0.0.64)"
  Say "Game exe:       $(if (-not (Test-Path $gameExe)) { 'missing' } elseif ((FileHash $gameExe) -eq $known.exe) { 'genuine 1.0.0.64' } else { 'DIFFERENT from the genuine 1.0.0.64 exe' })"

  $backupRoot = Join-Path $root 'backup_original'
  Say ''
  Say "backup_original (the tool's copy of the originals, made at the first Apply): $(if (Test-Path "$backupRoot\media") { 'created ' + (Get-Item "$backupRoot\media").CreationTime.ToString('yyyy-MM-dd HH:mm') } else { 'none yet' })" 'White'
  Say 'After an Apply the GAME copies are expected to be CHANGED (that is the mod). The BACKUP copies must all be genuine.'
  $badBackups = @()
  foreach ($p in $known.files.PSObject.Properties) {
    $rel = $p.Name
    $game = if ($rel -eq 'media/gamedbRC.slt') { Join-Path $media 'stripped\gamedbRC.slt' } else { Join-Path $mount $rel }
    $backup = Join-Path $backupRoot $rel
    $b = Check $backup $p.Value
    if ($b -like 'CHANGED*') { $badBackups += $rel }
    Say ''
    Say $rel 'White'
    Say "   game:   $(Check $game $p.Value)"
    Say "   backup: $b" $(if ($b -like 'CHANGED*') { 'Red' } else { 'Gray' })
  }

  # the value the first failing pattern looks for
  Say ''
  foreach ($z in @(@('game', (Join-Path $media 'GameTunableSettings.zip')), @('backup', (Join-Path $backupRoot 'media\GameTunableSettings.zip')))) {
    if (-not (Test-Path $z[1])) { continue }
    try {
      $zip = [IO.Compression.ZipFile]::OpenRead($z[1])
      $e = $zip.Entries | Where-Object Name -eq 'PlayerXPLevels.xml' | Select-Object -First 1
      $r = New-Object IO.StreamReader($e.Open()); $xml = $r.ReadToEnd(); $r.Dispose(); $zip.Dispose()
      $values = [regex]::Matches($xml, 'CreditRewardPerLevel="([^"]*)"') | ForEach-Object { $_.Groups[1].Value } | Select-Object -Unique
      Say "CR per level-up in $($z[0]) PlayerXPLevels.xml: $($values -join ', ')   (genuine: 0)"
    } catch { Say "CR per level-up in $($z[0]): cannot read ($($_.Exception.Message))" }
  }
}

# ExtraOptions
Say ''
Say 'data\bin, and winmm.dll next to azimuth.exe:' 'White'
@(Get-ChildItem (Join-Path $root 'data\bin') -Filter '*.dll*' -ErrorAction SilentlyContinue) + @(Get-Item (Join-Path $root 'winmm.dll') -ErrorAction SilentlyContinue) | ForEach-Object {
  $fork = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($_.FullName)).Contains('FH2_ExtraOptions')
  Say ('   {0,-32} {1,9} bytes  {2:yyyy-MM-dd HH:mm}{3}' -f $_.Name, $_.Length, $_.LastWriteTime, $(if ($fork) { '  (ExtraOptions)' }))
}

# other Azimuth folders on this PC: an earlier Apply from one of them may have changed the same game
Say ''
Say 'Other Azimuth folders (an Apply from another one changes the same game, but keeps its backup there):' 'White'
$profileDir = [Environment]::GetFolderPath('UserProfile')
$places = @((Split-Path $root), "$profileDir\Desktop", "$profileDir\OneDrive\Desktop", "$profileDir\Downloads", "$profileDir\Documents") | Select-Object -Unique | Where-Object { Test-Path $_ }
$others = foreach ($place in $places) {
  Get-ChildItem $place -Filter azimuth.exe -Recurse -Depth 3 -ErrorAction SilentlyContinue | ForEach-Object DirectoryName | Where-Object { $_ -ne $root }
}
$others = @($others | Select-Object -Unique)
if (-not $others) { Say '   none found' }
foreach ($o in $others) {
  $oIni = Join-Path $o 'data\azimuth.ini'
  $oFolders = if (Test-Path $oIni) { @([regex]::Matches([IO.File]::ReadAllText($oIni), '(?m)^(?:folder|game_folder)=(.+?)\s*$') | ForEach-Object { $_.Groups[1].Value.Trim('"') }) -join ' | ' }
  $oBackup = if (Test-Path "$o\backup_original\media") { 'yes, created ' + (Get-Item "$o\backup_original\media").CreationTime.ToString('yyyy-MM-dd HH:mm') } else { 'no' }
  Say "   $o"
  Say "      game folder(s): $oFolders;  FH2 Mods: $(Test-Path "$o\FH2 Mods");  backup_original: $oBackup"
}

Say ''
if ($manifestFile -and $badBackups) {
  Say "The tool's backup holds files another mod changed: $($badBackups -join ', ')." 'Yellow'
  Say 'That is fine (Apply builds on top of them) unless Apply failed with "pattern not found" in one of them. Then: put the genuine files back in the game (from the original game download, or copy backup_original from the other Azimuth folder), delete those files from backup_original, and Apply again.' 'Yellow'
} elseif ($manifestFile) { Say 'The backups are genuine, so they are not the cause. Send diagnose.txt.' 'Green' }

$reportPath = Join-Path $PSScriptRoot 'diagnose.txt'
[IO.File]::WriteAllLines($reportPath, $report)
Write-Host ''
Write-Host "Saved to $reportPath - send this file." -ForegroundColor Cyan
