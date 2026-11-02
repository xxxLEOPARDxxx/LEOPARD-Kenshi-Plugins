"""Generated copies only: omitted mip, wrong bank layer, wrong blend channel/center."""
from pathlib import Path
import os,subprocess
here=Path(__file__).resolve().parent;out=here/'build/geography'
cl='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC/bin/amd64/cl.exe'
fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
def run(args):return subprocess.run([str(x) for x in args],capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
def expect(args,text,name):
    result=run(args);(out/(name+'.log')).write_text(result.stdout+result.stderr)
    assert result.returncode!=0 and text in result.stdout+result.stderr,(name,result.returncode,result.stdout,result.stderr)
    print('PASS geography mutant:',name,'fails',text)
source=(here/'GpuCompressedUpload.h').read_text()
completion=(here/'GeographyCompletion.h').read_text()
production=(here/'PrivateGeography.h').read_text()
assert 'SatelliteGeography::CompleteBank(operations,uncertain)' in production
assert 'if(uncertain)throw std::runtime_error("geography native allocation/credit uncertain; bounded quarantine retained")' in production
for name,needle,replacement,message in (
    ('omit-geography-notify','ops.Notify();','/* omit native credit in generated copy */','one credit and Loaded after all layers'),
    ('early-geography-clear','ops.Notify();','uncertain=false;ops.Notify();','uncertainty latched before native credit')):
    assert completion.count(needle)==1
    header=out/(name+'.h');header.write_text(completion.replace(needle,replacement));exe=out/(name+'.exe')
    result=run([cl,'/nologo','/EHsc','/MD','/O2',f'/DGEOGRAPHY_COMPLETION_HEADER="{header.as_posix()}"',f'/Fe{exe}',f'/Fo{out/(name+".obj")}',here/'geography_completion_tests.cpp'])
    assert result.returncode==0,(result.stdout,result.stderr)
    expect([exe],message,name)
for name,needle,replacement in (
    ('wrong-layer','D3D11CalcSubresource(mip,layer,9)','D3D11CalcSubresource(mip,0,9)'),
    ('omit-last-mip','for(unsigned mip=0;mip<9;++mip)\n        context.p->UpdateSubresource','for(unsigned mip=0;mip<8;++mip)\n        context.p->UpdateSubresource'),
    ('omit-rgba-mips','context.p->GenerateMips(view);','/* omitted GenerateMips in test-only copy */')):
    assert source.count(needle)==1
    header=out/(name+'.h');header.write_text(source.replace(needle,replacement));exe=out/(name+'.exe')
    result=run([cl,'/nologo','/EHsc','/MD','/O2',f'/DGEOGRAPHY_UPLOAD_HEADER="{header.as_posix()}"',f'/Fe{exe}',f'/Fo{out/(name+".obj")}',here/'geography_upload_tests.cpp','/link','d3d11.lib'])
    assert result.returncode==0,(result.stdout,result.stderr)
    expect([exe,'D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/textures/GrassBadTestDIF.dds'],
        'RGBA2D all mips/alpha generated' if name=='omit-rgba-mips' else 'exact source BC block bytes/layer/mip',name)
helper=(here/'GeographySampling.hlsl').read_text();wrapper=(here/'geography_sampling_cs.hlsl').read_text()
for name,needle,replacement,message in (
    ('wrong-channel','blendWeights[selected[i]>>4u]','blendWeights[0]','native blend residual/channel weights'),
    ('patch-center','(worldXZ-selectorRegion.xy)','(float2(-36864.0,-36864.0)-selectorRegion.xy)','actual geographic world SID/channel selection')):
    assert helper.count(needle)==1
    hlsl=out/(name+'.hlsl');hlsl.write_text(wrapper.replace('#include "GeographySampling.hlsl"',helper.replace(needle,replacement)))
    compiled=out/(name+'.cso');result=run([fxc,'/nologo','/T','cs_5_0','/E','main','/O3','/Ges','/Fo'+str(compiled),hlsl]);assert result.returncode==0,(result.stdout,result.stderr)
    expect([out/'geography_sampling_warp.exe',compiled,out/'sampling.points',out/'sampling.expected',out/'blend.rgba'],message,name)
