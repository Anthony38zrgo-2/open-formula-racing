# Open Formula Rust physics integration run (single instrumented session).
#
# Starts the distribution through its normal entry point, waits for the in-pit garage,
# leaves the pits with a real key event, holds the accelerator / brake / steering for
# the scripted manoeuvre, and captures the instrumented runtime log.
#
# Usage:
#   pwsh -File tools/run_rust_integration.ps1 -ThrottleSeconds 6 -BrakeSeconds 3

param(
    [string]$RaceMan = 'williams-fuji-practice',
    [string]$LocalDir = 'D:/open-formula/distribution/user-settings',
    [int]$ThrottleSeconds = 6,
    [int]$BrakeSeconds = 3,
    [int]$SteerSeconds = 3,
    [int]$SettleSeconds = 3,
    [int]$StartupTimeoutSeconds = 45,
    [string]$CaptureDirectory = 'D:/open-formula/implementation/reports/rust_integration'
)

$ErrorActionPreference = 'Stop'

Add-Type -TypeDefinition @"
using System;
using System.Runtime.InteropServices;
public static class OfpKeyboard {
    [DllImport("user32.dll")] public static extern void keybd_event(byte bVk, byte bScan, uint dwFlags, UIntPtr dwExtraInfo);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hWnd);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hWnd, int nCmdShow);
    public static void Down(byte key) { keybd_event(key, 0, 0x01, UIntPtr.Zero); }
    public static void Up(byte key) { keybd_event(key, 0, 0x03, UIntPtr.Zero); }
}
"@

$distribution = 'D:/open-formula/distribution'
$executable = Join-Path $distribution 'bin/speed-dreams-2.exe'
$log = Join-Path $distribution 'user-settings/runtime.log'
$stdout = Join-Path $env:TEMP 'ofp-rust-integration.out'
$stderr = Join-Path $env:TEMP 'ofp-rust-integration.err'
New-Item -ItemType Directory -Force -Path $CaptureDirectory | Out-Null

if (Get-Process -Name speed-dreams-2 -ErrorAction SilentlyContinue) {
    throw 'A Speed Dreams session is already running; close it before the integration run.'
}
if (Test-Path $log) { Remove-Item $log -Force }

$process = Start-Process -FilePath $executable `
    -ArgumentList '-s', $RaceMan, '-lc', $LocalDir `
    -WorkingDirectory $distribution -PassThru `
    -RedirectStandardOutput $stdout -RedirectStandardError $stderr

function Wait-LogPattern([string]$pattern, [int]$timeoutSeconds) {
    $deadline = (Get-Date).AddSeconds($timeoutSeconds)
    while ((Get-Date) -lt $deadline) {
        foreach ($candidate in @($stderr, $log)) {
            if (Test-Path $candidate) {
                $match = Select-String -Path $candidate -Pattern $pattern -ErrorAction SilentlyContinue | Select-Object -Last 1
                if ($match) { return $match.Line }
            }
        }
        Start-Sleep -Milliseconds 250
    }
    return $null
}

function Focus-Game {
    $shell = New-Object -ComObject WScript.Shell
    [void]$shell.AppActivate($process.Id)
    Start-Sleep -Milliseconds 600
    return $shell
}

try {
    $pit = Wait-LogPattern 'Formula garage state: PIT' $StartupTimeoutSeconds
    if (-not $pit) { throw 'The pit garage did not open in time.' }

    $driving = $null
    for ($attempt = 0; $attempt -lt 4 -and -not $driving; $attempt++) {
        Start-Sleep -Seconds 1
        $shell = Focus-Game
        $shell.SendKeys('{ENTER}')
        $driving = Wait-LogPattern 'Formula garage state: DRIVING' 4
    }
    if (-not $driving) { throw 'Leaving the pits did not reach the driving state.' }

    Start-Sleep -Seconds $SettleSeconds
    [void](Focus-Game)
    [OfpKeyboard]::Down(0x26)   # Arrow Up: throttle
    Start-Sleep -Seconds $ThrottleSeconds
    [OfpKeyboard]::Up(0x26)

    if ($SteerSeconds -gt 0) {
        [OfpKeyboard]::Down(0x25)   # Arrow Left
        Start-Sleep -Seconds $SteerSeconds
        [OfpKeyboard]::Up(0x25)
    }

    if ($BrakeSeconds -gt 0) {
        [OfpKeyboard]::Down(0x28)   # Arrow Down: brake
        Start-Sleep -Seconds $BrakeSeconds
        [OfpKeyboard]::Up(0x28)
    }

    Start-Sleep -Seconds 2
}
finally {
    if (-not $process.HasExited) {
        Start-Sleep -Milliseconds 500
        Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        Start-Sleep -Seconds 1
    }
    if (Test-Path $log) { Copy-Item $log (Join-Path $CaptureDirectory 'runtime.log') -Force }
    if (Test-Path $stderr) { Copy-Item $stderr (Join-Path $CaptureDirectory 'runtime-stderr.log') -Force }
    if (Test-Path $stdout) { Copy-Item $stdout (Join-Path $CaptureDirectory 'runtime-stdout.log') -Force }
}

$diagnostics = @()
if (Test-Path $stderr) {
    $diagnostics = Select-String -Path $stderr -Pattern 'RUSTDIAG|RUSTSETUP|Rust physics:' |
        ForEach-Object { $_.Line.Trim() }
}
$diagnostics | Set-Content (Join-Path $CaptureDirectory 'instrumentation.txt') -Encoding utf8
$diagnostics | Select-Object -Last 20
