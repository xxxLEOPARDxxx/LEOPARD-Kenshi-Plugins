from pathlib import Path
import sys,hashlib,json
sys.path.insert(0,'D:/DEV/Kenshi/ours/SkillLevelUpInfo/tests/_tools')
import pefile
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
path=Path('D:/SteamLibrary/steamapps/common/Kenshi/OgreMain_x64.dll');data=path.read_bytes();assert hashlib.sha256(data).hexdigest()=='7d3888d21398647519f10a89b1f300d8606f761ad159e6021a797b1b9b3df5f6';pe=pefile.PE(data=data)
exports={e.name.decode():e.address for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name and not e.forwarder}
prefixes=[('MaterialManager','?getSingleton@MaterialManager'),('Material','?getByName@MaterialManager'),('Load','?load@Resource@'),('Creator','?getCreator@Resource@'),('Handle','?getHandle@Resource@'),('Group','?getGroup@Resource@'),('VertexShadow','?isVertexBufferShadowed@Mesh'),('IndexShadow','?isIndexBufferShadowed@Mesh'),('AutoEdges','?setAutoBuildEdgeLists@Mesh'),('MeshCtor','??0Mesh@Ogre@@QEAA@PEAVResourceManager'),('MeshDtor','??1Mesh@Ogre@@UEAA@XZ'),('SerializerCtor','??0MeshSerializer@Ogre@@QEAA@XZ'),('SerializerDtor','??1MeshSerializer@Ogre@@UEAA@XZ'),('Import','?importMesh@MeshSerializer'),('StreamCtor','??0MemoryDataStream@Ogre@@QEAA@PEAX_K_N2@Z'),('StreamDtor','??1MemoryDataStream@Ogre@@UEAA@XZ'),('SubmeshCount','?getNumSubMeshes@Mesh'),('Submesh','?getSubMesh@Mesh@Ogre@@QEBAPEAVSubMesh@2@G')]

rows=[]
for label,prefix in prefixes:
 match=[(n,r) for n,r in exports.items() if n.startswith(prefix)]
 assert len(match)==1,(label,match)
 name,r=match[0];rows.append(dict(label=label,name=name,rva=r,code=list(pe.get_data(r,24))))
lines=['#pragma once','namespace PrivateTownLod {','enum PrivateFunction { '+','.join('F'+r['label'] for r in rows)+',FunctionCount };','struct PrivatePin {const char*name;DWORD rva;BYTE bytes[24];};','static const PrivatePin PrivatePins[]={']
lines+=['{"%s",0x%x,{%s}},'%(r['name'],r['rva'],','.join(str(v) for v in r['code'])) for r in rows];lines+=['};','static const DWORD UnloadRva=0x270cf0,MeshVtable=0x68e018;','static const BYTE UnloadCode[24]={'+','.join(str(v) for v in pe.get_data(0x270cf0,24))+'};']
lines+=['struct NullCreatorWindow {DWORD rva;BYTE bytes[24];};','static const NullCreatorWindow NullCreatorWindows[]={']
for r in [0x26ffd1,0x270072,0x2709d2,0x270dce,0x1a2d71,0x1a2fd2,0x1c27ea]:lines.append('{0x%x,{%s}},'%(r,','.join(str(v) for v in pe.get_data(r,24))))
lines+=['};','static const NullCreatorWindow PrivateBackendWindows[]={']
backend=Path('D:/SteamLibrary/steamapps/common/Kenshi/RenderSystem_Direct3D11_x64.dll').read_bytes();assert hashlib.sha256(backend).hexdigest()=='5f224882ded5b6d3f68f14d6bb4e1762267bbb602b7b9733bd153663d1541bb9';bp=pefile.PE(data=backend)
for r in [0x1fe30,0x1fe61,0x1ff2f,0x1ff47,0x2024a,0x2026e,0x2028e,0x202a6]:lines.append('{0x%x,{%s}},'%(r,','.join(str(v) for v in bp.get_data(r,24))))
lines+=['};','}']
(OUT/'PrivateProfile.h').write_text('\n'.join(lines)+'\n');(OUT/'native-profile.json').write_text(json.dumps(rows,indent=2)+'\n');print('Private native profile PASS',len(rows),'exact exports; unload270CF0/vtable68E018+98')
