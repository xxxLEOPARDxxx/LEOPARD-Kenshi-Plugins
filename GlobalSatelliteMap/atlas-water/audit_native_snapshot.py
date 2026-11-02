"""Read-only installed native/source pin audit; --write-pins updates only our new adapter."""
from pathlib import Path
import argparse,hashlib,json,re,struct,sys
HERE=Path(__file__).resolve().parent
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi')
sys.path.insert(0,str(HERE.parent/'tests'))
from test_pe_contract import PE
MODULES={'RE_Kenshi/Kenshi_x64.exe':'504b362cde850d56afb1cea6f5b7b0ee014d9dd7b47e188599d91c804502cd3e','OgreMain_x64.dll':'7d3888d21398647519f10a89b1f300d8606f761ad159e6021a797b1b9b3df5f6','RenderSystem_Direct3D11_x64.dll':'5f224882ded5b6d3f68f14d6bb4e1762267bbb602b7b9733bd153663d1541bb9'}
SOURCES={
'data/materials/forward/water.material':'2e8cddfb41a8a672377741089e1bc9e7fde2868d676ed0fcea7e220858e2f119',
'data/materials/forward/water.hlsl':'347e9c59ada723e6033cc8a875dd3e724ee8ba5549b4d8bac38ae3cd4265baa5',
'data/materials/common/common.program':'85c10a97ad80b8cc3d74db2c23da673a1f61453bb5e37481dc9d364eb1143183',
'data/materials/common/lightingFunctions.hlsl':'51dd7cec6f898b3a79352067dbfbe586c1d364967154609ed821667d22966f9f',
'data/materials/common/shadowFunctions.hlsl':'6b96b21cbb18a52a093bf07b1aab588873b7b07c2efcbb808d347f7e775ec49e'}
MATCHES=['getConstantDefinition@GpuProgramParameters','getConstantDefinition@GpuSharedParameters','getFloatPointer@GpuProgramParameters','getFloatPointer@GpuSharedParameters','getFloatConstantList@GpuProgramParameters','getFloatConstantList@GpuSharedParameters','findAutoConstantEntry@GpuProgramParameters','_getDerivedOrientation@Node','_getDerivedScale@Node','getRootSceneNode@SceneManager','_getTexturePtr@TextureUnitState','getVertexProgramParameters@Pass','getFragmentProgramParameters@Pass','getTextureMipmapBias@TextureUnitState','getTextureFiltering@TextureUnitState','getTextureAddressingMode@TextureUnitState','getTextureAnisotropy@TextureUnitState','getTextureBorderColour@TextureUnitState','getTextureCompareEnabled@TextureUnitState','getName@TextureUnitState','getDepthBiasConstant@Pass','getDepthBiasSlopeScale@Pass']
def run(write=False):
    modules={}
    for name,expected in MODULES.items():
        p=PE(GAME/name)
        if hashlib.sha256(p.data).hexdigest()!=expected:raise ValueError('native module hash differs: '+name)
        modules[name]=p
    for name,expected in SOURCES.items():
        if hashlib.sha256((GAME/name).read_bytes()).hexdigest()!=expected:raise ValueError('water source hash differs: '+name)
    p=modules['OgreMain_x64.dll'];rows=[]
    for name,(rva,forwarded) in sorted(p.exports().items()):
        if any(m in name for m in MATCHES):
            if forwarded:raise ValueError('unexpected forwarded export')
            rows.append('{0x%x,"%s",{%s}}'%(rva,name,','.join('0x%02x'%b for b in p.data[p.offset(rva):p.offset(rva)+24])))
    if len(rows)!=25:raise ValueError('expected25 water exports, found '+str(len(rows)))
    pin='static const ExportPin ExportPins[]={\n'+',\n'.join(rows)+'\n};\n'
    layout_rvas=[0x37ca30,0x37dbfd,0x37dc2b,0x37dc3c,0x37dcb0,0x37e5a7]
    pin+='static const GsmCodeWindow PassiveWindows[]={\n'+',\n'.join('{0x%x,{%s}}'%(r,','.join('0x%02x'%b for b in p.data[p.offset(r):p.offset(r)+24])) for r in layout_rvas)+'\n};\n'
    pin+='struct SourcePin {const char*path;const char*sha256;};\nstatic const SourcePin SourcePins[]={\n'+',\n'.join('{"%s","%s"}'%(name,sha) for name,sha in SOURCES.items())+'\n};\n'
    header=HERE/'NativeWaterSnapshot.h';text=header.read_text();start='// BEGIN GENERATED NATIVE WATER PINS\n';end='// END GENERATED NATIVE WATER PINS'
    a=text.index(start)+len(start);b=text.index(end,a)
    if write:header.write_text(text[:a]+pin+text[b:])
    elif text[a:b]!=pin:raise ValueError('adapter export/source pins differ from audited native profile')
    engine=modules['RE_Kenshi/Kenshi_x64.exe']
    if engine.data[engine.offset(0x9d671f):engine.offset(0x9d671f)+5]!=bytes.fromhex('ba01000000'):raise ValueError('native water constructor no longer passes root1')
    call=engine.data[engine.offset(0x9d6728):engine.offset(0x9d6728)+6]
    if call[:2]!=b'\xff\x15' or 0x9d672e+struct.unpack_from('<i',call,2)[0]!=0x2246eb0:raise ValueError('native water root call IAT differs')
    at=engine.offset(engine.u32(engine.opt+120));root_import=None
    while engine.u32(at+12):
        thunk=engine.offset(engine.u32(at) or engine.u32(at+16));iat=engine.u32(at+16);i=0
        while True:
            value=struct.unpack_from('<Q',engine.data,thunk+8*i)[0]
            if not value:break
            if iat+8*i==0x2246eb0 and not value>>63:root_import=engine.string(value+2)
            i+=1
        at+=20
    if root_import!='?getRootSceneNode@SceneManager@Ogre@@QEAAPEAVSceneNode@2@W4SceneMemoryMgrTypes@2@@Z':raise ValueError('native water root IAT symbol differs')
    includes=re.findall(r'#include\s+"([^"]+)"',(GAME/'data/materials/forward/water.hlsl').read_text())
    if set(includes)!={'shadowFunctions.hlsl','lightingFunctions.hlsl'}:raise ValueError('native water include closure differs')
    result={'modules':len(MODULES),'sourceFiles':len(SOURCES),'exports':len(rows),'passiveLayoutWindows':len(layout_rvas),'nativeWaterRootIndex':1,'nativeWaterRootIat':root_import,'adapterSha256':hashlib.sha256(header.read_bytes()).hexdigest(),'runtimeObserved':False}
    out=HERE/'_generated/native-snapshot';out.mkdir(parents=True,exist_ok=True);(out/'profile-audit.json').write_text(json.dumps(result,indent=2)+'\n')
    print('Native water snapshot source/profile PASS '+json.dumps(result,sort_keys=True))
if __name__=='__main__':
    parser=argparse.ArgumentParser();parser.add_argument('--write-pins',action='store_true');args=parser.parse_args();run(args.write_pins)
