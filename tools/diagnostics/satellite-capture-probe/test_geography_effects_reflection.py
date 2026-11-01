"""Actual production DXBC VS/PS interface and existing native binding checks."""
from pathlib import Path
import re,subprocess,sys
fxc='C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe'
vs,ps=[subprocess.check_output([fxc,'/dumpbin',p],text=True,errors='replace') for p in sys.argv[1:3]]
def signature(text,label):
    body=text.split('// '+label+' signature:',1)[1].split('//\n//\n',1)[0]
    return {(m[1],int(m[2])):(m[3],int(m[4]),m[5],m[6]) for line in body.splitlines() for m in [re.fullmatch(r'//\s+(\w+)\s+(\d+)\s+(\w+)\s+(\d+)\s+(\w+)\s+(\w+)\s*.*',line)] if m}
inputs=signature(vs,'Input');outputs=signature(vs,'Output');pixels=signature(ps,'Input')
assert inputs=={('POSITION',0):('xyzw',0,'NONE','float'),('NORMAL',0):('xyz',1,'NONE','float'),('BINORMAL',0):('xyz',2,'NONE','float')},inputs
expected={('SV_Position',0):('xyzw',0,'POS','float')}
for i,mask in enumerate(('xyz','xyz','xyz','xyzw','xy','xyzw','xyzw','xyzw','xyzw')):expected['TEXCOORD',i]=(mask,i+1,'NONE','float')
assert outputs==expected and pixels==expected,(outputs,pixels,expected)
assert re.search(r'float4 geoDistortion\[15\];\s*// Offset:\s+144 Size:\s+240',vs)
for name,offset,size in [('worldViewProjMatrix',0,64),('worldMatrix',64,64),('morph',128,4),('biomeData',384,16),('overlayData',400,16)]:
    assert re.search(r'\b'+name+r';\s*// Offset:\s+'+str(offset)+r' Size:\s+'+str(size),vs),name
assert not re.search(r'^//\s+\w+\s+(texture|sampler)\s',vs,re.M),'no VS textures/samplers'
assembly=ps.split('\nps_4_0\n',1)[1]
first_branch=re.search(r'^if_(?:z|nz)\b',assembly,re.M).start()
packed_derivatives=list(re.finditer(r'^deriv_rt[xy] [^\n]*, v[6789]\.',assembly,re.M))
assert len(packed_derivatives)==8 and all(m.start()<first_branch for m in packed_derivatives),'actual DXBC packed derivatives precede every selection branch'
here=Path(__file__).resolve().parent;binding=(here/'PrivateGeography.h').read_text();embedding=(here/'embed_geography_shader.py').read_text()
for code in ('cache.Inspect(owner,1,existing.get(),GeographyVertexSource)','cache.Record(owner,1,vertexProgram.get(),GeographyVertexSource)','cache.Owns(1,vertexProgram.get())','pass->setVertexProgram(VertexName())','"worldViewProjMatrix",Ogre::GpuProgramParameters::ACT_WORLDVIEWPROJ_MATRIX','"worldMatrix",Ogre::GpuProgramParameters::ACT_WORLD_MATRIX','"morph",Ogre::GpuProgramParameters::ACT_CUSTOM,10','"geoDistortion",distortion,15,4'):
    assert code in binding,code
assert embedding.index('GeographyCliffPrepare(texCoordsV')<embedding.index('[unroll] for(uint slot=0;slot<4;'), 'all gradients before divergent selection'
assert 'gradX.z=cliff.y;gradY.z=cliff.z' in embedding
print('PASS actual production VS/PS TEXCOORD0..8 masks/registers,15records/16Bstride/240Btotal, all8packedDXBCderivatives beforebranches,native matrix/morph10 bindings,existingcacheSlot1,noVS SRVs')
