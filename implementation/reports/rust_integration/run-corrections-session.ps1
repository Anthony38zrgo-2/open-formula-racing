# Single final integration session for the requested Rust presentation/setup corrections.
$ErrorActionPreference = 'Stop'
$root = 'D:/open-formula'
$capture = Join-Path $root 'implementation/reports/rust_integration/corrections-20261022'
New-Item -ItemType Directory -Force -Path $capture | Out-Null
$config = Join-Path $root 'distribution/user-settings/config/rust_physics.xml'
$override = Join-Path $root 'distribution/user-settings/config/rust_setup_0.tsv'
$configBefore = [IO.File]::ReadAllBytes($config)
$overrideBefore = if (Test-Path $override) { [IO.File]::ReadAllBytes($override) } else { $null }
if (Get-Process -Name speed-dreams-2 -ErrorAction SilentlyContinue) { throw 'An existing game session is running.' }
$process = $null
try {
    $text = [IO.File]::ReadAllText($config)
    $text = $text.Replace('name="integration scenario" val="0"', 'name="integration scenario" val="1"')
    $text = $text.Replace('</section>', '<attnum name="open garage at session start" val="0" /></section>')
    [IO.File]::WriteAllText($config, $text)
    $process = Start-Process -FilePath "$root/distribution/bin/speed-dreams-2.exe" -ArgumentList '-s','williams-fuji-practice','-lc',"$root/distribution/user-settings" -WorkingDirectory "$root/distribution" -WindowStyle Hidden -PassThru -RedirectStandardOutput "$capture/stdout.log" -RedirectStandardError "$capture/stderr.log"
    $deadline = (Get-Date).AddSeconds(55)
    while ((Get-Date) -lt $deadline -and -not $process.HasExited) {
        Start-Sleep -Milliseconds 500
        if (Select-String -Path "$capture/stderr.log" -Pattern 'RUSTSETUP reset=' -Quiet) { Start-Sleep -Seconds 2; break }
    }
}
finally {
    if ($process -and -not $process.HasExited) { Stop-Process -Id $process.Id -Force }
    [IO.File]::WriteAllBytes($config, $configBefore)
    if ($null -ne $overrideBefore) { [IO.File]::WriteAllBytes($override, $overrideBefore) }
    elseif (Test-Path $override) { Remove-Item -LiteralPath $override }
}
$nan = @(Select-String -Path "$capture/stdout.log" -Pattern 'detected NaN').Count
$errors = @(Select-String -Path "$capture/stderr.log" -Pattern 'rejected the step|ABI mismatch|layout mismatch').Count
"render_nan=$nan physics_or_abi_errors=$errors" | Tee-Object -FilePath "$capture/summary.txt"
Select-String -Path "$capture/stderr.log" -Pattern 'Rust physics:|RUSTGARAGE|RUSTSETUP|RUSTDIAG' | Select-Object -Last 9 | ForEach-Object { $_.Line }
