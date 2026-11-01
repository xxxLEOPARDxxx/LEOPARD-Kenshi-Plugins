param([int]$Group=0,[int]$Groups=1)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$records=Get-Content "$PSScriptRoot/localization-builds.json" -Raw | ConvertFrom-Json
$out='D:/DEV/Kenshi/build/LocalizationMaintenance'
New-Item -ItemType Directory -Force $out | Out-Null
$results=@()
for($i=$Group;$i -lt $records.Count;$i+=$Groups) {
    $record=$records[$i]
    $args=@{SrcDir=$record.source;Name=$record.name}
    if($record.project){$args.FromVcxproj=$true}
    if($record.subdirs){$args.SubDirs=@($record.subdirs)}
    if($record.include){$args.Include=@($record.include)}
    if($record.define){$args.Define=@($record.define)}
    $started=[DateTime]::UtcNow
    & "$repo/build.ps1" @args *> "$out/$($record.name).log"
    $code=$LASTEXITCODE
    $dll="D:/DEV/Kenshi/build/$($record.name)/$($record.name).dll"
    if($code -eq 0 -and (!(Test-Path $dll) -or (Get-Item $dll).LastWriteTimeUtc -lt $started)){$code=99}
    $results+=@{name=$record.name;exit=$code;dll=$dll;source=$record.source}
    $results | ConvertTo-Json -Depth 5 | Set-Content "$out/group-$Group.json" -Encoding utf8
    Write-Output "$($record.name): $code"
    if($code -ne 0){Get-Content "$out/$($record.name).log" -Tail 14;exit $code}
}
