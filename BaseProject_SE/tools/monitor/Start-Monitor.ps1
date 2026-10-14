param([string]$PythonPath = '')

$ErrorActionPreference = 'Stop'
try {
    $candidates = @()
    if ($PythonPath) {
        $candidates += $PythonPath
    } else {
        foreach ($name in @('python', 'py')) {
            $command = Get-Command $name -ErrorAction SilentlyContinue
            if ($command -and $command.Source -notlike '*\WindowsApps\*') {
                $candidates += $command.Source
            }
        }
        $candidates += (Join-Path $env:USERPROFILE '.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe')
    }

    $selectedPython = $null
    foreach ($candidate in $candidates) {
        if (-not (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            continue
        }
        try {
            & $candidate -c 'import sys; sys.exit(0 if sys.version_info.major == 3 else 1)' 2>$null
            if ($LASTEXITCODE -eq 0) {
                $selectedPython = $candidate
                break
            }
        } catch {
            continue
        }
    }
    if (-not $selectedPython) {
        throw 'Python 3 not found. Install Python or use -PythonPath with its full path.'
    }

    Write-Host 'Monitor: http://127.0.0.1:8765'
    Write-Host 'Keep this window open while using the monitor.'
    & $selectedPython (Join-Path $PSScriptRoot 'server.py')
    if ($LASTEXITCODE -ne 0) {
        throw "Monitor server stopped with exit code $LASTEXITCODE. See the error above."
    }
} catch {
    Write-Host $_.Exception.Message -ForegroundColor Red
} finally {
    Read-Host 'Press Enter to close this window'
}
