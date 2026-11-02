"""Compile functional negatives against the production pipeline and native oracles."""
from pathlib import Path
import json,os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated/pipeline'
FXC='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
MATERIALS=Path('D:/SteamLibrary/steamapps/common/Kenshi/data/materials')
def swap(s,old,new):
    if s.count(old)!=1:raise ValueError('mutation site changed: '+old)
    return s.replace(old,new)
def run():
    root=OUT/'mutants';root.mkdir(exist_ok=True)
    env=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include;{HERE}',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
    header=(HERE/'WaterPipeline.h').read_text().replace('#include "AtlasWater.h"','#include "'+(HERE/'AtlasWater.h').as_posix()+'"')
    fixture=(HERE/'pipeline_warp.cpp').read_text().replace('#include "warp_tests.cpp"','#include "'+(HERE/'warp_tests.cpp').as_posix()+'"')
    tone=(HERE/'ToneWater.hlsl').read_text().replace('#include "../shaders/NativeExposure.hlsl"','#include "'+(HERE.parent/'shaders/NativeExposure.hlsl').as_posix()+'"')
    terrain=(HERE.parent/'atlas-view/AtlasRelight.hlsl').read_text().replace('#include "../shaders/NativeExposure.hlsl"','#include "'+(HERE.parent/'shaders/NativeExposure.hlsl').as_posix()+'"')
    cases=[
      ('omit-water-composition','header','if(SUCCEEDED(hr))hr=water_.Render(water,source,waterSamplers,work_.p,callerOwnedBytes+PipelinePrivateBytes-PrivateBytes);','/* missing water */'),
      ('reject-native-one-mip-cubes','header','AtlasView::LightView(waterInputs[p],device_.p,p>=3,1)','AtlasView::LightView(waterInputs[p],device_.p,p>=3,p<3?1:p==3?4:8)'),
      ('omit-tone-state-restore','header','SavedCompute saved(c);','/* missing tone state save/restore */'),
      ('inherit-false-native-predicate','header','SavedPredicate predicate(c);','/* missing tone predicate scope */'),
      ('allow-readonly-display-input-alias','header','!AtlasView::Identity(r.p,output)&&',''),
      ('freeze-initial-water-height','header','if(SUCCEEDED(hr))hr=water_.Render(water,source,waterSamplers,work_.p,callerOwnedBytes+PipelinePrivateBytes-PrivateBytes);','Uniforms frozen=water;frozen.waterPlane[0]=100;if(SUCCEEDED(hr))hr=water_.Render(frozen,source,waterSamplers,work_.p,callerOwnedBytes+PipelinePrivateBytes-PrivateBytes);'),
      ('double-exposure','tone','float3 colour=SatelliteNativeExposure(raw.rgb,toneExposure.xy);','float3 first=SatelliteNativeExposure(raw.rgb,toneExposure.xy);float3 colour=SatelliteNativeExposure(first,toneExposure.xy);'),
      ('force-opaque-cache-coverage','tone','float4(colour,raw.a)','float4(colour,1)'),
      ('expose-terrain-before-water','terrain','#ifndef ATLAS_LINEAR_HDR','#if 1'),
      ('clip-linear-hdr-before-water','terrain','float3 colour=direct.specular+environment.specular+(direct.diffuse+environment.diffuse)*albedo;','float3 colour=saturate(direct.specular+environment.specular+(direct.diffuse+environment.diffuse)*albedo);'),
    ]
    results=[]
    for name,kind,old,new in cases:
        folder=root/name;folder.mkdir(exist_ok=True)
        exe=OUT/'warp.exe';args=[OUT/n for n in ['TerrainHdr.cso','TerrainOracle.cso','Water.cso','NativeWater.cso','ReferenceVS.cso','ReferenceTerrainPS.cso','Tone.cso','ToneOracle.cso','TerrainDefault.cso']]
        if kind=='header':
            h=folder/'WaterPipeline.h';h.write_text(swap(header,old,new));cpp=folder/'fixture.cpp';cpp.write_text(fixture.replace('#include "WaterPipeline.h"','#include "'+h.as_posix()+'"'))
            exe=folder/'fixture.exe'
            subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',f'/Fo{folder}/fixture.obj',f'/Fe{exe}',str(cpp),'/link','d3d11.lib'],env=env,check=True,capture_output=True)
        else:
            src=folder/'shader.hlsl';src.write_text(swap(tone if kind=='tone' else terrain,old,new));code=folder/'shader.cso'
            defines=['/D','DX11=1']+(['/D','ATLAS_LINEAR_HDR=1'] if kind=='terrain' else [])
            subprocess.run([FXC,'/nologo','/T','cs_5_0','/E','main','/O3','/Gec',*defines,'/I',str(MATERIALS/'common'),'/I',str(MATERIALS/'post'),'/Fo'+str(code),str(src)],check=True,capture_output=True)
            args[6 if kind=='tone' else 0]=code
        result=subprocess.run([str(exe),*(str(a) for a in args)],capture_output=True,text=True)
        diagnostic=(result.stdout+result.stderr).strip()
        if result.returncode==0 or 'FAIL ' not in diagnostic:raise AssertionError(f'{name}: expected functional rejection, got{result.returncode}: {diagnostic}')
        print('REJECTED',name,diagnostic);results.append(dict(name=name,exitCode=result.returncode,diagnostic=diagnostic))
    (OUT/'pipeline-mutations.json').write_text(json.dumps(results,indent=2)+'\n')
    print(f'PASS {len(results)} independently compiled functional pipeline negative controls')
if __name__=='__main__':run()
