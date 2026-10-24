# Adds the missing `context` field to every hand-built `TriRaycastSample` literal.
#
# These literals were written against the retired chassis-anchor distance contract and
# keep that meaning explicitly (`..TriRaycastSample::default()` sets
# `WheelRayContext::from_chassis_anchor`). Production code (open_formula.rs) sets the
# context itself and is left untouched.
param(
    [Parameter(Mandatory = $true)][string[]]$Path
)

foreach ($file in $Path) {
    $text = [System.IO.File]::ReadAllText($file)
    $builder = New-Object System.Text.StringBuilder
    $index = 0
    $patched = 0
    $marker = 'TriRaycastSample {'
    while ($true) {
        $found = $text.IndexOf($marker, $index, [System.StringComparison]::Ordinal)
        if ($found -lt 0) {
            [void]$builder.Append($text.Substring($index))
            break
        }
        $open = $found + $marker.Length          # index just after '{'
        $depth = 1
        $cursor = $open
        while ($cursor -lt $text.Length -and $depth -gt 0) {
            $ch = $text[$cursor]
            if ($ch -eq '{') { $depth++ }
            elseif ($ch -eq '}') { $depth-- }
            $cursor++
        }
        $close = $cursor - 1                      # index of matching '}'
        $body = $text.Substring($open, $close - $open)
        [void]$builder.Append($text.Substring($index, $open - $index))
        if ($body -match '(^|[\s,])context\s*:') {
            [void]$builder.Append($body)
        } else {
            $trimmed = $body.TrimEnd()
            if ($trimmed.Length -gt 0 -and -not $trimmed.EndsWith(',')) {
                [void]$builder.Append($body)
                if ($body.EndsWith("`n")) { [void]$builder.Append('                ') }
                [void]$builder.Append(', ')
            } else {
                [void]$builder.Append($body)
            }
            [void]$builder.Append('..TriRaycastSample::default()')
            $patched++
        }
        [void]$builder.Append('}')
        $index = $close + 1
    }
    if ($patched -gt 0) {
        [System.IO.File]::WriteAllText($file, $builder.ToString())
    }
    Write-Output ("{0}: {1} literal(s) patched" -f $file, $patched)
}
