# Settings window for the FH2 data mods: edit values, Apply writes the game files, Apply & Play also starts the game.
Add-Type -AssemblyName System.Windows.Forms, System.Drawing
[Windows.Forms.Application]::EnableVisualStyles()
$dir = $PSScriptRoot
$settingsPath = Join-Path $dir 'settings.json'
$launcher = Join-Path (Split-Path $dir) 'azimuth.exe'   # this folder sits next to azimuth.exe
$s = Get-Content $settingsPath -Raw | ConvertFrom-Json

$form = New-Object Windows.Forms.Form -Property @{ Text = 'FH2 Mods'; StartPosition = 'CenterScreen'; FormBorderStyle = 'Sizable'; Font = New-Object Drawing.Font('Segoe UI', 10) }
# the settings scroll, the status and buttons stay pinned at the bottom, so they show at any display scale
$scroll = New-Object Windows.Forms.Panel -Property @{ Dock = 'Fill'; AutoScroll = $true }
$grid = New-Object Windows.Forms.TableLayoutPanel -Property @{ ColumnCount = 2; AutoSize = $true; Padding = 12 }
$scroll.Controls.Add($grid)
$footer = New-Object Windows.Forms.TableLayoutPanel -Property @{ ColumnCount = 1; AutoSize = $true; Padding = '12,0,12,8'; Dock = 'Bottom' }
$form.Controls.Add($scroll); $form.Controls.Add($footer)
$inputs = @{}

function Section($text) {
  $l = New-Object Windows.Forms.Label -Property @{ Text = $text; AutoSize = $true; Font = New-Object Drawing.Font('Segoe UI', 10, [Drawing.FontStyle]::Bold); Margin = '0,10,0,2' }
  $grid.Controls.Add($l); $grid.SetColumnSpan($l, 2)
}
function NumberRow($key, $label, $min, $max, $decimals, $step) {
  $grid.Controls.Add((New-Object Windows.Forms.Label -Property @{ Text = $label; AutoSize = $true; Anchor = 'Left'; Margin = '0,6,12,0' }))
  $n = New-Object Windows.Forms.NumericUpDown -Property @{ Minimum = $min; Maximum = $max; DecimalPlaces = $decimals; Increment = $step; Width = 160; ThousandsSeparator = $true }
  $n.Value = [decimal]$s.$key
  $grid.Controls.Add($n); $inputs[$key] = $n
}
function CheckRow($key, $label) {
  $c = New-Object Windows.Forms.CheckBox -Property @{ Text = $label; AutoSize = $true; Checked = [bool]$s.$key }
  $grid.Controls.Add($c); $grid.SetColumnSpan($c, 2); $inputs[$key] = $c
}

Section 'Traffic'
NumberRow 'FreeRoamTrafficPercent' 'Free roam traffic (%)  0 = off, 100 = normal' 0 300 0 10
CheckRow 'RaceTraffic' 'Traffic in races and showcases'
CheckRow 'RoadTripTraffic' 'Traffic in road trips'
if ($null -eq $s.GhostTraffic) { $s | Add-Member GhostTraffic $false }   # settings.json from before this option
CheckRow 'GhostTraffic' 'Ghost cars: drive through every other car (they stay visible)'
Section 'Crashes'
foreach ($k in 'NoCrashSpin', 'SlideOffWalls', 'LightBreakables') { if ($null -eq $s.$k) { $s | Add-Member $k $false } }   # settings.json from before these options
CheckRow 'NoCrashSpin' 'Less spinning and flipping from crashes (reduces it, does not remove it)'
CheckRow 'SlideOffWalls' 'Slide along walls and barriers instead of stopping dead'
CheckRow 'LightBreakables' 'Breakables (fences, signs, bins...) weigh almost nothing, so they barely slow you'
Section 'World keys  (free roam)'
if ($null -eq $s.WorldControls) { $s | Add-Member WorldControls $false }   # settings.json from before this option
CheckRow 'WorldControls' "Time: Ctrl+T midnight, Y sunrise, U midday, I sunset, P freeze clock`nWeather: Ctrl+W rain, R clear`nCtrl+G traffic off/on, N repair, B back on the road`nCtrl+F sky road (experimental, buggy)"
Section 'Money'
NumberRow 'CreditsPerLevel' 'CR per level-up' 0 999999999 0 1000000
NumberRow 'CreditsPerKm' 'CR per km in races' 0 100000000 0 10000
Section 'Skills'
NumberRow 'SkillPointsMultiplier' 'Skill points  (x, 1 = normal)' 1 1000 0 10
NumberRow 'SkillXPMultiplier' 'Skill chain XP  (x, 1 = normal)' 1 1000 0 10
NumberRow 'MaxSkillMultiplier' 'Max chain multiplier  (normal 5)' 1 100 1 1
NumberRow 'SkillChainSeconds' 'Seconds between skills to keep a chain  (0 = game''s own)' 0 60 1 1
CheckRow 'CrashesKeepSkillChain' 'Crashes do not reset the skill chain'
Section 'Packs  (for every car; a pack your garage uses can only be switched off after fitting the stock part)'
foreach ($k in 'PackEngineSwaps', 'PackTrafficCars', 'PackFreeCars') { if ($null -eq $s.$k) { $s | Add-Member $k $false } }   # settings.json from before packs
if ($null -eq $s.PackEngineParts) { $s | Add-Member PackEngineParts 0 }
NumberRow 'PackEngineParts' 'Race+ engine parts: extra torque  (%, 0 = off)' 0 200 0 5
CheckRow 'PackEngineSwaps' 'Engine swaps: every engine fits every car'
CheckRow 'PackTrafficCars' 'Traffic and AI cars for sale in the Autoshow'
CheckRow 'PackFreeCars' 'Free cars: every car in the Autoshow costs 1 CR'
Section 'Overlay  (in-game menu: Insert; boost, nitrous, launch, drift, stunts, UFO, teleport, timers, gauges)'
if ($null -eq $s.StartOverlay) { $s | Add-Member StartOverlay $true }   # settings.json from before the overlay
CheckRow 'StartOverlay' 'Start the overlay with Apply && Play (the game must be in windowed or borderless mode)'
Section 'Cockpit camera'
NumberRow 'CockpitFOV' 'Cockpit FOV  (normal 48)' 30 100 0 1
NumberRow 'SeatOffsetZ' 'Seat back/forward  (m, normal 0)' -1 1 2 0.05
NumberRow 'SeatOffsetY' 'Seat up/down  (m, normal 0)' -1 1 2 0.05

Section 'Car mods  (x1 = stock; grip and brakes act on the race tires and race brakes)'
$carsPath = Join-Path $dir 'cars.json'
$carMods = @{}   # car id -> @{power=..; drag=..; ...}
(Get-Content $carsPath -Raw | ConvertFrom-Json).PSObject.Properties | ForEach-Object {
  $m = @{}; $_.Value.PSObject.Properties | ForEach-Object { $m[$_.Name] = [double]$_.Value }; $carMods[[int]$_.Name] = $m
}
$carList = & python -I (Join-Path $dir 'car_power.py') --list | ConvertFrom-Json
$carNames = @{}; $carList | ForEach-Object { $carNames[[int]$_.id] = $_.name }
$grid.Controls.Add((New-Object Windows.Forms.Label -Property @{ Text = 'Car (type to search)'; AutoSize = $true; Anchor = 'Left'; Margin = '0,6,12,0' }))
$carBox = New-Object Windows.Forms.ComboBox -Property @{ Width = 260; AutoCompleteMode = 'SuggestAppend'; AutoCompleteSource = 'ListItems' }
$carList | ForEach-Object { [void]$carBox.Items.Add($_.name) }   # item index = index in $carList
function SelectedCar { if ($carBox.SelectedIndex -ge 0) { [int]$carList[$carBox.SelectedIndex].id } }
$grid.Controls.Add($carBox)
$carFields = [ordered]@{ power = 'Power  (x)'; drag = 'Air drag  (x, lower = faster)'; downforce = 'Downforce  (x)'; grip = 'Tire grip  (x)'; brakes = 'Brake strength  (x)'; turbo = 'Turbo boost  (x, turbo cars only)'; gearing = 'Gearing  (x, higher = taller gears, more top speed)'; redline = 'Rev limit  (x, up to 2)'; gears = 'Forward gears  (1 = stock gearbox, up to 8)' }
$carInputs = @{}
$loading = $false
foreach ($k in $carFields.Keys) {
  $grid.Controls.Add((New-Object Windows.Forms.Label -Property @{ Text = $carFields[$k]; AutoSize = $true; Anchor = 'Left'; Margin = '0,6,12,0' }))
  $n = New-Object Windows.Forms.NumericUpDown -Property @{ Minimum = 0.1; Maximum = 100; DecimalPlaces = 2; Increment = 0.1; Width = 160; Value = 1; Enabled = $false }
  if ($k -eq 'gears') { $n.Minimum = 1; $n.Maximum = 8; $n.DecimalPlaces = 0; $n.Increment = 1 }   # a count, not a multiplier
  if ($k -eq 'redline') { $n.Maximum = 2 }   # the game crashes above x2
  $n.Add_ValueChanged({
    $id = SelectedCar
    if ($loading -or $null -eq $id) { return }
    $m = @{}
    foreach ($f in $carFields.Keys) { $m[$f] = [double]$carInputs[$f].Value }
    if (@($m.Values | Where-Object { $_ -ne 1 }).Count) { $carMods[$id] = $m } else { $carMods.Remove($id) }
    ShowModded
  })
  $grid.Controls.Add($n); $carInputs[$k] = $n
}
$modded = New-Object Windows.Forms.Label -Property @{ AutoSize = $true; MaximumSize = '520,0'; Margin = '0,6,0,0' }
$grid.Controls.Add($modded); $grid.SetColumnSpan($modded, 2)
function ShowModded {
  $names = $carMods.Keys | ForEach-Object { $carNames[$_] }
  $modded.Text = if ($names) { 'Modded cars: ' + ($names -join ', ') } else { 'Modded cars: none (all stock)' }
}
$carBox.Add_SelectedIndexChanged({
  $id = SelectedCar
  if ($null -eq $id) { return }
  $m = $carMods[$id]
  $script:loading = $true
  foreach ($f in $carFields.Keys) { $carInputs[$f].Enabled = $true; $carInputs[$f].Value = [Math]::Min($carInputs[$f].Maximum, [decimal]$(if ($m -and $null -ne $m[$f]) { $m[$f] } else { 1 })) }   # cars.json from before a field existed: x1
  $script:loading = $false
})
$resetCar = New-Object Windows.Forms.Button -Property @{ Text = 'Set this car back to stock'; AutoSize = $true }
$resetCar.Add_Click({ foreach ($f in $carFields.Keys) { $carInputs[$f].Value = 1 } })
$grid.Controls.Add($resetCar); $grid.SetColumnSpan($resetCar, 2)
# a random car from the save's garage, shown here; the game has no way to be told to switch cars
$randomCar = New-Object Windows.Forms.Button -Property @{ Text = 'Random car from my garage'; AutoSize = $true }
$randomCar.Add_Click({
  $out = & python -I (Join-Path $dir 'car_power.py') --random 2>&1 | Select-Object -Last 1
  $i = 0..($carList.Count - 1) | Where-Object { "$($carList[$_].id)" -eq "$out" } | Select-Object -First 1
  if ($null -eq $i) { $status.ForeColor = 'Firebrick'; $status.Text = "Random car: $out"; return }
  $carBox.SelectedIndex = $i
  $status.ForeColor = 'DarkGreen'; $status.Text = "Drive the $($carList[$i].name): pick it in your garage in game."
})
$grid.Controls.Add($randomCar); $grid.SetColumnSpan($randomCar, 2)
ShowModded

$note = New-Object Windows.Forms.Label -Property @{ AutoSize = $true; MaximumSize = '520,0'; ForeColor = [Drawing.Color]::DimGray; Margin = '0,10,0,0'
  Text = 'Changes apply the next time the game starts. Keep your total CR under about 2,000,000,000: the game may store it as a 32-bit number, and VIP doubles every payout.' }
$grid.Controls.Add($note); $grid.SetColumnSpan($note, 2)

$status = New-Object Windows.Forms.Label -Property @{ AutoSize = $true; MaximumSize = '520,0'; Margin = '0,6,0,0' }
$footer.Controls.Add($status)

$buttons = New-Object Windows.Forms.FlowLayoutPanel -Property @{ AutoSize = $true; Margin = '0,10,0,0' }
$footer.Controls.Add($buttons)
function Button($text, $action) { $b = New-Object Windows.Forms.Button -Property @{ Text = $text; AutoSize = $true; Padding = '8,2,8,2' }; $b.Add_Click($action); $buttons.Controls.Add($b) }

function RunApply($extraArgs) {
  if (Get-Process azimuth -ErrorAction SilentlyContinue) { $status.ForeColor = 'Firebrick'; $status.Text = 'Close the game first.'; return $false }
  $status.ForeColor = 'Black'; $status.Text = 'Working...'; $form.Refresh()
  $out = & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $dir 'apply.ps1') @extraArgs 2>&1
  if ($LASTEXITCODE -ne 0) { $status.ForeColor = 'Firebrick'; $status.Text = "Failed: $($out | Select-Object -Last 1)"; return $false }
  $carArgs = @($extraArgs | ForEach-Object { if ($_ -eq '-Restore') { '--restore' } else { $_ } })
  $carOut = & python -I (Join-Path $dir 'car_power.py') @carArgs 2>&1
  if ($LASTEXITCODE -ne 0) { $status.ForeColor = 'Firebrick'; $status.Text = "Car mods failed: $($carOut | Select-Object -Last 1)"; return $false }
  $status.ForeColor = 'DarkGreen'; $status.Text = "Done: $($out | Select-Object -Last 1); cars: $($carOut | Select-Object -Last 1)"; return $true
}
function Save {
  foreach ($k in $inputs.Keys) { $v = $inputs[$k]; $s.$k = if ($v -is [Windows.Forms.CheckBox]) { $v.Checked } else { [double]$v.Value } }
  $s | ConvertTo-Json | Set-Content $settingsPath -Encoding UTF8
  $out = [ordered]@{}; foreach ($id in $carMods.Keys | Sort-Object) { $out["$id"] = $carMods[$id] }
  [IO.File]::WriteAllText($carsPath, ($out | ConvertTo-Json), (New-Object Text.UTF8Encoding($false)))  # python's json wants no BOM
}

Button 'Apply' { Save; [void](RunApply @()) }
$impulse = Join-Path (Split-Path $launcher) 'SimpleImpulseH2'   # trigger rumble for Xbox controllers; waits for the game, then hooks it
Button 'Apply && Play' { Save; if (RunApply @()) {
  Start-Process $launcher -WorkingDirectory (Split-Path $launcher)
  if ($s.StartOverlay -and (Test-Path $overlay)) { Start-Process $overlay -WorkingDirectory $dir }
  if (Test-Path "$impulse\FH2TriggerBridge.dll") { Start-Process python "`"$impulse\source\fh2_trigger.py`"" -WorkingDirectory $impulse -WindowStyle Minimized }
  $form.Close() } }
Button 'Restore originals' { [void](RunApply @('-Restore')) }
$overlay = Join-Path $dir 'FH2 Mods Overlay.exe'
if (Test-Path $overlay) { Button 'Overlay' { Start-Process $overlay -WorkingDirectory $dir; $status.ForeColor = 'DarkGreen'; $status.Text = 'Overlay started: press Insert in the game.' } }

$form.Add_Load({
  # fit the content, but never beyond the screen; anything that doesn't fit scrolls
  $area = [Windows.Forms.Screen]::FromControl($form).WorkingArea
  $chrome = $form.Size - $form.ClientSize
  $w = [Math]::Max($grid.PreferredSize.Width, $footer.PreferredSize.Width) + [Windows.Forms.SystemInformation]::VerticalScrollBarWidth
  $h = $grid.PreferredSize.Height + $footer.PreferredSize.Height
  $form.Size = New-Object Drawing.Size([Math]::Min($w + $chrome.Width, $area.Width), [Math]::Min($h + $chrome.Height, $area.Height))
  $form.MinimumSize = New-Object Drawing.Size(300, ($footer.PreferredSize.Height + $chrome.Height + 80))
  $form.Location = New-Object Drawing.Point(($area.X + ($area.Width - $form.Width) / 2), ($area.Y + ($area.Height - $form.Height) / 2))
})
$form.Add_Shown({ $scroll.AutoScrollPosition = New-Object Drawing.Point(0, 0) })   # focusing the first input scrolls it into view
[void]$form.ShowDialog()
