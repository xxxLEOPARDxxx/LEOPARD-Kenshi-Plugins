"""Pinned native shaders and ABI evidence. Writes only this helper's outputs."""
from pathlib import Path
import hashlib,importlib.util,json,subprocess,struct
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi');FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
spec=importlib.util.spec_from_file_location('pe',HERE.parents[1]/'tests/test_pe_contract.py');pe=importlib.util.module_from_spec(spec);spec.loader.exec_module(pe)
pins={'data/materials/deferred/objects.hlsl':'e8511d4bf9d5cf17b7b4dadec33bc6b6a52a9c4b37353f3c206c349c65e70cb8','data/materials/deferred/objects.program':'b5867b400db10248e2a7363f4e5edeaf6b5a9bda28ab8e92f1c000a7a2e10591','data/materials/deferred/objects.material':'5f647ddd97651aa66fe1c2cfd008c78b2d5d78391e972d41a95b3fe2ea4c5551','data/materials/deferred/buildings.material':'0805ea3d1c4bfcc2582d4dea37224f137ad9e66efdd8c1f9c1c76b644c0fe2f4','data/materials/deferred/gbuffer.hlsl':'d6a525658f224379269369c9adaa928fc82a72285aed5cffb21d077df4c9c943','data/materials/common/wet.hlsl':'e9042fbf57df1463d014761576701dd3c7e8680f70c89adcb9196ffdb5580d11','data/materials/common/common.program':'85c10a97ad80b8cc3d74db2c23da673a1f61453bb5e37481dc9d364eb1143183','data/materials/black.dds':'7b42094c0ed83cff8e7daaa98f2b259105cbbed77a632e10f8a2cb77bc706e30'}
for path,sha in pins.items():assert hashlib.sha256((GAME/path).read_bytes()).hexdigest()==sha,path
names=['Plaster-Concrete_DIF.dds','Plaster-Concrete_NML.dds','Plaster-Walls_DIF.dds','Plaster-Walls_NML.dds','Concrete_Damaged_DIF.dds','Concrete_Damaged_NML.dds','Wall4_Diff.dds','Wall4_NM.dds','Gate4_Diff.dds','Gate4_NM.dds','market _d.dds','market _n.dds','Turbulent.dds']
files={}
for p in (GAME/'data').rglob('*.dds'):
 if p.name.casefold() in {n.casefold() for n in names}:files.setdefault(p.name.casefold(),[]).append(p)
for n in names:
 matches=files.get(n.casefold(),[]);assert len(matches)==1,(n,matches)
 p=matches[0];pins[p.relative_to(GAME).as_posix()]=hashlib.sha256(p.read_bytes()).hexdigest()
arrays=['#pragma once'];shaders=[]
for name,target,entry,defines in [('InstanceVP80','vs_4_0','main_vs',['INSTANCED']),('InstanceVP83','vs_4_0','main_vs',['COLOURING','INSTANCED']),('InstanceFP43','ps_4_0','main_ps',['DX11','DUST','DUAL_TEXTURE','COLOURING']),('InstanceFP48','ps_4_0','main_ps',['DX11','DUST','TRANSPARENCY'])]:
 path=OUT/(name+'.cso');args=[str(FXC),'/nologo','/T',target,'/E',entry,'/O3']
 for define in defines:args+=['/D',define+'=1']
 subprocess.run(args+['/I',str(GAME/'data/materials/common'),'/Fo',str(path),str(GAME/'data/materials/deferred/objects.hlsl')],check=True)
 data=path.read_bytes();arrays+=['static const unsigned char '+name+'[]={',','.join(str(b) for b in data),'};']
 dump=subprocess.check_output([str(FXC),'/dumpbin',str(path)],text=True);(OUT/(name+'.reflection.txt')).write_text(dump)
 assert 'Offset:   32 Size:    64' in dump if target.startswith('vs') else 'Offset:   44 Size:     4' in dump
 if name=='InstanceFP43':assert 't6' in dump and 't7' not in dump
 if name=='InstanceFP48':assert 'threshold' in dump and 'Offset:   28 Size:     4' in dump and 't3' in dump
 shaders.append(dict(name=name,defines=defines,bytes=len(data),sha256=hashlib.sha256(data).hexdigest()))
(OUT/'InstanceShaders.h').write_text('\n'.join(arrays)+'\n')
# A separate explicit output mode. The four original bytecodes/header above
# remain native and unchanged. Change ONLY the two final material writers;
# native sampling, black-metal wetness, clip, dust, gloss, normal alpha and depth
# stay in the source exactly as installed.
original=(GAME/'data/materials/deferred/objects.hlsl').read_bytes()
writer_changes=[(b'writeAlbedo   ( buffer, diffuse.rgb, fragCoord.xy );',b'buffer.buf0.rgb = diffuse.rgb;'),(b'writeMetalness( buffer, metalness );',b'/* Verified black metalness is not stored over full RGB. */')]
rgb_source=original
for before,after in writer_changes:
 assert rgb_source.count(before)==1,before
 rgb_source=rgb_source.replace(before,after)
rgb_path=OUT/'InstanceRawRgb.hlsl';rgb_path.write_bytes(rgb_source)
rgb_arrays=['#pragma once'];rgb_shaders=[]
for kind,defines in [(43,['DX11','DUST','DUAL_TEXTURE','COLOURING']),(48,['DX11','DUST','TRANSPARENCY'])]:
 name='InstanceFP%dRgb'%kind;path=OUT/(name+'.cso');args=[str(FXC),'/nologo','/T','ps_4_0','/E','main_ps','/O3']
 for define in defines:args+=['/D',define+'=1']
 subprocess.run(args+['/I',str(GAME/'data/materials/common'),'/I',str(GAME/'data/materials/deferred'),'/Fo',str(path),str(rgb_path)],check=True)
 data=path.read_bytes();rgb_arrays+=['static const unsigned char '+name+'[]={',','.join(str(b) for b in data),'};']
 dump=subprocess.check_output([str(FXC),'/dumpbin',str(path)],text=True);(OUT/(name+'.reflection.txt')).write_text(dump)
 assert 'Offset:   44 Size:     4' in dump
 if kind==43:assert 't6' in dump and 't7' not in dump
 if kind==48:assert 'threshold' in dump and 'Offset:   28 Size:     4' in dump and 't3' in dump
 rgb_shaders.append(dict(name=name,defines=defines,bytes=len(data),sha256=hashlib.sha256(data).hexdigest()))
(OUT/'InstanceRgbShaders.h').write_text('\n'.join(rgb_arrays)+'\n')
source=(GAME/'data/materials/deferred/objects.hlsl').read_bytes();(OUT/'InstanceSource.h').write_text('#pragma once\nstatic const unsigned char InstanceSource[]={'+','.join(str(x) for x in source)+'};\n')
native=[('RE_Kenshi/Kenshi_x64.exe','504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e',[0x569960,0x933213,0x9334aa]),('OgreMain_x64.dll','7d3888d21398647519f10a89b1f300d8606f761ad159e6021a797b1b9b3df5f6',[0x125b80,0x124d20,0x124da0,0x124db0,0x124dc5,0x124dd0,0x130760,0x130775,0x143290,0x1432be,0x1432c8,0x143309,0x143c80,0x1dbd70,0x389e70,0x38a1e0,0x216520,0x216850,0xf95b0,0x3e090,0xdddc0,0xdebf0,0x2ca206,0x2ca230,0x2ca260,0x1260e1,0x126176,0x142904,0x14291f]),('RenderSystem_Direct3D11_x64.dll','5f224882ded5b6d3f68f14d6bb4e1762267bbb602b7b9733bd153663d1541bb9',[0x12c29,0x12c89,0x12cd9,0x12d2f,0x12d7f,0x28e50,0x28e68,0x28e9c,0x28eb4])]
code=['#pragma once','struct InstanceCodePin{unsigned module;DWORD rva;BYTE bytes[24];};','static const InstanceCodePin InstancePins[]={'];windows=[]
for index,(path,sha,rvas) in enumerate(native):
 image=pe.PE(GAME/path);assert hashlib.sha256(image.data).hexdigest()==sha
 if index==1:
  rvas=rvas+[0xdec1e,0xded41,0xded56]
  expected={'?_getOwner@InstancedEntity':0x125b80,'?_getMeshReference@InstanceBatch':0x124d20,'?getMaterial@InstanceBatch':0x124da0,'?getRenderOperation@InstanceBatch':0x124db0,'?isInUse@InstancedEntity':0x143c80,'?getVisible@MovableObject':0x1dbd70,'?getSingleton@GpuProgramManager':0xdddc0,'?getSharedParameters@GpuProgramManager':0xdebf0,'?getSceneManager@InstanceManager':0x3e090,'?getWorldTransforms@InstanceBatchHW':0x130760,'?getTransforms3x4@InstancedEntity':0x143290}
  exports=image.exports()
  for name,rva in expected.items():
   found=[value for key,value in exports.items() if key.startswith(name+'@')]
   assert len(found)==1 and found[0][0]==rva,(name,found)
  assert struct.unpack_from('<16f',image.data,image.offset(0x8d5ee0))==(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1)
  rvas=rvas+[0x8d5ee0,0x8d5ef8,0x8d5f10]
 if index==2:
  # Native VertexElement switch: all SDK float/short/colour enum mappings;
  # specifically COLOUR_ABGR11 targets MOV EAX,28 at28E6B.
  targets=[struct.unpack_from('<I',image.data,image.offset(0x28e9c)+4*n)[0] for n in range(12)]
  assert targets[11]==0x28e6b and image.data[image.offset(targets[11]):image.offset(targets[11])+6]==bytes.fromhex('b81c000000c3')
 for r in rvas:
  data=image.data[image.offset(r):image.offset(r)+24];code.append('{%d,0x%x,{%s}},'%(index,r,','.join(str(x) for x in data)));windows.append(dict(module=index,rva=hex(r),code24=data.hex()))
code+=['};'];(OUT/'InstanceRuntimeProfile.h').write_text('\n'.join(code)+'\n')
(OUT/'InstanceAssetPins.h').write_text('#pragma once\nstruct InstanceAssetPin{const char*path;const char*sha256;};\nstatic const InstanceAssetPin InstanceAssetPins[]={\n'+',\n'.join('{"'+p+'","'+s+'"}' for p,s in pins.items())+'\n};\n')
(OUT/'source-contract.json').write_text(json.dumps(dict(files=pins,shaders=shaders,windows=windows,rgb_shaders=rgb_shaders,rgb_source=dict(nativeSha256=hashlib.sha256(original).hexdigest(),sha256=hashlib.sha256(rgb_source).hexdigest(),writerChanges=[dict(before=a.decode('ascii'),after=b.decode('ascii')) for a,b in writer_changes])),indent=2)+'\n')
print('PASS four unmodified native instanced shaders +two output-only RGB variants,%d source/asset pins and%d native code24 windows'%(len(pins),len(windows)))
