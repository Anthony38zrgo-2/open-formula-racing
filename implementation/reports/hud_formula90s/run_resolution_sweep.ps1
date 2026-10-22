# Resolution sweep for the Formula90s HUD.
#
# Runs the instrumented session once per resolution, captures frames and records the
# layout rectangles the HUD logs for the visible area. screen.xml is restored afterwards.
param(
    [int]$Seconds = 22
)
# Every resolution to exercise, in order.
$Resolutions = @('1280x720', '1920x1080', '2560x1440', '3440x1440')
$ErrorActionPreference = 'Stop'
$root = 'D:/open-formula'
$screen = Join-Path $root 'distribution/user-settings/config/screen.xml'
$physics = Join-Path $root 'distribution/user-settings/config/rust_physics.xml'
$screenBefore = [IO.File]::ReadAllBytes($screen)
$physicsBefore = [IO.File]::ReadAllBytes($physics)
if (Get-Process -Name speed-dreams-2 -ErrorAction SilentlyContinue) { throw 'An existing game session is running.' }

$physicsText = [IO.File]::ReadAllText($physics)
$physicsText = $physicsText.Replace('name="integration scenario" val="0"', 'name="integration scenario" val="1"')
$physicsText = $physicsText.Replace('</section>', '<attnum name="open garage at session start" val="0" /></section>')
[IO.File]::WriteAllText($physics, $physicsText)

$env:OPEN_FORMULA_HUD_DIAG = '1'
$results = @()
try {
    foreach ($resolution in $Resolutions) {
        $parts = $resolution -split 'x'
        $width = [int]$parts[0]
        $height = [int]$parts[1]
        $text = [IO.File]::ReadAllText($screen)
        $text = $text -replace '(<attnum name="window width" val=")\d+(")', "`${1}$width`${2}"
        $text = $text -replace '(<attnum name="window height" val=")\d+(")', "`${1}$height`${2}"
        [IO.File]::WriteAllText($screen, $text)
        $capture = Join-Path $root "implementation/reports/hud_formula90s/resolution-$resolution"
        New-Item -ItemType Directory -Force -Path $capture | Out-Null
        $frames = Join-Path $capture 'frames'
        New-Item -ItemType Directory -Force -Path $frames | Out-Null
        $env:OPEN_FORMULA_CAPTURE_DIR = $frames
        $env:OPEN_FORMULA_CAPTURE_FRAMES = '300,600,900'
        $process = Start-Process -FilePath "$root/distribution/bin/speed-dreams-2.exe" `
            -ArgumentList '-s', 'williams-fuji-practice', '-lc', "$root/distribution/user-settings" `
            -WorkingDirectory "$root/distribution" -PassThru `
            -RedirectStandardOutput "$capture/stdout.log" -RedirectStandardError "$capture/stderr.log"
        Start-Sleep -Seconds $Seconds
        $process.Refresh()
        $alive = -not $process.HasExited
        if ($alive) { Stop-Process -Id $process.Id -Force }
        Start-Sleep -Milliseconds 500
        $rects = @(Select-String -Path "$capture/stderr.log" -Pattern 'Formula HUD group ' |
            ForEach-Object { ($_.Line -split 'Formula HUD group ')[1].Trim() })
        $shots = @(Get-ChildItem $frames -Filter *.png -ErrorAction SilentlyContinue).Count
        $results += "resolution=$resolution alive=$alive frames=$shots"
        $results += $rects
    }
}
finally {
    [IO.File]::WriteAllBytes($screen, $screenBefore)
    [IO.File]::WriteAllBytes($physics, $physicsBefore)
}
$results | Tee-Object -FilePath (Join-Path $root 'implementation/reports/hud_formula90s/resolution-sweep.txt')
