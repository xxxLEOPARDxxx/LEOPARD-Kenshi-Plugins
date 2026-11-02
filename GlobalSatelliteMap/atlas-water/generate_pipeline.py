"""Build the three fixed water-pipeline shaders from installed pinned assets.

generate() returns paths; it writes only atlas-water/_generated/pipeline.
Root's module generator owns integrating these arrays into the main DLL.
"""
from pathlib import Path
import hashlib
import json
import subprocess
import os
import sys
import generate_water

HERE=Path(__file__).resolve().parent
OUT=HERE/'_generated/pipeline'
FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
MATERIALS=Path('D:/SteamLibrary/steamapps/common/Kenshi/data/materials')
SHADERS=(
 ('AtlasTerrainHdrShader','TerrainHdr.cso',HERE.parent/'atlas-view/AtlasRelight.hlsl',['/D','DX11=1','/D','ATLAS_LINEAR_HDR=1']),
 ('AtlasWaterMaterialShader','Water.cso',HERE/'AtlasWater.hlsl',[]),
 ('AtlasWaterToneShader','Tone.cso',HERE/'ToneWater.hlsl',['/D','DX11=1']),
)
STAGE_FIXTURE=r'''cbuffer FixtureStage:register(b0){float4 addColour;};
Texture2D<float4> source:register(t0);RWTexture2D<float4> result:register(u0);
[numthreads(8,8,1)]void main(uint3 id:SV_DispatchThreadID){float4 v=source.Load(int3(id.xy,0));if(v.a>0)v.rgb+=addColour.rgb;result[id.xy]=v;}
'''

def generate():
    native=generate_water.generate()
    OUT.mkdir(parents=True,exist_ok=True)
    lines=['#pragma once','// Generated locally from pinned installed native assets.']
    records=[]
    for symbol,name,source,defines in SHADERS:
        target=OUT/name
        subprocess.run([str(FXC),'/nologo','/T','cs_5_0','/E','main','/O3','/Gec',*defines,
            '/I',str(MATERIALS/'common'),'/I',str(MATERIALS/'post'),'/Fo'+str(target),str(source)],check=True)
        code=target.read_bytes()
        lines+=['static const unsigned char '+symbol+'[]={',
            ',\n'.join(','.join(str(b) for b in code[i:i+24]) for i in range(0,len(code),24)),'};']
        records.append(dict(symbol=symbol,file=name,bytes=len(code),sha256=hashlib.sha256(code).hexdigest()))
    header=OUT/'WaterPipelineShaders.h'
    header.write_text('\n'.join(lines)+'\n')
    metadata=dict(privateGpuBytes=3408256,fixedWindowSide=256,waterScratchSide=512,
        sourceWaterSha256=native['waterSha256'],sourceLightingSha256=native['lightingSha256'],shaders=records)
    (OUT/'pipeline-shaders.json').write_text(json.dumps(metadata,indent=2)+'\n')
    # Test fixture only; never added to the production shader header or ABI.
    fixture=OUT/'StageAdd.hlsl';fixture.write_text(STAGE_FIXTURE)
    subprocess.run([str(FXC),'/nologo','/T','cs_5_0','/E','main','/O3','/Fo'+str(OUT/'StageAdd.cso'),str(fixture)],check=True)
    return dict(header=str(header),metadata=metadata)

def stage_mutants():
    """Run after run_pipeline_tests.ps1; compile real modified header copies."""
    vc=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');sdk=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
    env=dict(os.environ,INCLUDE=f'{vc}/include;{sdk}/Include;{HERE}',LIB=f'{vc}/lib/amd64;{sdk}/Lib/x64')
    source=(HERE/'WaterPipeline.h').read_text().replace('#include "AtlasWater.h"','#include "'+(HERE/'AtlasWater.h').as_posix()+'"')
    fixture=(HERE/'pipeline_warp.cpp').read_text().replace('#include "warp_tests.cpp"','#include "'+(HERE/'warp_tests.cpp').as_posix()+'"')
    stage='if(SUCCEEDED(hr)&&stage){SavedPredicate stagePredicate(c.p);SavedCompute stageSaved(c.p);hr=stage->apply(stage->context,cell,terrain,workView_.p,work_.p);}'
    tone='if(SUCCEEDED(hr))hr=Tone(c.p,terrain,output);'
    assert source.count(stage)==source.count(tone)==1
    edits=[
        ('omit-stage',stage,''),
        ('twice-stage','hr=stage->apply(stage->context,cell,terrain,workView_.p,work_.p);','hr=stage->apply(stage->context,cell,terrain,workView_.p,work_.p);if(SUCCEEDED(hr))hr=stage->apply(stage->context,cell,terrain,workView_.p,work_.p);'),
        ('stage-requires-water','if(!waterEnabled&&!stage)return S_FALSE;','if(!waterEnabled)return S_FALSE;'),
        ('water-inputs-when-off','if(stage&&!stage->apply)return E_INVALIDARG;','if(stage&&!stage->apply)return E_INVALIDARG;if(!waterInputs||!waterSamplers)return E_INVALIDARG;'),
        ('copy-on-stage-failure',tone,tone+'else Tone(c.p,terrain,output);'),
        ('ignore-stage-failure','hr=stage->apply(stage->context,cell,terrain,workView_.p,work_.p);','stage->apply(stage->context,cell,terrain,workView_.p,work_.p);'),
        ('omit-stage-state','SavedCompute stageSaved(c.p);',''),
        ('inherit-stage-predicate','SavedPredicate stagePredicate(c.p);',''),
    ]
    variants=[]
    for name,old,new in edits:
        assert source.count(old)==1,(name,source.count(old));variants.append((name,source.replace(old,new)))
    variants += [('stage-after-water',source.replace(stage,'').replace(tone,stage+tone)),('stage-after-exposure',source.replace(stage,'').replace(tone,tone+stage))]
    results=[]
    for name,body in variants:
        folder=OUT/'stage-mutants'/name;folder.mkdir(parents=True,exist_ok=True)
        header=folder/'WaterPipeline.h';header.write_text(body);cpp=folder/'fixture.cpp';cpp.write_text(fixture.replace('#include "WaterPipeline.h"','#include "'+header.as_posix()+'"'))
        exe=folder/'fixture.exe';built=subprocess.run([str(vc/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',f'/Fo{folder}/fixture.obj',f'/Fe{exe}',str(cpp),'/link','d3d11.lib'],env=env,capture_output=True,text=True);(folder/'build.log').write_text(built.stdout+built.stderr)
        assert built.returncode==0,(name,'compile failure',built.stdout,built.stderr)
        args=[OUT/n for n in ['TerrainHdr.cso','TerrainOracle.cso','Water.cso','NativeWater.cso','ReferenceVS.cso','ReferenceTerrainPS.cso','Tone.cso','ToneOracle.cso','TerrainDefault.cso']]
        ran=subprocess.run([str(exe),*(str(p) for p in args)],capture_output=True,text=True);diagnostic=ran.stdout+ran.stderr;(folder/'run.log').write_text(diagnostic)
        assert ran.returncode!=0 and 'FAIL ' in diagnostic,(name,'survived/unfair failure',ran.returncode,diagnostic)
        reason=next(x for x in diagnostic.splitlines() if 'FAIL ' in x);print('REJECTED',name,reason);results.append(dict(name=name,exitCode=ran.returncode,reason=reason))
    (OUT/'stage-mutants.json').write_text(json.dumps(results,indent=2)+'\n');print('PASS',len(results),'compiled HDR-stage functional mutants')

if __name__=='__main__':
    if sys.argv[1:]==['--stage-mutants']:stage_mutants()
    else:print(json.dumps(generate(),indent=2))
