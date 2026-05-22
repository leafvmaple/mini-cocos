# Refactor mini-cocos sources: route data-structure/algorithm STL through
# base/ZCStd.h + the `mstd` namespace alias. Excludes platform/win32/ (still
# uses <filesystem>/<system_error>/std::wstring directly) and the switcher
# header base/ZCStd.h itself.
#
# Reads/writes files as UTF-8 (no BOM) — critical on Chinese-locale Windows
# where the PowerShell 5.1 default ANSI is cp936.

$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
$srcDir = Join-Path $repoRoot 'src'

$stlIncludes = @(
    'vector', 'string', 'unordered_map', 'set', 'array',
    'algorithm', 'utility', 'functional', 'memory', 'new', 'limits'
)

# Order matters: longer / more-specific patterns first.
$nsReplacements = @(
    @('std::remove_if', 'mstd::remove_if'),
    @('std::stable_sort', 'mstd::stable_sort'),
    @('std::numeric_limits', 'mstd::numeric_limits'),
    @('std::make_unique', 'mstd::make_unique'),
    @('std::unique_ptr', 'mstd::unique_ptr'),
    @('std::unordered_map', 'mstd::unordered_map'),
    @('std::equal_to', 'mstd::equal_to'),
    @('std::to_string', 'mstd::to_string'),
    @('std::find_if', 'mstd::find_if'),
    @('std::function', 'mstd::function'),
    @('std::nothrow', 'mstd::nothrow'),
    @('std::forward', 'mstd::forward'),
    @('std::clamp', 'mstd::clamp'),
    @('std::vector', 'mstd::vector'),
    @('std::string', 'mstd::string'),
    @('std::array', 'mstd::array'),
    @('std::size_t', 'mstd::size_t'),
    @('std::sort', 'mstd::sort'),
    @('std::find', 'mstd::find'),
    @('std::remove', 'mstd::remove'),
    @('std::hash', 'mstd::hash'),
    @('std::less', 'mstd::less'),
    @('std::pair', 'mstd::pair'),
    @('std::move', 'mstd::move'),
    @('std::swap', 'mstd::swap'),
    @('std::max', 'mstd::max'),
    @('std::min', 'mstd::min'),
    @('std::set', 'mstd::set')
)

$utf8 = New-Object System.Text.UTF8Encoding $false  # no BOM

$files = Get-ChildItem -Recurse -Path $srcDir -Include *.cpp, *.h `
| Where-Object {
    $_.FullName -notmatch '\\platform\\win32\\' -and
    $_.FullName -notmatch '\\base\\ZCStd\.h$'
}

$sentinel = '___ZCSTD_INCLUDE_SENTINEL___'
$canonical = '#include "base/ZCStd.h"'

foreach ($f in $files) {
    $orig = [System.IO.File]::ReadAllText($f.FullName, $utf8)
    $text = $orig

    foreach ($t in $stlIncludes) {
        $pat = '(?m)^[ \t]*#include[ \t]*<' + [Regex]::Escape($t) + '>[ \t]*\r?$'
        $text = [Regex]::Replace($text, $pat, $sentinel)
    }

    # Collapse consecutive sentinels into one.
    while ($text -match ($sentinel + '\r?\n' + $sentinel)) {
        $text = $text -replace ($sentinel + '\r?\n' + $sentinel), $sentinel
    }

    $text = $text.Replace($sentinel, $canonical)

    # De-dup canonical include lines across the file (keep first occurrence).
    $lines = $text -split "`r?`n"
    $seen = $false
    $kept = New-Object System.Collections.Generic.List[string]
    foreach ($ln in $lines) {
        if ($ln.Trim() -eq $canonical) {
            if ($seen) { continue }
            $seen = $true
            $kept.Add($ln) | Out-Null
        }
        else {
            $kept.Add($ln) | Out-Null
        }
    }
    $text = ($kept -join "`r`n")

    foreach ($pair in $nsReplacements) {
        $from = $pair[0]; $to = $pair[1]
        $rx = '\b' + [Regex]::Escape($from) + '\b'
        $text = [Regex]::Replace($text, $rx, $to)
    }

    if ($text -ne $orig) {
        [System.IO.File]::WriteAllText($f.FullName, $text, $utf8)
        Write-Host "patched: $($f.FullName.Substring($repoRoot.Length + 1))"
    }
}

Write-Host "done."
