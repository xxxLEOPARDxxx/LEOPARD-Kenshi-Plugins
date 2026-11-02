"""Pinned installed native DistantTown shaders and offline model ABI evidence."""
from pathlib import Path
import hashlib, importlib.util, json, struct, subprocess, sys
sys.dont_write_bytecode=True
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
spec=importlib.util.spec_from_file_location('model_pe',HERE.parent/'tests/test_pe_contract.py')
pe_module=importlib.util.module_from_spec(spec);spec.loader.exec_module(pe_module);PE=pe_module.PE
EXPECTED={'OgreMain_x64.dll':'7d3888d21398647519f10a89b1f300d8606f761ad159e6021a797b1b9b3df5f6',
 'RenderSystem_Direct3D11_x64.dll':'5f224882ded5b6d3f68f14d6bb4e1762267bbb602b7b9733bd153663d1541bb9',
 'RE_Kenshi/Kenshi_x64.exe':'504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e'}
def generate():
 OUT.mkdir(exist_ok=True);pes={}
 for name,sha in EXPECTED.items():
  p=PE(GAME/name);assert hashlib.sha256(p.data).hexdigest()==sha,name;pes[name]=p
 paths=[GAME/'data/materials/deferred'/s for s in ['distant_town.hlsl','distant_town.material','gbuffer.hlsl']]
 # Exact native source closure is checked before shader compilation.
 pinned={'distant_town.hlsl':'ea54dc4c8361e0b300ce34cefe1a26d927ebb5082942642ca769824ddd75c066',
 'distant_town.material':'eaa612b91dad500c9f77dc253019363d2dffea5653a43ed660bd0cc5e4450259',
 'gbuffer.hlsl':'d6a525658f224379269369c9adaa928fc82a72285aed5cffb21d077df4c9c943'}
 manifest=[dict(path=str(p.relative_to(GAME)).replace('\\','/'),sha256=hashlib.sha256(p.read_bytes()).hexdigest()) for p in paths]
 for item in manifest:assert item['sha256']==pinned[Path(item['path']).name]
 shaders=[];header=['#pragma once']
 for symbol,target,entry in [('AtlasModelVertexShader','vs_4_0','main_vs'),('AtlasModelPixelShader','ps_4_0','main_fs')]:
  path=OUT/(symbol+'.cso');subprocess.run([str(FXC),'/nologo','/T',target,'/E',entry,'/O3','/Gec','/D','DX11=1','/I',str(paths[0].parent),'/Fo'+str(path),str(paths[0])],check=True)
  data=path.read_bytes();header+=['static const unsigned char '+symbol+'[]={',',\n'.join(','.join(str(v) for v in data[i:i+24]) for i in range(0,len(data),24)),'};']
  dump=subprocess.check_output([str(FXC),'/dumpbin',str(path)],text=True);(OUT/(symbol+'.reflection.txt')).write_text(dump)
  shaders.append(dict(symbol=symbol,bytes=len(data),sha256=hashlib.sha256(data).hexdigest()))
 (OUT/'ModelShaders.h').write_text('\n'.join(header)+'\n')
 ogre=pes['OgreMain_x64.dll'];exports=ogre.exports()
 (OUT/'cull-exports.txt').write_text('\n'.join(f'{r:x} {n}' for n,(r,f) in exports.items() if any(x in n for x in ['Culling','ColourVertex','ColourType','ColourValue'])))
 (OUT/'vertexdata-exports.txt').write_text('\n'.join(f'{r:x} {n}' for n,(r,f) in exports.items() if 'VertexData@Ogre@@QEAA@' in n))
 sys.path.insert(0,'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools')
 import pefile,capstone
 d3d=pes['RenderSystem_Direct3D11_x64.dll'];pf=pefile.PE(data=d3d.data);base=pf.OPTIONAL_HEADER.ImageBase
 md=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);md.detail=True
 for n,(r,f) in exports.items():
  if n.startswith('??0VertexData@Ogre@@QEAA@PEAVHardwareBufferManagerBase'):
   data=ogre.data[ogre.offset(r):ogre.offset(r)+2048];lines=[]
   for ins in md.disasm(data,r):
    if ins.mnemonic=='int3':break
    lines.append(f'{ins.address:x}: {ins.bytes.hex():24} {ins.mnemonic} {ins.op_str}')
   (OUT/'native-vertexdata-constructor.asm').write_text('\n'.join(lines)+'\n')
 imports={i.address-base:d.dll.decode()+'!'+(i.name.decode() if i.name else str(i.ordinal)) for d in pf.DIRECTORY_ENTRY_IMPORT for i in d.imports}
 table=[]
 for index in range(220):
  r=struct.unpack_from('<Q',d3d.data,d3d.offset(0x7b618)+index*8)[0]-base
  if not 0x1000<=r<0x70000:break
  code=d3d.data[d3d.offset(r):d3d.offset(r)+12];name=''
  for ins in md.disasm(code,r):
   if ins.mnemonic!='jmp':break
   for op in ins.operands:
    if op.type==capstone.x86.X86_OP_MEM and op.mem.base==capstone.x86.X86_REG_RIP:name=imports.get(ins.address+ins.size+op.mem.disp,'')
  table.append(f'{index:3d} {index*8:04x} {r:06x} {name}')
 (OUT/'backend-vtable.txt').write_text('\n'.join(table)+'\n')
 for a,b in [(0x333d0,0x33520),(0x288f0,0x28c80),(0x34360,0x34410),(0x33270,0x33310),(0x386e0,0x38750),(0x2ae10,0x2ae50),(0x324b0,0x324d0),(0x28e40,0x28f50)]:
  code=d3d.data[d3d.offset(a):d3d.offset(a)+b-a]
  (OUT/f'backend-{a:x}.asm').write_text('\n'.join(f'{i.address:x}: {i.bytes.hex():24} {i.mnemonic} {i.op_str}' for i in md.disasm(code,a))+'\n')
 for a,b in [(0x1088e0,0x108910)]:
  code=ogre.data[ogre.offset(a):ogre.offset(a)+b-a]
  (OUT/f'ogre-{a:x}.asm').write_text('\n'.join(f'{i.address:x}: {i.bytes.hex():24} {i.mnemonic} {i.op_str}' for i in md.disasm(code,a))+'\n')
 for prefix in ['?injectMovableObject@SceneManager','?extractMovableObject@SceneManager']:
  name,rva=next((n,r) for n,(r,f) in exports.items() if n.startswith(prefix));at=ogre.offset(rva);data=ogre.data[at:at+1000]
  lines=[]
  for ins in md.disasm(data,rva):
   if ins.mnemonic=='int3':break
   lines.append(f'{ins.address:x}: {ins.bytes.hex():24} {ins.mnemonic} {ins.op_str}')
  (OUT/(prefix.split('@')[0][1:]+'.asm')).write_text('\n'.join(lines)+'\n')
 metadata=dict(sources=manifest,shaders=shaders,modules=EXPECTED)
 pins=[]
 prefixes=['?getMesh@Entity','?getNumSubEntities@Entity','?getSubEntity@Entity@Ogre@@QEAAPEAVSubEntity@2@_K',
 '?getParentSceneNode@MovableObject','?getCreator@SceneNode','?_getManager@MovableObject','?_getFullTransform@Node','?getRelativeOrigin@SceneManager',
 '?getSubMesh@SubEntity','?getMaterial@SubEntity','?_getRenderOperation@SubMesh','?hasVertexAnimation@Mesh','?getBounds@Mesh',
 '?getMovableObjectCollection@SceneManager@Ogre@@IEB','?isLoaded@Resource@','?getNumTechniques@Material',
 '?getTechnique@Material@Ogre@@QEAAPEAVTechnique@2@G','?getNumPasses@Technique','?getPass@Technique@Ogre@@QEAAPEAVPass@2@G',
 '?getVertexProgramName@Pass','?getFragmentProgramName@Pass','?getTextureUnitState@Pass@Ogre@@QEAAPEAVTextureUnitState@2@_K',
 '?getElements@VertexDeclaration','?getBindings@VertexBufferBinding','?getVertexSize@HardwareVertexBuffer','?getType@HardwareIndexBuffer',
 '?getDepthCheckEnabled@Pass','?getDepthWriteEnabled@Pass','?getDepthFunction@Pass','?getCullingMode@Pass','?getPolygonMode@Pass',
 '?getSourceBlendFactor@Pass','?getDestBlendFactor@Pass','?hasSeparateSceneBlending@Pass','?getSceneBlendingOperation@Pass',
 '?hasSeparateSceneBlendingOperations@Pass','?getColourWriteEnabled@Pass','?isAlphaToCoverageEnabled@Pass','?getAlphaRejectFunction@Pass',
 '?getDepthBiasConstant@Pass','?getDepthBiasSlopeScale@Pass','?getIterationDepthBias@Pass',
 '?getName@Resource@','?getNumTextureUnitStates@Pass','?getSource@VertexElement','?getOffset@VertexElement',
 '?getType@VertexElement','?getSemantic@VertexElement','?getIndex@VertexElement',
 '?isNull@AxisAlignedBox','?isInfinite@AxisAlignedBox','?getMinimum@AxisAlignedBox@Ogre@@QEBA','?getMaximum@AxisAlignedBox@Ogre@@QEBA']
 for prefix in prefixes:
  found=[(n,r) for n,(r,f) in exports.items() if n.startswith(prefix) and not f]
  if len(found)!=1:raise ValueError((prefix,found,[n for n in exports if n.startswith('@'.join(prefix.split('@')[:2]))]))
  n,r=found[0];pins.append(dict(name=n,rva=r,code=list(ogre.data[ogre.offset(r):ogre.offset(r)+24])))
 # Primary RTTI vtables only: no guessed offsets/casts to unrelated movable types.
 types={}
 for typename in ['Entity','Mesh','Material','SceneNode']:
  needle=('.?AV'+typename+'@Ogre@@').encode();pos=ogre.data.find(needle);assert pos>=16
  td=next(v+pos-o-16 for v,s,o,n in ogre.sections if o<=pos<o+n);vt=[];raw=ogre.data;imageBase=struct.unpack_from('<Q',raw,ogre.opt+24)[0]
  for v,s,o,n in ogre.sections:
   for delta in range(0,n-24,4):
    a=struct.unpack_from('<6I',raw,o+delta)
    if a[0]!=1 or a[1]!=0 or a[3]!=td or a[5]!=v+delta:continue
    target=struct.pack('<Q',imageBase+v+delta);ref=raw.find(target)
    assert ref>=0;vt.append(next(b+ref-z+8 for b,sz,z,nn in ogre.sections if z<=ref<z+nn))
  assert len(vt)==1,(typename,vt);types[typename]=vt[0]
 windows={r:list(ogre.data[ogre.offset(r):ogre.offset(r)+24]) for r in [0x2cc43e,0x2cc4f5,0x38eae0,0x38eaf8,0x38eb0e]}
 d3dWindows={r:list(d3d.data[d3d.offset(r):d3d.offset(r)+24]) for r in [0x288e0,0x243a0,0x286f4,0x2873c,0x241cf,0x2420a,0x1fe20,0x1fee2,0x33270,0x332bc,0x332d7,0x332ef,0x38723,0x343a8,0x324b0,0x28e50,0x28e6b,0x28e9c,0x28eb4]}
 vertexMapping={}
 for number in range(12):
  target=struct.unpack_from('<I',d3d.data,d3d.offset(0x28e9c)+number*4)[0];at=d3d.offset(target);assert d3d.data[at]==0xb8
  vertexMapping[number]=struct.unpack_from('<I',d3d.data,at+1)[0]
 assert {i:vertexMapping[i] for i in [0,1,2,3,4,10,11]}=={0:41,1:16,2:6,3:2,4:28,10:28,11:28}
 metadata['nativeVertexFormats']=vertexMapping
 # Never call the ctor; its entry may be KEP's visibility-init detour.
 # Preserve the unchanged native state/entity-write tail evidence instead.
 exe=pes['RE_Kenshi/Kenshi_x64.exe'];exeWindows={r:list(exe.data[exe.offset(r):exe.offset(r)+24]) for r in [0xd5fd5,0x92ced0,0x92cf0e]}
 data=exe.data[exe.offset(0x92ced0):exe.offset(0x92ced0)+0x200]
 (OUT/'native-showDistantTown.asm').write_text('\n'.join(f'{i.address:x}: {i.bytes.hex():24} {i.mnemonic} {i.op_str}' for i in md.disasm(data,0x92ced0))+'\n')
 metadata.update(nativeExports=pins,types=types,ogreWindows=windows,d3dWindows=d3dWindows)
 profile=['#pragma once','// Generated from exact installed modules; all code windows are24 bytes.','struct ModelExportPin {DWORD rva;const char*name;BYTE bytes[24];};','static const ModelExportPin ModelExportPins[]={']
 for pin in pins:profile.append('{0x%x,"%s",{%s}},'%(pin['rva'],pin['name'],','.join(str(x) for x in pin['code'])))
 profile+=['};']
 for key,peObj,w in [('Ogre',ogre,windows),('D3d',d3d,d3dWindows),('Exe',exe,exeWindows)]:
  nt=peObj.u32(0x3c);profile+=['static const DWORD Model%sTimestamp=0x%x,Model%sSize=0x%x;'%(key,peObj.u32(nt+8),key,peObj.u32(peObj.opt+56)), 'static const GsmCodeWindow Model%sWindows[]={'%key]
  profile += ['{0x%x,{%s}},'%(r,','.join(str(x) for x in code)) for r,code in w.items()];profile+=['};']
 for name,r in types.items():profile.append('static const DWORD Model%sVtable=0x%x;'%(name,r))
 profile+=['struct ModelSourcePin {const char*path;const char*sha256;};','static const ModelSourcePin ModelSourcePins[]={']+['{"%s","%s"},'%(s['path'],s['sha256']) for s in manifest]+['};']
 (OUT/'ModelProfile.h').write_text('\n'.join(profile)+'\n')
 (OUT/'models.json').write_text(json.dumps(metadata,indent=2)+'\n')
 return metadata
if __name__=='__main__':
 result=generate();print(json.dumps(dict(shaders=result['shaders'],sourceCount=len(result['sources']),nativeExports=len(result['nativeExports']),types=result['types']),indent=2))
