# Single instrumented Formula90s HUD session.
#
# Runs the physics integration scenario (deterministic launch, gear changes, braking and
# cornering) so the HUD is exercised with real values, captures frames and keeps the
# runtime log. `-Skip` forwards OPEN_FORMULA_HUD_SKIP for component bisection.
param(
    [string]$Skip = '',
    [int]$Seconds = 26,
    [string]$Tag = 'session'
)
$ErrorActionPreference = 'Stop'
$root = 'D:/open-formula'
$capture = Join-Path $root "implementation/reports/hud_formula90s/$Tag"
New-Item -ItemType Directory -Force -Path $capture | Out-Null
$frames = Join-Path $root 'implementation/reports/hud_formula90s/frames'
New-Item -ItemType Directory -Force -Path $frames | Out-Null
$config = Join-Path $root 'distribution/user-settings/config/rust_physics.xml'
$configBefore = [IO.File]::ReadAllBytes($config)
if (Get-Process -Name speed-dreams-2 -ErrorAction SilentlyContinue) { throw 'An existing game session is running.' }
$process = $null
$env:OPEN_FORMULA_HUD_DIAG = '1'
$env:OPEN_FORMULA_CAPTURE_DIR = $frames
$env:OPEN_FORMULA_CAPTURE_FRAMES = '240,420,600,780,960,1140'
$env:OPEN_FORMULA_HUD_SKIP = $Skip
try {
    $text = [IO.File]::ReadAllText($config)
    $text = $text.Replace('name="integration scenario" val="0"', 'name="integration scenario" val="1"')
    $text = $text.Replace('</section>', '<attnum name="open garage at session start" val="0" /></section>')
    [IO.File]::WriteAllText($config, $text)
    $process = Start-Process -FilePath "$root/distribution/bin/speed-dreams-2.exe" `
        -ArgumentList '-s', 'williams-fuji-practice', '-lc', "$root/distribution/user-settings" `
        -WorkingDirectory "$root/distribution" -PassThru `
        -RedirectStandardOutput "$capture/stdout.log" -RedirectStandardError "$capture/stderr.log"
    Start-Sleep -Seconds $Seconds
}
finally {
    $process.Refresh()
    $alive = -not $process.HasExited
    if ($alive) { Stop-Process -Id $process.Id -Force }
    [IO.File]::WriteAllBytes($config, $configBefore)
}
$diagnostics = @(Select-String -Path "$capture/stderr.log" -Pattern 'RUSTDIAG' | Measure-Object).Count
$hud = @(Select-String -Path "$capture/stderr.log" -Pattern 'FORMULAHUD n=' | Measure-Object).Count
$captures = @(Get-ChildItem $frames -Filter *.png -ErrorAction SilentlyContinue | Measure-Object).Count
"skip='$Skip' alive_after_${Seconds}s=$alive rustdiag=$diagnostics formulahud=$hud frames=$captures" |
    Tee-Object -FilePath "$capture/summary.txt"
Select-String -Path "$capture/stderr.log" -Pattern 'FORMULAHUD n=' | Select-Object -Last 2 | ForEach-Object { $_.Line.Trim() }
