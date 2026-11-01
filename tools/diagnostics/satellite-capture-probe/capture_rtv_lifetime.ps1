param(
    [Parameter(Mandatory=$true)][int]$ProbeProcessId,
    [Parameter(Mandatory=$true)][string]$ExpectedStart,
    [Parameter(Mandatory=$true)][ValidateRange(1,2147483647)][int]$Token,
    [Parameter(Mandatory=$true)][string]$ProofName,
    [ValidateRange(15,300)][int]$TimeoutSeconds=75
)
$ErrorActionPreference='Stop'
$gameRoot='D:/SteamLibrary/steamapps/common/Kenshi'
$probeRoot=Join-Path $gameRoot 'mods/Codex Satellite Capture Probe'
$ini=Join-Path $probeRoot 'SatelliteCaptureProbe.ini'
$log=Join-Path $gameRoot 'RE_Kenshi_log.txt'
$proof=Join-Path (Join-Path $gameRoot '_codex_work/global-satellite-20261101/rtv-lifetime-20261101') $ProofName
New-Item -ItemType Directory -Path $proof -Force | Out-Null
function Read-Log {
    $stream=[System.IO.FileStream]::new($log,[System.IO.FileMode]::Open,[System.IO.FileAccess]::Read,[System.IO.FileShare]::ReadWrite -bor [System.IO.FileShare]::Delete)
    try {$reader=[System.IO.StreamReader]::new($stream);try{return $reader.ReadToEnd()}finally{$reader.Dispose()}}finally{$stream.Dispose()}
}
function Sample {
    $process=Get-Process -Id $ProbeProcessId
    if($process.StartTime.ToUniversalTime().ToString('o') -ne $ExpectedStart){throw 'Unexpected process identity'}
    [ordered]@{utc=[DateTime]::UtcNow.ToString('o');privateBytes=$process.PrivateMemorySize64;workingSet=$process.WorkingSet64;handles=$process.HandleCount;threads=$process.Threads.Count}
}
$baseline=Sample
$config=Get-Content -LiteralPath $ini -Raw
if($config -notmatch '(?m)^EnableRequestFile=1\s*$' -or $config -notmatch '(?m)^IsolatedTerrain=1\s*$'){throw 'Not the explicit isolated request configuration'}
foreach($key in @('PrimaryGridBatch','RecycleTerrainBetweenCells')){
    if($config -notmatch "(?m)^$key=0\s*$"){throw 'This proof is one independently owned surface set'}
}
$config=[regex]::Replace($config,'(?m)^PrimaryX=\d+','PrimaryX=1')
$config=[regex]::Replace($config,'(?m)^PrimaryZ=\d+','PrimaryZ=1')
$config=[regex]::Replace($config,'(?m)^Request=\d+',"Request=$Token")
$tokenPattern="(?m)explicit request token=$Token[ \t]*\r?$"
if([regex]::IsMatch((Read-Log),$tokenPattern)){throw 'Request token already used'}
$began=[DateTime]::UtcNow
$samples=@();$section='';$success=$false
try {
    Set-Content -LiteralPath $ini -Value $config -Encoding ASCII
    do {
        Start-Sleep -Milliseconds 200
        $samples+=,(Sample)
        [string]$text=Read-Log
        $found=[regex]::Matches($text,$tokenPattern)
        if($found.Count) {
            $index=$found[$found.Count-1].Index
            $lineStart=$text.LastIndexOf("`n",$index)+1
            $section=$text.Substring($lineStart)
        }
        if($section -match 'quarantined|retaining resources|cleanup unverified|budget exceeded|did not stabilize|capture refused|context changed|RTV.*refused'){throw $section}
        $success=$section.Contains('owned RTV lifetime proof complete; creation debts=6')
    } while(!$success -and ([DateTime]::UtcNow-$began).TotalSeconds -lt $TimeoutSeconds)
    if(!$success){throw 'Timeout before owned RTV cleanup proof'}
    if(([regex]::Matches($section,'owned RTV creation debt released slot=')).Count -ne 6){throw 'Not exactly six creation-reference releases'}
    if(([regex]::Matches($section,'kernel-joined; omitted HANDLE close repaired')).Count -ne 1){throw 'Missing unique worker close'}
    $prefix=[regex]::Match($section,('(isolated_terrain_'+$ProbeProcessId+'_\d+_\d+)_gbuffer_raw')).Groups[1].Value
    if(!$prefix){throw 'No image prefix'}
    $files=@(Get-ChildItem -LiteralPath $probeRoot -Filter ($prefix+'*.bmp'))
    if($files.Count -ne 3){throw 'Expected GBuffer, raw Sun and exposed Sun images'}
    $files | Copy-Item -Destination $proof -Force
    $row=[ordered]@{processId=$ProbeProcessId;processStart=$ExpectedStart;token=$Token;elapsedMs=[int]([DateTime]::UtcNow-$began).TotalMilliseconds;baseline=$baseline;after=Sample;prefix=$prefix;files=@($files|ForEach-Object{@{name=$_.Name;bytes=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLower()}});log=$section;scope='distant Terrain raster, successful native owned RTV cleanup; not complete map'}
    $row|ConvertTo-Json -Depth 7|Set-Content -LiteralPath (Join-Path $proof 'capture.json') -Encoding utf8
    Write-Output ([pscustomobject]@{proof=$ProofName;ms=$row.elapsedMs;debts=6;privateBefore=$baseline.privateBytes;privateAfter=$row.after.privateBytes;prefix=$prefix})
} finally {
    # A timed-out foreground request must not fire much later when the user
    # returns to the game. Do this before any evidence IO, including disk-full
    # failure paths. Cancel only our token, preserving newer requests.
    try {
        $currentConfig=Get-Content -LiteralPath $ini -Raw
        if($currentConfig -match "(?m)^Request=$Token[ \t]*\r?$") {
            $currentConfig=[regex]::Replace($currentConfig,'(?m)^Request=\d+','Request=0')
            Set-Content -LiteralPath $ini -Value $currentConfig -Encoding ASCII
        }
    } finally {
        $section|Set-Content -LiteralPath (Join-Path $proof 'request.log') -Encoding utf8
        $samples|ConvertTo-Json -Depth 4|Set-Content -LiteralPath (Join-Path $proof 'memory-samples.json') -Encoding utf8
        $config|Set-Content -LiteralPath (Join-Path $proof 'request.ini') -Encoding ASCII
    }
}
