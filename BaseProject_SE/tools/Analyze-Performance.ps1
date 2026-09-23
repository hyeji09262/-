param(
    [Parameter(Mandatory = $true)]
    [string[]]$Path
)

# Read-only JSONL summary. Malformed/incomplete final lines are reported, not fatal.
$timings = @{}
$counts = @{}
$gauges = @{}
$windows = 0
$invalidLines = 0
$sessions = @()
$lastTimestamp = 0

foreach ($logPath in $Path) {
    Get-Content -LiteralPath $logPath -Encoding UTF8 | ForEach-Object {
        try {
            $record = $_ | ConvertFrom-Json -ErrorAction Stop
            if ($record.event -eq 'session_start') {
                $sessions += $record
            }
            if ($record.event -eq 'performance_window') {
                $windows++
                foreach ($field in $record.timings_ms.PSObject.Properties) {
                    if (-not $timings.ContainsKey($field.Name)) {
                        $timings[$field.Name] = @{ samples = 0; sum = 0.0; max = 0.0 }
                    }
                    $metric = $timings[$field.Name]
                    $metric.samples += $field.Value.samples
                    $metric.sum += $field.Value.sum
                    $metric.max = [Math]::Max($metric.max, $field.Value.max)
                }
                foreach ($field in $record.counters_sum.PSObject.Properties) {
                    $counts[$field.Name] += $field.Value
                }
                if ($record.unix_ms -ge $lastTimestamp) {
                    $lastTimestamp = $record.unix_ms
                    foreach ($field in $record.gauges_last.PSObject.Properties) {
                        $gauges[$field.Name] = $field.Value
                    }
                }
            }
        }
        catch {
            $invalidLines++
        }
    }
}

$frames = $counts['render.frames']
$drawsPerFrame = if ($frames -gt 0) { $counts['render.draw_calls.total'] / $frames } else { $null }
$fps = if ($timings.ContainsKey('frame.interval_ms') -and $timings['frame.interval_ms'].sum -gt 0) {
    1000 * $timings['frame.interval_ms'].samples / $timings['frame.interval_ms'].sum
} else { $null }
$summary = @($timings.GetEnumerator() | ForEach-Object {
    [pscustomobject]@{
        name = $_.Key
        samples = $_.Value.samples
        mean_ms = $_.Value.sum / $_.Value.samples
        max_ms = $_.Value.max
        sum_ms = $_.Value.sum
    }
} | Sort-Object -Property sum_ms -Descending)

[ordered]@{
    schema_version = 1
    event = 'performance_summary'
    source_files = $Path
    sessions = $sessions
    windows = $windows
    malformed_lines = $invalidLines
    frames = $frames
    fps_from_mean_interval = $fps
    draw_calls_per_frame = $drawsPerFrame
    timings_inclusive_do_not_add = $summary
    counters_sum = $counts
    gauges_latest = $gauges
} | ConvertTo-Json -Depth 12
