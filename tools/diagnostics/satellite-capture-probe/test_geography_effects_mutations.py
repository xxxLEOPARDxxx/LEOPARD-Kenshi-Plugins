"""Generated-copy GPU mutants; production files and native reference stay intact."""
from pathlib import Path
import subprocess
from geography_vertex_source import owned_vertex
here=Path(__file__).resolve().parent;out=here/'build/geography'
fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
helper=(here/'GeographyCliff.hlsl').read_text();fixture=(here/'geography_effects_fixture.hlsl').read_text()
mutants=[]
mutants.append(('abs-amplitude',owned_vertex(helper.replace('*distortion[sid].y','*abs(distortion[sid].y)')),fixture))
mutants.append(('two-pi',owned_vertex(helper.replace('*distortion[sid].x','*(distortion[sid].x*6.283185307)')),fixture))
mutants.append(('wrong-sid-component',owned_vertex(helper.replace('sid==index+1u','sid==(index+1u)%15u+1u')),fixture))
mutants.append(('lost-morph',owned_vertex(helper).replace('position.y = lerp(position.y, position.w, morph);','position.y = position.y;'),fixture))
mutants.append(('world-phase',owned_vertex(helper).replace('GeographyCliffHeights(position.xyz,','GeographyCliffHeights(mul(worldMatrix,position).xyz,'),fixture))
start,end=helper.rsplit('    return result;',1)
mutants.append(('derivative-after-sid',owned_vertex(start+'    return float3(result.x,ddx(result.x),ddy(result.x));'+end),fixture))
pixel=fixture.replace('float3 cliff=GeographyCliffSelect(prepared,sid+1u);','float4 params=fixtureDistortion[sid];float height=i.coord.z+(cos(i.coord.x*5000.0*params.x)+cos(i.coord.y*5000.0*params.x))*params.y;float3 cliff=float3(height,ddx(height),ddy(height));')
mutants.append(('pixel-cosine',owned_vertex(helper),pixel))
def run(args):return subprocess.run([str(v) for v in args],capture_output=True,text=True,creationflags=subprocess.CREATE_NO_WINDOW)
for name,body,wrapper in mutants:
    source=out/('effects-'+name+'.hlsl');source.write_text(wrapper.replace('// VERTEX_FUNCTION',body.replace('main_vs (','OwnedMainVs (').replace('uniform ','')))
    compiled=[]
    for stage,entry in (('vs','fixture_vs'),('ps','fixture_ps')):
        path=out/('effects-'+name+'-'+stage+'.cso');compiled.append(path)
        result=run([fxc,'/nologo','/T',stage+'_4_0','/E',entry,'/O3','/Gec','/D','TEXTURED=1','/Fo'+str(path),source]);assert result.returncode==0,(name,result.stdout,result.stderr)
    result=run([out/'geography_effects_warp.exe',out/'effects-reference-vs.cso',compiled[0],out/'effects-reference-ps.cso',compiled[1],out/'effects-distortion.bin'])
    (out/('effects-'+name+'.log')).write_text(result.stdout+result.stderr)
    assert result.returncode!=0 and 'FAIL native ' in result.stdout+result.stderr,(name,result.returncode,result.stdout,result.stderr)
    print('PASS effects mutant:',name,'fails actual native-source GPU parity')
