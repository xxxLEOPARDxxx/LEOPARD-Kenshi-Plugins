"""Five generated-copy native-fade mutants must fail float GPU readback."""
from pathlib import Path
import subprocess
here=Path(__file__).resolve().parent;out=here/'build/geography'
cl='C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC/bin/amd64/cl.exe';fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
def run(args):return subprocess.run([str(x) for x in args],capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
def expect(exe,shader,name):
    result=run([exe,out/'fade-vs.cso',out/'fade-reference-ps.cso',shader]);(out/('fade-'+name+'.log')).write_text(result.stdout+result.stderr)
    assert result.returncode!=0 and 'native fade float RGBA/normal-alpha/absorbance/order parity' in result.stdout+result.stderr,(name,result.returncode,result.stdout,result.stderr)
    print('PASS fade mutant:',name,'fails native float material parity')
policy=(here/'GeographyPolicy.h').read_text()
swapped=policy.replace('float((argb>>16)&255u)','RED_COMPONENT').replace('float(argb&255u)','float((argb>>16)&255u)').replace('RED_COMPONENT','float(argb&255u)')
for name,text in [('argb-swap',swapped),('raw-distance',policy.replace('distance>0?1.0f/distance:distance','distance'))]:
    assert text!=policy
    header=out/('fade-'+name+'.h');header.write_text(text);exe=out/('fade-'+name+'.exe')
    result=run([cl,'/nologo','/EHsc','/MD','/O2',f'/DGEOGRAPHY_FADE_HEADER="{header.as_posix()}"','/Fe'+str(exe),'/Fo'+str(out/('fade-'+name+'.obj')),here/'geography_fade_warp.cpp','/link','d3d11.lib']);assert result.returncode==0,(name,result.stdout,result.stderr)
    expect(exe,out/'fade-owned-ps.cso',name)
source=(out/'fade-owned.hlsl').read_text();rgb='albedo.rgb = lerp(albedo.rgb, distant.rgb, fadeout);';normal='n = lerp(n, float4(0.5,0.5,1.0,1.0), fadeout);'
assert source.count(rgb)==1 and source.count(normal)==1
late=source.replace(rgb,'').replace(normal,'').replace('// return result',rgb+'\n'+normal+'\n// return result')
for name,text in [('albedo-alpha',source.replace(rgb,'albedo = lerp(albedo, distant, fadeout);')),('normal-alpha',source.replace(normal,'n.rgb = lerp(n.rgb, float3(0.5,0.5,1.0), fadeout);')),('after-road-cliff',late)]:
    assert text!=source
    hlsl=out/('fade-'+name+'.hlsl');hlsl.write_text(text);shader=out/('fade-'+name+'.cso')
    result=run([fxc,'/nologo','/T','ps_4_0','/E','fixture_ps','/O3','/Gec','/Fo'+str(shader),hlsl]);assert result.returncode==0,(name,result.stdout,result.stderr)
    expect(out/'geography_fade_warp.exe',shader,name)
