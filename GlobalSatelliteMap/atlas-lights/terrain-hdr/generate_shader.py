"""Pinned native fragment/common sources; generated bodies remain private/ignored."""
from pathlib import Path
import hashlib,json,re
HERE=Path(__file__).resolve().parent;GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi');OUT=HERE/'_generated';OUT.mkdir(parents=True,exist_ok=True)
profile=(HERE.parent/'NativeLocalLightProfile.h').read_text()
pins=re.findall(r'\{"(data/[^\"]+)","([0-9a-f]{64})"\}',profile)
if len(pins)!=7:raise ValueError('expected seven native local-light source pins')
for relative,expected in pins:
    if hashlib.sha256((GAME/relative).read_bytes()).hexdigest()!=expected:raise ValueError('native source differs: '+relative)
native=(GAME/'data/materials/deferred/deferred.hlsl').read_text();start=native.index('// Attenuation',native.index('float4 light_fs('));end=native.index('\n\t#ifdef MODE',start);tail=native[start:end]
if tail.count('#ifdef SPOTLIGHT')!=1 or tail.count('#endif')!=1 or tail.count('float3 viewDir = -normalize( worldPos );')!=1:raise ValueError('native local fragment cutpoint differs')
tail=tail.replace('#ifdef SPOTLIGHT','if(light.directionType.w==2) {').replace('#endif','}').replace('float3 viewDir = -normalize( worldPos );','float3 viewDir = normalize(parallelView.xyz);')
tail+='\nreturn color;\n'
common=(GAME/'data/materials/common/constants.hlsl').read_text()+'\n'+(GAME/'data/materials/common/lightingFunctions.hlsl').read_text()
(OUT/'NativeLocal.hlsl').write_text(common)
source=(HERE/'LocalLights.hlsl').read_text();a=source.index('// BEGIN NATIVE LOCAL TAIL');b=source.index('// END NATIVE LOCAL TAIL',a);source=source[:a]+'// BEGIN NATIVE LOCAL TAIL\n'+tail+' '+source[b:];(OUT/'LocalLights.generated.hlsl').write_text(source.replace('"_generated/NativeLocal.hlsl"','"NativeLocal.hlsl"'))
(OUT/'source-proof.json').write_text(json.dumps({'sourcePins':dict(pins),'tailSha256':hashlib.sha256(tail.encode()).hexdigest(),'adaptations':['known absolute surface position','declared top-down view direction','runtime point/spot branch','fragment clip becomes zero contribution'],'runtimeCalls':0},indent=2)+'\n')
print('Local-light shader source pins/extracted native attenuation+spot+BRDF PASS')
