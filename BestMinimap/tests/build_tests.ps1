$ErrorActionPreference='Stop'
$vc='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC'
$sdk='C:/Program Files/Microsoft SDKs/Windows/v7.1'
$env:INCLUDE="$vc/include;$sdk/Include"
$env:LIB="$vc/lib/amd64;$sdk/Lib/x64"
$env:PATH="$vc/bin/amd64;"+$env:PATH
$out='D:/DEV/Kenshi/build/BestMinimapTests'
New-Item -ItemType Directory -Force -Path $out | Out-Null
$source=[IO.File]::ReadAllText("$PSScriptRoot/../BestMinimap.cpp")
$begin=$source.IndexOf('    void DrawCameraRange(')
$end=$source.IndexOf('    // Последний нарисованный вид:', $begin)
if($begin -lt 0 -or $end -lt $begin){throw 'Production renderer boundary missing'}
[IO.File]::WriteAllText("$out/camera_stroke_under_test.h",$source.Substring($begin,$end-$begin))
$begin=$source.IndexOf('    bool ReadMainImage(')
$end=$source.IndexOf('    // Виджеты и текстура', $begin)
if($begin -lt 0 -or $end -lt $begin){throw 'Production reader boundary missing'}
[IO.File]::WriteAllText("$out/camera_reader_under_test.h",$source.Substring($begin,$end-$begin))
foreach($test in @('camera_policy_tests','camera_stroke_tests','camera_reader_tests')){
    & "$vc/bin/amd64/cl.exe" /nologo /EHsc /MD /O2 "/I$out" "/Fo$out/$test.obj" "/Fe$out/$test.exe" "$PSScriptRoot/$test.cpp"
    if($LASTEXITCODE){exit $LASTEXITCODE}
    & "$out/$test.exe"
    if($LASTEXITCODE){exit $LASTEXITCODE}
}
