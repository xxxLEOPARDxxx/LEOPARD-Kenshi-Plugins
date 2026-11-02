"""Read-only scalar caller native pins; emits only this separate helper profile."""
from pathlib import Path
import argparse,hashlib,json,re
import pefile
HERE=Path(__file__).resolve().parent;GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
EXPECTED={'RE_Kenshi/Kenshi_x64.exe':'504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e','OgreMain_x64.dll':'7d3888d21398647519f10a89b1f300d8606f761ad159e6021a797b1b9b3df5f6','KenshiLib.dll':'5b6240622d06801dc39896e385efb834b8e1fe2fcdf86067d071ae7a04f6c83b'}
def run(write=False):
    modules={}
    for name,expected in EXPECTED.items():
        raw=(GAME/name).read_bytes()
        if hashlib.sha256(raw).hexdigest()!=expected:raise ValueError('origin caller module hash differs: '+name)
        modules[name]=(raw,pefile.PE(data=raw))
    raw,pe=modules['OgreMain_x64.dll'];exports={s.name.decode():s.address for s in pe.DIRECTORY_ENTRY_EXPORT.symbols if s.name and not s.forwarder}
    pins=[];generated='#pragma once\n// Exact pinned scalar-caller exports and passive town/proxy field proof.\n'
    for label,name,rva in [('Camera','?getDerivedPosition@Camera@Ogre@@QEBAAEBVVector3@2@XZ',0x82c40),('Local','?getPosition@Node@Ogre@@QEBA?AVVector3@2@XZ',0x1de1c0)]:
        if exports.get(name)!=rva:raise ValueError('origin caller export differs: '+name)
        at=pe.get_offset_from_rva(rva);data=raw[at:at+24];generated+='static const NativeLocalLights::FunctionPin '+label+'Pin={0x%x,"%s",{%s}};\n'%(rva,name,','.join('0x%02x'%v for v in data));pins.append({'label':label,'name':name,'rva':hex(rva),'first24':data.hex()})
    ex,ep=modules['RE_Kenshi/Kenshi_x64.exe'];windows=[0x926408,0x926425,0xd5fd0,0xd83f1,0xd8414,0xd8792]
    generated+='static const GsmCodeWindow TownWindows[]={\n'+',\n'.join('{0x%x,{%s}}'%(r,','.join('0x%02x'%v for v in ex[ep.get_offset_from_rva(r):ep.get_offset_from_rva(r)+24])) for r in windows)+'\n};\n'
    p=HERE/'OriginProofProfile.h'
    if write:p.write_text(generated)
    elif p.read_text()!=generated:raise ValueError('origin proof profile differs')
    out=HERE/'_generated/origin-proof';out.mkdir(parents=True,exist_ok=True);(out/'audit.json').write_text(json.dumps({'modules':EXPECTED,'getters':pins,'townWindows':[hex(v) for v in windows],'townGetterInvoked':False,'gameCalls':0,'profileSha256':hashlib.sha256(p.read_bytes()).hexdigest()},indent=2)+'\n');print('Origin caller audit PASS:3module hashes/2extra getters/6town-proxy windows; no runtime call')
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--write',action='store_true');args=parser.parse_args();run(args.write)
