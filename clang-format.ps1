# Optional parameter: subdirectory to format (relative or absolute). If omitted, whole tree.
param(
    [Parameter(Position=0)]
    [string]$SubDir,
    [Parameter()] [switch]$Parallel,            # Enable parallel formatting using jobs
    [Parameter()] [int]$MaxConcurrency = [Environment]::ProcessorCount,  # Max parallel jobs
    [Parameter()] [ValidateSet('Group','PerFile')] [string]$ParallelStrategy = 'Group' # Strategy for parallel execution
)

# Check clang-format availability (high priority fix)
$ClangFormatCmd = Get-Command clang-format -ErrorAction SilentlyContinue
if (-not $ClangFormatCmd) {
    Write-Error "clang-format not found in PATH. Please install LLVM clang-format."
    exit 3
}

# Resolve target root path
if ([string]::IsNullOrWhiteSpace($SubDir)) {
    $TargetRoot = Get-Location
    Write-Host "[Info] No subdirectory specified. Formatting entire directory: $TargetRoot" -ForegroundColor Cyan
} else {
    if (-not (Test-Path -LiteralPath $SubDir)) {
        Write-Error "Specified subdirectory does not exist: $SubDir"
        exit 1
    }
    $Resolved = Resolve-Path -LiteralPath $SubDir | Select-Object -First 1 -ExpandProperty Path
    if (-not (Test-Path -LiteralPath $Resolved -PathType Container)) {
        Write-Error "Specified path is not a directory: $Resolved"
        exit 1
    }
    $TargetRoot = $Resolved
    Write-Host "[Info] Formatting only subdirectory: $TargetRoot" -ForegroundColor Cyan
}

# Gather files
Write-Host "[Info] Collecting source files (*.cpp, *.h, *.hpp) ..." -ForegroundColor Cyan
$FilesToFormat = Get-ChildItem -Path $TargetRoot -Recurse -File |
    Where-Object { $_.Extension -in '.cpp','.h','.hpp' }

if (-not $FilesToFormat -or $FilesToFormat.Count -eq 0) {
    Write-Warning "No matching source files found under: $TargetRoot"
    exit 0
}

# Calculate total and init counter
$TotalCount = $FilesToFormat.Count
$Counter = 0

# Iterate & format (sequential or parallel)
$Stopwatch = [System.Diagnostics.Stopwatch]::StartNew()
$Errors = @()

if ($Parallel -and $MaxConcurrency -gt 1) {
    Write-Host "[Info] Parallel mode enabled (MaxConcurrency=$MaxConcurrency, Strategy=$ParallelStrategy)" -ForegroundColor Cyan
    $FilePaths = $FilesToFormat | ForEach-Object { $_.FullName }
    if ($MaxConcurrency -gt $FilePaths.Count) { $MaxConcurrency = $FilePaths.Count }
    if ($MaxConcurrency -lt 1) { $MaxConcurrency = 1 }

    if ($ParallelStrategy -eq 'Group') {
        # Group strategy: fewer jobs, less overhead, coarse progress (fixed double counting)
        $GroupSize = [Math]::Ceiling($FilePaths.Count / $MaxConcurrency)
        $Jobs = @()
        $ScriptBlock = {
            param($Paths)
            foreach ($p in $Paths) {
                try {
                    clang-format -i $p 2>$null
                    if ($LASTEXITCODE -ne 0) { "ERR:$p" } else { "OK:$p" }
                } catch { "ERR:$p" }
            }
        }
        for ($i = 0; $i -lt $FilePaths.Count; $i += $GroupSize) {
            $end = [Math]::Min($i + $GroupSize - 1, $FilePaths.Count - 1)
            $slice = $FilePaths[$i..$end]
            $Jobs += Start-Job -ScriptBlock $ScriptBlock -ArgumentList @($slice)
        }

        # Track completed outputs without re-counting
        $ProcessedSet = [System.Collections.Generic.HashSet[string]]::new()
        $ErrorSet     = [System.Collections.Generic.HashSet[string]]::new()
        $RemainingJobs = @($Jobs)

        while ($RemainingJobs.Count -gt 0) {
            foreach ($job in @($RemainingJobs)) {
                if ($job.State -eq 'Completed') {
                    $out = Receive-Job -Job $job 2>$null
                    foreach ($line in $out) {
                        if ($line -match '^(OK|ERR):(.*)$') {
                            $path = $Matches[2]
                            if ($ProcessedSet.Add($path)) {
                                if ($line.StartsWith('ERR:')) { $ErrorSet.Add($path) | Out-Null }
                            }
                        }
                    }
                    $RemainingJobs = $RemainingJobs | Where-Object { $_.Id -ne $job.Id }
                    Remove-Job -Job $job -Force 2>$null
                }
            }
            $ProcessedTotal = $ProcessedSet.Count
            $PercentComplete = ($ProcessedTotal / $TotalCount) * 100
            Write-Progress -Activity "Clang-Format Formatting Source Files" `
                           -Status "Processed $ProcessedTotal / $TotalCount (Errors: $($ErrorSet.Count))" `
                           -PercentComplete $PercentComplete
            Start-Sleep -Milliseconds 200
        }

        $Errors = @($ErrorSet)
        $Counter = $ProcessedSet.Count
        Write-Progress -Activity "Clang-Format Formatting Source Files" -Completed $true
    } else {
        # PerFile strategy: one job per file, smoother progress, more overhead
        $ScriptBlock = {
            param($Path)
            try {
                clang-format -i $Path 2>$null
                if ($LASTEXITCODE -ne 0) { "ERR:$Path" } else { "OK:$Path" }
            } catch { "ERR:$Path" }
        }
        $Pending = [System.Collections.Queue]::new()
        foreach ($p in $FilePaths) { $Pending.Enqueue($p) }
        $ActiveJobs = @()
        $Results = @()
        while ($Pending.Count -gt 0 -or $ActiveJobs.Count -gt 0) {
            while ($ActiveJobs.Count -lt $MaxConcurrency -and $Pending.Count -gt 0) {
                $next = $Pending.Dequeue()
                $ActiveJobs += Start-Job -ScriptBlock $ScriptBlock -ArgumentList $next
            }
            foreach ($j in @($ActiveJobs)) {
                if ($j.State -ne 'Running') {
                    $out = Receive-Job -Job $j 2>$null
                    if ($out) { $Results += $out }
                    Remove-Job -Job $j -Force 2>$null
                    $ActiveJobs = $ActiveJobs | Where-Object { $_.Id -ne $j.Id }
                }
            }
            $ProcessedOk = ($Results | Where-Object { $_ -like 'OK:*' }).Count
            $ProcessedErr = ($Results | Where-Object { $_ -like 'ERR:*' }).Count
            $ProcessedTotal = $ProcessedOk + $ProcessedErr
            $PercentComplete = ($ProcessedTotal / $TotalCount) * 100
            Write-Progress -Activity "Clang-Format Formatting Source Files" `
                           -Status "Processed $ProcessedTotal / $TotalCount (Errors: $ProcessedErr)" `
                           -PercentComplete $PercentComplete
            Start-Sleep -Milliseconds 100
        }
        $Errors = $Results | Where-Object { $_ -like 'ERR:*' } | ForEach-Object { $_.Substring(4) }
        $Counter = $Results.Count
        Write-Progress -Activity "Clang-Format Formatting Source Files" -Completed $true
    }
} else {
    if ($Parallel) { Write-Host "[Warn] Parallel requested but MaxConcurrency < 2; falling back to sequential." -ForegroundColor Yellow }
    $FilesToFormat | ForEach-Object {
        $Counter++
        $PercentComplete = ($Counter / $TotalCount) * 100
        Write-Progress -Activity "Clang-Format Formatting Source Files" `
                       -Status "Formatting file ($Counter / $TotalCount): $($_.FullName)" `
                       -PercentComplete $PercentComplete
        try {
            clang-format -i $_.FullName 2>$null
            if ($LASTEXITCODE -ne 0) { $Errors += $_.FullName }
        } catch { $Errors += $_.FullName }
    }
    Write-Progress -Activity "Clang-Format Formatting Source Files" -Completed $true
}

$Stopwatch.Stop()
Write-Host "[Info] Completed formatting $TotalCount file(s) in $([Math]::Round($Stopwatch.Elapsed.TotalSeconds,2))s" -ForegroundColor Green
if ($Errors.Count -gt 0) {
    Write-Warning "clang-format reported issues on $($Errors.Count) file(s):"
    $Errors | ForEach-Object { Write-Host "  $_" -ForegroundColor Yellow }
    exit 2
}
exit 0