"""Compile real negative controls; require the WARP executable to reject each."""
from pathlib import Path
import json
import os
import subprocess

HERE=Path(__file__).resolve().parent
OUT=HERE/'_generated'
FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')

def swap(text,old,new,count=1):
    if text.count(old)!=count:raise ValueError('negative-control site changed: '+old)
    return text.replace(old,new)

def run():
    root=OUT/'mutants';root.mkdir(exist_ok=True)
    env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include;{HERE}'
    env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
    header=(HERE/'AtlasWater.h').read_text().replace('../atlas-view/AtlasRelight.h',(HERE.parent/'atlas-view/AtlasRelight.h').as_posix())
    warp=(HERE/'warp_tests.cpp').read_text().replace('../atlas-view/ContextFixture.h',(HERE.parent/'atlas-view/ContextFixture.h').as_posix())
    shader=(HERE/'AtlasWater.hlsl').read_text()
    material=(OUT/'NativeWaterMaterial.hlsl').read_text()
    cases=[
        ('omit-compute-restore','header',lambda s:swap(s,'c_->CSSetShader(shader_,classes_,count_);','/* deliberately omit original CS */')),
        ('reset-native-append-counter','header',lambda s:swap(s,'c_->CSSetUnorderedAccessViews(0,1,&uav_,NULL);','UINT reset=0;c_->CSSetUnorderedAccessViews(0,1,&uav_,&reset);')),
        ('fixed-initial-water-height','shader',lambda s:swap(s,'ground>waterPlane.x','ground>100.0f')),
        ('restart-local-pixel-coordinates','shader',lambda s:swap(s,'id.xy+uint2(sourceWindow.yz)','id.xy')),
        ('clip-linear-hdr','shader',lambda s:swap(s,'float4(water.rgb,base.a)','float4(saturate(water.rgb),base.a)')),
        ('remove-stationary-flow-guard','material',lambda s:swap(s,'speed > 0.0f ? direction / speed : float2(0,0)','direction / speed',2)),
        ('force-normal-mip-zero','material',lambda s:swap(s,'map.SampleGrad(mapSampler,coord + direction * speed * t,baseDx + flowDx*t,baseDy + flowDy*t)','map.SampleLevel(mapSampler,coord + direction * speed * t,0)')),
        ('wrong-native-reflection-fold','material',lambda s:swap(s,'float3 rf = 1.0f;','float3 rf = 0.0f;')),
    ]
    results=[]
    for name,kind,mutate in cases:
        directory=root/name;directory.mkdir(exist_ok=True);(directory/'_generated').mkdir(exist_ok=True)
        (directory/'AtlasWater.h').write_text(mutate(header) if kind=='header' else header)
        (directory/'warp_tests.cpp').write_text(warp)
        (directory/'WaterUniforms.hlsl').write_bytes((HERE/'WaterUniforms.hlsl').read_bytes())
        (directory/'AtlasWater.hlsl').write_text(mutate(shader) if kind=='shader' else shader)
        (directory/'_generated/NativeWaterMaterial.hlsl').write_text(mutate(material) if kind=='material' else material)
        exe=directory/'warp.exe';cso=directory/'AtlasWater.cso'
        if kind=='header':
            command=[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W4',f'/Fo{directory}/warp.obj',f'/Fe{exe}',str(directory/'warp_tests.cpp'),'/link','d3d11.lib']
            subprocess.run(command,env=env,check=True,capture_output=True)
            cso=OUT/'AtlasWater.cso'
        else:
            subprocess.run([str(FXC),'/nologo','/T','cs_5_0','/E','main','/O3','/Gec',f'/Fo{cso}',str(directory/'AtlasWater.hlsl')],check=True,capture_output=True)
            exe=OUT/'warp.exe'
        result=subprocess.run([str(exe),str(cso),str(OUT/'NativeWater.cso'),str(OUT/'ReferenceVS.cso'),str(OUT/'ReferenceTerrainPS.cso')],capture_output=True,text=True)
        diagnostic=(result.stdout+result.stderr).strip()
        if result.returncode==0 or 'FAIL ' not in diagnostic:raise AssertionError(f'{name}: expected functional WARP rejection, got {result.returncode}: {diagnostic}')
        results.append(dict(name=name,exitCode=result.returncode,diagnostic=diagnostic))
        print('REJECTED',name,diagnostic)
    (OUT/'mutations.json').write_text(json.dumps(results,indent=2)+'\n')
    print(f'PASS {len(results)} independently compiled functional negative controls')

if __name__=='__main__':run()
