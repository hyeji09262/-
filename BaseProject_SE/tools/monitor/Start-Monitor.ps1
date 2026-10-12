param([string]$PythonPath = '')

$ErrorActionPreference = 'Stop'
if ($PythonPath) {
    & $PythonPath (Join-Path $PSScriptRoot 'server.py')
    exit $LASTEXITCODE
}
if (Get-Command py -ErrorAction SilentlyContinue) {
    & py -3 (Join-Path $PSScriptRoot 'server.py')
} elseif (Get-Command python -ErrorAction SilentlyContinue) {
    & python (Join-Path $PSScriptRoot 'server.py')
} else {
    throw 'Python 3가 필요합니다. 설치 후 이 파일을 다시 실행하세요.'
}
