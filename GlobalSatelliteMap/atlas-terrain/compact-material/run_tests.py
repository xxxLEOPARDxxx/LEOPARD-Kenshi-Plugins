from pathlib import Path
import os,subprocess,sys,hashlib,json
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
def run(name,cmd):
 p=subprocess.run(cmd,env=env,cwd=OUT,capture_output=True,text=True);(OUT/(name+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True);assert not p.returncode,name
run('generate',[sys.executable,str(HERE/'generate.py')])
assert hashlib.sha256((HERE.parent/'fixed-geometry/FixedTerrainGeometry.h').read_bytes()).hexdigest()=='8c77945b97bc5925ff78d7845677d4c18d21a920176c1f7279ec6d08349d7698'
path=Path('D:/SteamLibrary/steamapps/common/Kenshi/data/newland/land/fullmap.tif');h=hashlib.sha256()
with path.open('rb')as f:
 for b in iter(lambda:f.read(1024*1024),b''):h.update(b)
assert h.hexdigest()=='6f7d4bc7baaf80dbf8db91e6f367db71f87fe38f053a34108f192584c9637b18'
with path.open('rb')as f:
 raw=bytearray()
 for z in range(7358,7491):f.seek(43686+(z*16385+7358)*2);raw+=f.read(133*2)
assert hashlib.sha256(raw).hexdigest()=='4c2943629387364ee0bfd9f630656affd94fdc7671ab0754b5199e8f3109d3f2';(OUT/'height.u16').write_bytes(raw)
for name in ['policy','warp']:
 exe=OUT/(name+'.exe');run(name+'-build',[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/W3',str(HERE/(name+'_tests.cpp')),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib']);run(name+'-run',[str(exe)])
if '--skip-mutants' not in sys.argv:run('mutants',[sys.executable,str(HERE/'test_mutants.py')])
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
controls=[]
for name,spacing,center in [('control3',3,-13824),('control9',9,-13824),('shift3',3,-13632),('control1',1,-13824)]:
 span=256*spacing;controls.append(dict(name=name,side=256,spacing=spacing,center=[center,center],bounds=[center-span/2,center-span/2,center+span/2,center+span/2],camera=[center,10000,center],farClip=20000,formats=['RGBA8 packed native albedo-metal-gloss','RGBA8 native world normal/alpha0','R32F native radial distance/farClip'],files={n:dict(bytes=(OUT/n).stat().st_size,sha256=sha(OUT/n))for n in [name+'-vertex-cb.bin',name+'-pixel-cb.bin']+[name+'-gbuffer%d.bin'%i for i in range(3)]}))
(OUT/'controls.json').write_text(json.dumps(dict(scope='actual WARP fixed geometry/native materials; no live native or Sun/display parity',sourceContractSha256=sha(OUT/'contract.json'),geometrySha256=sha(HERE.parent/'fixed-geometry/FixedTerrainGeometry.h'),tifSha256=h.hexdigest(),sourceHeightWindowSha256=hashlib.sha256(raw).hexdigest(),controls=controls,comparison=dict(shift64Pixels=dict(samples=36864,maxPackedNormalByte=1,changedComponents=196,maxHeightMeters=0.00243142879),spacing3vs9=dict(samples=7396,index='9j -> 3(3j-255), j85..170',maxMaterialNormalByte=47,changedComponents=28802,maxHeightMeters=0.00238656325,materialEqualityClaim=False))),indent=2)+'\n')
print('PASS controlled compact material suite; no game/native calls')
