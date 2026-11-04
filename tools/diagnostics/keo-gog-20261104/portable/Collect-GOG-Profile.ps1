[CmdletBinding()]
param([string]$GameDir = '', [string]$OutputDirectory = '')
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2

# Offline only: no process, game-code execution, native patch, network or upload.
function HexBytes([byte[]]$Bytes) { return ([BitConverter]::ToString($Bytes)).Replace('-','').ToLowerInvariant() }
function HashBytes([byte[]]$Bytes, [string]$Kind) {
    $algorithm = [Security.Cryptography.HashAlgorithm]::Create($Kind)
    try { return HexBytes ($algorithm.ComputeHash($Bytes)) } finally { $algorithm.Dispose() }
}
function ReadBounded([string]$Path, [long]$Maximum) {
    $item = Get-Item -LiteralPath $Path
    if ($item.PSIsContainer -or $item.Length -le 0 -or $item.Length -gt $Maximum) { throw "Invalid bounded file: $Path" }
    $bytes = [IO.File]::ReadAllBytes($item.FullName)
    if ($bytes.Length -gt $Maximum) { throw 'File grew past size limit' }
    return ,$bytes
}
function ReadPE([byte[]]$Bytes) {
    if ($Bytes.Length -lt 64 -or $Bytes[0] -ne 77 -or $Bytes[1] -ne 90) { throw 'Missing MZ header' }
    $pe = [long][BitConverter]::ToUInt32($Bytes,60)
    if ($pe -lt 64 -or $pe + 24 -gt $Bytes.Length -or [BitConverter]::ToUInt32($Bytes,[int]$pe) -ne 17744) { throw 'Invalid PE header' }
    $machine = [BitConverter]::ToUInt16($Bytes,[int]$pe+4)
    $count = [BitConverter]::ToUInt16($Bytes,[int]$pe+6)
    $optionalSize = [BitConverter]::ToUInt16($Bytes,[int]$pe+20)
    $optional = $pe + 24
    if ($machine -ne 34404 -or $count -lt 1 -or $count -gt 96 -or $optionalSize -lt 112 -or $optional + $optionalSize -gt $Bytes.Length) { throw 'AMD64 PE32+ required' }
    if ([BitConverter]::ToUInt16($Bytes,[int]$optional) -ne 523) { throw 'PE32+ required' }
    $imageSize = [long][BitConverter]::ToUInt32($Bytes,[int]$optional+56)
    $headers = [long][BitConverter]::ToUInt32($Bytes,[int]$optional+60)
    $sectionStart = $optional + $optionalSize
    if ($headers -gt $Bytes.Length -or $imageSize -lt $headers -or $sectionStart + $count * 40 -gt $headers) { throw 'Invalid image/header extent' }
    $sections = @()
    for ($index=0; $index -lt $count; $index++) {
        $at = [int]($sectionStart + $index * 40)
        $virtualSize = [long][BitConverter]::ToUInt32($Bytes,$at+8)
        $rva = [long][BitConverter]::ToUInt32($Bytes,$at+12)
        $diskSize = [long][BitConverter]::ToUInt32($Bytes,$at+16)
        $diskOffset = [long][BitConverter]::ToUInt32($Bytes,$at+20)
        if ($rva + [Math]::Max($virtualSize,$diskSize) -gt $imageSize -or $diskOffset + $diskSize -gt $Bytes.Length -or ($diskSize -gt 0 -and $diskOffset -lt $headers)) { throw 'Invalid section extent' }
        foreach ($old in $sections) {
            if ([Math]::Max($rva,$old.rva) -lt [Math]::Min($rva+[Math]::Max($virtualSize,$diskSize),$old.rva+[Math]::Max($old.virtual_size,$old.disk_size))) { throw 'Overlapping virtual sections' }
        }
        $sections += [ordered]@{name=[Text.Encoding]::ASCII.GetString($Bytes,$at,8).TrimEnd([char]0);rva=$rva;virtual_size=$virtualSize;disk_size=$diskSize;disk_offset=$diskOffset}
    }
    return [ordered]@{machine=$machine;timestamp=[BitConverter]::ToUInt32($Bytes,[int]$pe+8);image_size=$imageSize;headers=$headers;image_base=[BitConverter]::ToUInt64($Bytes,[int]$optional+24);entrypoint=[BitConverter]::ToUInt32($Bytes,[int]$optional+16);sections=$sections}
}
function WindowHex([byte[]]$Bytes, $Profile, [long]$Rva) {
    if ($Rva -lt 0) { throw 'Invalid RVA' }
    foreach ($section in $Profile.sections) {
        $relative = $Rva - $section.rva
        if ($relative -ge 0 -and $relative + 32 -le $section.disk_size) {
            $window = New-Object byte[] 32
            [Array]::Copy($Bytes,$section.disk_offset+$relative,$window,0,32)
            return HexBytes $window
        }
    }
    throw ('Unbacked GOG code window: 0x{0:x}' -f $Rva)
}
function FindGame([string]$Requested) {
    if ($Requested) {
        $item = Get-Item -LiteralPath $Requested
        if ($item.PSIsContainer) { return $item.FullName }
        if ($item.Name -ne 'Kenshi_x64.exe') { throw 'Provide the game directory or Kenshi_x64.exe' }
        if ($item.Directory.Name -eq 'RE_Kenshi') { return $item.Directory.Parent.FullName }
        return $item.Directory.FullName
    }
    $near = [IO.DirectoryInfo]$PSScriptRoot
    for ($depth=0; $depth -lt 5 -and $null -ne $near; $depth++) {
        if ((Test-Path -LiteralPath (Join-Path $near.FullName 'Kenshi_x64.exe') -PathType Leaf) -or (Test-Path -LiteralPath (Join-Path $near.FullName 'RE_Kenshi/Kenshi_x64.exe') -PathType Leaf)) { return $near.FullName }
        $near = $near.Parent
    }
    $answer = Read-Host 'Path to your GOG Kenshi game directory'
    if (-not $answer) { throw 'No game directory provided' }
    return FindGame $answer.Trim('"')
}

$report = [ordered]@{schema=1;captured_utc=[DateTime]::UtcNow.ToString('o');scope='Offline compatibility diagnostics only; not a working GOG KEO build';native_profile_verified=$false;runtime_tested=$false;status='required_gog_inputs_unverified';executables=@();code_windows=@();errors=@()}
try {
    $root = FindGame $GameDir
    $selected = $null
    foreach ($relative in @('RE_Kenshi/Kenshi_x64.exe','Kenshi_x64.exe')) {
        $path = Join-Path $root $relative
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { continue }
        $bytes = ReadBounded $path 134217728
        $profile = ReadPE $bytes
        $md5 = HashBytes $bytes 'MD5'
        $identity = 'unknown'
        if ($md5 -eq '213349bf76a0a758067f9ed0aef2ab01') { $identity = 'gog_1.0.65' }
        elseif ($md5 -eq 'df4a5a7ef8a29deb24b70e7b7f4a222a') { $identity = 'steam_1.0.65' }
        $report.executables += [ordered]@{relative_path=$relative;bytes=$bytes.Length;sha256=(HashBytes $bytes 'SHA256');md5=$md5;identity=$identity;pe=$profile}
        if ($identity -eq 'gog_1.0.65' -and $null -eq $selected) { $selected = @{bytes=$bytes;profile=$profile} }
    }
    if ($report.executables.Count -eq 0) { throw 'Kenshi_x64.exe not found in the requested directory' }
    $tablePath = Join-Path $root 'RE_Kenshi/RVAs/GOG_1.0.65.br'
    if (Test-Path -LiteralPath $tablePath -PathType Leaf) {
        $table = ReadBounded $tablePath 400000
        if ($table.Length % 4 -ne 0) { throw 'Invalid raw uint32 RVA table length' }
        $digest = HashBytes $table 'SHA256'
        $expectedDll = ''
        if ($digest -eq 'a2a6394e95312a89e3d3f6842fbfda9c16e7d79749f1ee796fbd6168600bf347' -and $table.Length -eq 30520) { $expectedDll = '5b6240622d06801dc39896e385efb834b8e1fe2fcdf86067d071ae7a04f6c83b' }
        elseif ($digest -eq '8910248abd88d78b777c8f77d5e8ef5287b87757001000cade44e7082e58cbc0' -and $table.Length -eq 30712) { $expectedDll = '9c7f3c6f63f8faff3573095199494f8e985fda91f54b9c3cad7079d0a56b1886' }
        $report.rvas = [ordered]@{relative_path='RE_Kenshi/RVAs/GOG_1.0.65.br';bytes=$table.Length;entries=($table.Length/4);sha256=$digest;known_gog_table=[bool]$expectedDll;expected_kenshilib_sha256=$expectedDll}
        $library = Join-Path $root 'KenshiLib.dll'
        if (Test-Path -LiteralPath $library -PathType Leaf) {
            $dllBytes = ReadBounded $library 134217728
            $dllHash = HashBytes $dllBytes 'SHA256'
            $report.kenshilib = [ordered]@{bytes=$dllBytes.Length;sha256=$dllHash;matches_known_table_pair=([bool]$expectedDll -and $dllHash -eq $expectedDll)}
        }
        if ($null -ne $selected -and $expectedDll) {
            $report.status = 'gog_image_and_known_table_identified'
            $ledgerPath = Join-Path $PSScriptRoot 'coverage.json'
            $ledgerRaw = ReadBounded $ledgerPath 4194304
            $ledger = [Text.Encoding]::UTF8.GetString($ledgerRaw) | ConvertFrom-Json
            $rows = @($ledger.manifest)
            if ($rows.Count -gt 512) { throw 'Coverage manifest bound' }
            $report.coverage_source_head = $ledger.source_head
            foreach ($row in $rows) {
                $addresses = @($row.official_gog_rvas)
                if ($addresses.Count -gt 64) { throw 'Coverage candidate bound' }
                if ($addresses.Count -eq 1) {
                    $rva = [Convert]::ToInt64($addresses[0].Substring(2),16)
                    $report.code_windows += [ordered]@{name=$row.name;kind=$row.kind;rva=$addresses[0];first32=(WindowHex $selected.bytes $selected.profile $rva)}
                }
            }
        }
    } else { $report.errors += 'GOG_1.0.65.br missing; no package was downloaded or modified' }
} catch { $report.errors += $_.Exception.Message; $report.status = 'diagnostic_input_refused' }

if (-not $OutputDirectory) { $OutputDirectory = Join-Path $PSScriptRoot 'reports' }
$null = New-Item -ItemType Directory -Path $OutputDirectory -Force
$name = 'gog-profile-' + [DateTime]::UtcNow.ToString('yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8) + '.json'
$output = Join-Path $OutputDirectory $name
$json = $report | ConvertTo-Json -Depth 16
$stream = New-Object IO.FileStream($output,[IO.FileMode]::CreateNew,[IO.FileAccess]::Write,[IO.FileShare]::None)
try { $utf8 = New-Object Text.UTF8Encoding($false); $writer = New-Object IO.StreamWriter($stream,$utf8); try { $writer.WriteLine($json); $writer.Flush() } finally { $writer.Dispose() } } finally { $stream.Dispose() }
Write-Host ('Status: ' + $report.status)
Write-Host ('Report: ' + $output)
Write-Host 'No files were uploaded. Send only this JSON report to the mod maintainer.'
Write-Host 'This is diagnostics, not a working GOG KEO build.'
if ($report.status -eq 'gog_image_and_known_table_identified') { exit 0 }
exit 2
