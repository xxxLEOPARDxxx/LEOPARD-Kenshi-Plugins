"""Offline exact local-light module/export/source pins; no process or game calls."""
from pathlib import Path
import argparse,hashlib,json,re,struct,sys
import pefile
HERE=Path(__file__).resolve().parent;GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
EXPECTED={'RE_Kenshi/Kenshi_x64.exe':'504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e','OgreMain_x64.dll':'7d3888d21398647519f10a89b1f300d8606f761ad159e6021a797b1b9b3df5f6'}
FUNCTIONS=[('Visible','?getVisible@MovableObject@'),('Manager','?_getManager@MovableObject@'),('PowerScale','?getPowerScale@Light@'),('Custom','?getCustomParameter@Light@'),('Parent','?getParentSceneNode@MovableObject@'),('Position','?_getDerivedPosition@Node@'),('Direction','?getDerivedDirection@Light@'),('Diffuse','?getDiffuseColour@Light@'),('Specular','?getSpecularColour@Light@'),('Range','?getAttenuationRange@Light@'),('Constant','?getAttenuationConstant@Light@'),('Linear','?getAttenuationLinear@Light@'),('Quadric','?getAttenuationQuadric@Light@'),('Type','?getType@Light@'),('Inner','?getSpotlightInnerAngle@Light@'),('Outer','?getSpotlightOuterAngle@Light@'),('Falloff','?getSpotlightFalloff@Light@'),('Cos','?Cos@Math@Ogre@@SAMAEBVRadian@2@_N@Z')]
FUNCTIONS.append(('RelativeOrigin','?getRelativeOrigin@SceneManager@Ogre@@QEBA?AVVector3@2@XZ'))
WINDOWS=[0x2d96e4,0x2d971f,0x2d9742,0x814cfa,0x812914,0x2d9536,0x2d9588,0x2d5e5a,0x2d5e71,0x2d8c0a,0x2d8c50,0x2d8b9a,0x2d8bbf,0x66c0c0,0x66c0d9,0x66c106,0x66c114,0x66c170,0x9b0090,0x2d8df7,0x2d8e09,0x2d8e21,0x2d8e4a,0x2d9295,0x2d7bea,0x2d7c1e,0x2d7c52,0xed6024]
def run(write=False):
    modules={}
    for name,sha in EXPECTED.items():
        raw=(GAME/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=sha:raise ValueError('native module hash differs: '+name)
        modules[name]=(raw,pefile.PE(data=raw))
    exe,ep=modules['RE_Kenshi/Kenshi_x64.exe'];ogre,op=modules['OgreMain_x64.dll']
    def bytes_at(raw,pe,r,n):return raw[pe.get_offset_from_rva(r):pe.get_offset_from_rva(r)+n]
    exports={s.name.decode():(s.address,s.forwarder) for s in op.DIRECTORY_ENTRY_EXPORT.symbols if s.name}
    rows=[];evidence=[]
    for label,pattern in FUNCTIONS:
        found=[(n,r) for n,(r,f) in exports.items() if n.startswith(pattern) and not f]
        if len(found)!=1:raise ValueError('native function identity ambiguous: '+label)
        name,rva=found[0];raw=bytes_at(ogre,op,rva,24);rows.append('{0x%x,"%s",{%s}}'%(rva,name,','.join('0x%02x'%b for b in raw)));evidence.append({'label':label,'name':name,'rva':hex(rva),'first24':raw.hex()})
    generated='#pragma once\n// Generated only after exact full native module hashes match.\n'
    generated+='enum FunctionIndex {'+','.join('F'+label for label,_ in FUNCTIONS)+',FunctionCount};\n'
    generated+='static const FunctionPin Functions[]={\n'+',\n'.join(rows)+'\n};\n'
    generated+='static const GsmCodeWindow EngineWindows[]={\n'+',\n'.join('{0x%x,{%s}}'%(r,','.join('0x%02x'%b for b in bytes_at(exe,ep,r,24))) for r in WINDOWS)+'\n};\n'
    origin_windows=[0x2c7b04,0x2c7b1f,0x6a9a70]
    generated+='static const GsmCodeWindow OriginWindows[]={\n'+',\n'.join('{0x%x,{%s}}'%(r,','.join('0x%02x'%b for b in bytes_at(ogre,op,r,24))) for r in origin_windows)+'\n};\n'
    generated+='static const DWORD EngineTimestamp=0x%x,EngineSize=0x%x;\n'%(ep.FILE_HEADER.TimeDateStamp,ep.OPTIONAL_HEADER.SizeOfImage)
    generated+='static const DWORD LightVtable=0x67d5f8,RegistryGlobal=0x212cf50,ClockGlobal=0x212f3c0,SinIat=0x2244ba0;\n'
    actual_import=next((m.dll.decode().lower()+'!'+e.name.decode() for m in ep.DIRECTORY_ENTRY_IMPORT for e in m.imports if e.address-ep.OPTIONAL_HEADER.ImageBase==0x2244ba0 and e.name),None)
    if actual_import!='msvcr100.dll!sinf':raise ValueError('native flicker sine import differs')
    pending=['data/materials/deferred/deferred.material','data/materials/deferred/deferred.hlsl'];seen=set();sources=[]
    while pending:
        relative=pending.pop(0)
        if relative in seen:continue
        seen.add(relative);path=GAME/relative;raw=path.read_bytes();sources.append({'path':relative,'sha256':hashlib.sha256(raw).hexdigest()})
        for child_name in re.findall(rb'#include\s+"([^"]+)"',raw):
            child=path.parent/child_name.decode()
            if not child.is_file():child=GAME/'data/materials/common'/child_name.decode()
            pending.append(child.relative_to(GAME).as_posix())
    generated+='struct SourcePin {const char*path;const char*sha256;};\nstatic const SourcePin SourcePins[]={\n'+',\n'.join('{"%s","%s"}'%(s['path'],s['sha256']) for s in sources)+'\n};\n'
    header=HERE/'NativeLocalLightProfile.h'
    if write:header.write_text(generated)
    elif header.read_text()!=generated:raise ValueError('native local-light generated pins differ')
    # Private synthetic native-clock fixture: exact native instructions, only
    # RIP constant/ramp targets relocated within one owned code/data allocation.
    code=bytearray(bytes_at(exe,ep,0x66c0c0,0x71));ramp=bytes_at(exe,ep,0x9b0090,13)
    def relative(at,target,origin,size):struct.pack_into('<i',code,at,target-origin-size)
    relative(0x2a,0xa0,0x26,8);relative(0x6c,0xa4,0x68,8)
    relative(0x47,0x80,0x46,5);relative(0x64,0x80,0x63,5)
    fixture=code+bytes(0x80-len(code))+ramp+bytes(0xa0-0x80-len(ramp))+bytes_at(exe,ep,0x16fc40c,4)+bytes_at(exe,ep,0x167b308,4)
    out=HERE/'_generated';out.mkdir(parents=True,exist_ok=True);(out/'clock-fixture.bin').write_bytes(fixture)
    origin=bytearray(bytes_at(ogre,op,0x2c7af0,0x47));sign=bytes_at(ogre,op,0x6a9a70,4)
    if origin[:7]!=bytes.fromhex('488b81f0050000') or origin[0x2f:0x38]!=bytes.fromhex('0f57c10f57d10f57d9') or sign!=bytes.fromhex('00000080') or origin[-1]!=0xc3:raise ValueError('relative-origin root0/negative/sret proof differs')
    struct.pack_into('<i',origin,11,0x80-15);origin_fixture=origin+bytes(0x80-len(origin))+sign;(out/'origin-fixture.bin').write_bytes(origin_fixture)
    result={'modules':EXPECTED,'functions':evidence,'engineWindows':len(WINDOWS),'originWindows':len(origin_windows),'originGetterAbi':{'rva':'0x2c7af0','sceneRootOffset':'0x5f0','rootPositionOffset':'0x50','rootSoaIndexOffset':'0x38','returns':'negative root0 local XYZ; RCX scene,RDX12byte output,RAX output','fixtureSha256':hashlib.sha256(origin_fixture).hexdigest()},'sources':sources,'clockFixtureSha256':hashlib.sha256(fixture).hexdigest(),'runtimeObserved':False,'profileHeaderSha256':hashlib.sha256(header.read_bytes()).hexdigest()}
    (out/'profile-audit.json').write_text(json.dumps(result,indent=2)+'\n');print('Local-light native profile PASS:2 modules/'+str(len(FUNCTIONS))+'exports/'+str(len(WINDOWS))+'engine+'+str(len(origin_windows))+'origin windows/'+str(len(sources))+'source files; fixtures synthetic, no game module loaded')
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--write',action='store_true');args=parser.parse_args();run(args.write)
