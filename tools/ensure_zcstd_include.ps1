# Ensure every mini-cocos source file that references `mstd::` explicitly
# includes "base/ZCStd.h". The engine PCH covers this for the MSVC build, but
# clangd / IDE tools that do not honour the MSVC PCH need the explicit include.
#
# Insertion point:
#   - .h : right after the first `#pragma once` line.
#   - .cpp: at the very top of the file (before any other #include).
#
# Skips platform/win32/ and base/ZCStd.h itself.

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$srcDir = Join-Path $repoRoot 'src'
$utf8 = New-Object System.Text.UTF8Encoding $false
$canonical = '#include "base/ZCStd.h"'

$files = Get-ChildItem -Recurse -Path $srcDir -Include *.cpp, *.h `
| Where-Object {
    $_.FullName -notmatch '\\platform\\win32\\' -and
    $_.FullName -notmatch '\\base\\ZCStd\.h$'
}

foreach ($f in $files) {
    $text = [System.IO.File]::ReadAllText($f.FullName, $utf8)

    if ($text -notmatch '\bmstd::') { continue }
    if ($text -match '(?m)^\s*#include\s*"base/ZCStd\.h"\s*$') { continue }

    if ($f.Extension -eq '.h') {
        $newText = [Regex]::Replace(
            $text,
            '(?m)^(\s*#pragma\s+once\s*)$',
            "`$1`r`n`r`n$canonical",
            [System.Text.RegularExpressions.RegexOptions]::None,
            [TimeSpan]::FromSeconds(5)
        )
        if ($newText -eq $text) {
            # No #pragma once; prepend.
            $newText = "$canonical`r`n`r`n$text"
        }
    }
    else {
        $newText = "$canonical`r`n$text"
    }

    [System.IO.File]::WriteAllText($f.FullName, $newText, $utf8)
    Write-Host "added include: $($f.FullName.Substring($repoRoot.Length + 1))"
}

Write-Host "done."
