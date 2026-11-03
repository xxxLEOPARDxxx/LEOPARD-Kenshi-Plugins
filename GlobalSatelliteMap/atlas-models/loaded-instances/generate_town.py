"""Compile isolated town HDR composition against installed native lighting."""
from pathlib import Path
import hashlib,json,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest()
pins={
 'data/materials/common/constants.hlsl':'724c4b4f370ce4fa6a14d16e3cf7b2a82c076f8ff623110fd1d8ab25ab920530',
 'data/materials/common/lightingFunctions.hlsl':'51dd7cec6f898b3a79352067dbfbe586c1d364967154609ed821667d22966f9f',
}
for p,s in pins.items():assert sha(GAME/p)==s,p
target=OUT/'TownCompose.cso'
subprocess.run([str(FXC),'/nologo','/T','cs_5_0','/E','main','/O3','/I',str(GAME/'data/materials/common'),'/Fo',str(target),str(HERE/'TownCompose.hlsl')],check=True)
data=target.read_bytes()
(OUT/'TownComposeShader.h').write_text('#pragma once\nstatic const unsigned char TownComposeShader[]={\n'+','.join(str(b) for b in data)+'\n};\n')
dump=subprocess.check_output([str(FXC),'/dumpbin',str(target)],text=True)
assert 'CB0[14]' in dump and 'Offset:  208 Size:    16' in dump and 't9' in dump and 'u3' in dump
(OUT/'town-shader.json').write_text(json.dumps(dict(sourceSha256=sha(HERE/'TownCompose.hlsl'),shaderSha256=sha(target),nativeIncludes=pins,nativeFormulaReferenceSha256=sha(HERE.parents[1]/'shaders/SunProbe.hlsl')),indent=2)+'\n')
# Independent test oracle takes the existing native SunProbe lighting body,
# not TownLight. Only the verified rawRGB loader and constant planeY are adapted.
native=(HERE.parents[1]/'shaders/SunProbe.hlsl').read_text()
assert sha(HERE.parents[1]/'shaders/SunProbe.hlsl')=='f4cc2501f27acfa78ff5ea94afabcdc6ff2276ba599d4d0ccd66ba3ff31c45b8'
body=native[native.index('    float3 albedo = SatelliteDecodeAlbedo'):native.index('    colour=SatelliteExposureOrRaw')]
body=body.replace('SatelliteDecodeAlbedo(pixelCoord,int(captureWindow.w))','rawMaterial.Load(int3(pixelCoord,0)).rgb')
body=body.replace('albedoBuffer.Load(int3(pixelCoord,0)).ba','float2(0,rawMaterial.Load(int3(pixelCoord,0)).a)')
body=body.replace('normalBuffer.Load','rawNormal.Load').replace('normalize(sunDirection)','normalize(sunDirection.xyz)')
body=body.replace('ambientMap.Sample(ambientSampler,ambientUV)','ambientMap.SampleLevel(ambientSampler,ambientUV,0)')
body=body.replace('SatelliteAmbientUV(relativeWorld, worldOffset, worldSize)','SatelliteAmbientUV(relativeWorld, worldOffset.xyz, worldSize)')
header=(HERE/'TownCompose.hlsl').read_text().split('// Exact current native SunProbe closure')[0].replace('exposure,viewDirection,dispatchWindow','exposure,nativeViewDirection,dispatchWindow')
oracle=header+'\n#include "../../../shaders/OrthographicPosition.hlsl"\n[numthreads(8,8,1)] void main(uint3 id:SV_DispatchThreadID){\n int2 pixelCoord=int2(id.xy);float2 xz=tileBounds.xy+(float2(id.xy)+.5)/256*tileBounds.zw;float3 relativeWorld=float3(xz.x,5,xz.y)-referenceSide.xyz;float3 captureBack=nativeViewDirection.xyz;\n'+body+'\n acceptedHdr[id.xy]=float4(colour,1);\n}\n'
(OUT/'TownNativeOracle.hlsl').write_text(oracle)
oracleTarget=OUT/'TownNativeOracle.cso'
subprocess.run([str(FXC),'/nologo','/T','cs_5_0','/E','main','/O3','/I',str(GAME/'data/materials/common'),'/Fo',str(oracleTarget),str(OUT/'TownNativeOracle.hlsl')],check=True)
data=oracleTarget.read_bytes();(OUT/'TownNativeOracle.h').write_text('#pragma once\nstatic const unsigned char TownNativeOracle[]={\n'+','.join(str(b) for b in data)+'\n};\n')
