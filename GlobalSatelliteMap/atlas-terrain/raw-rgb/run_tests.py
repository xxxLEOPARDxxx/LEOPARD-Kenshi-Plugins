from pathlib import Path
import os,subprocess,sys,hashlib,json,re
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
def run(name,args):
 p=subprocess.run(args,cwd=OUT,env=env,capture_output=True,text=True);(OUT/(name+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='',flush=True);assert not p.returncode,name
run('fixture',[sys.executable,str(HERE/'prepare_fixture.py')]);run('generate',[sys.executable,str(HERE/'generate.py')])
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
frozen=json.loads((HERE.parent/'compact-material/verification.json').read_text())
for n,h in frozen['production'].items():assert hashlib.sha256((HERE.parent/'compact-material'/n).read_bytes()).hexdigest()==h,n
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
provenance=json.loads((OUT/'fixture-provenance.json').read_text());assert sha(HERE.parent/'compact-material/verification.json')==provenance['compactFreezeSha256'],'dependency changed during test'
for n,h in frozen['artifacts'].items():
 path=HERE.parent/'compact-material/_generated'/n
 if path.exists():assert sha(path)==h,('compact artifact changed',n)
for name in ['control3','control9','shift3','control1']:
 for suffix,i in [('-normal.rgba8',1),('-radial.r32f',2)]:assert sha(OUT/(name+suffix))==frozen['artifacts'][name+'-gbuffer%d.bin'%i]
controls=[]
for name,spacing,center,wet,water,brightness in [('control3',3,-13824,0,-10000,1),('control9',9,-13824,0,-10000,1),('shift3',3,-13632,0,-10000,1),('control1',1,-13824,0,-10000,1),('wet3',3,-13824,.4,-10000,1),('underwater3',3,-13824,0,10000,1),('hdr3',3,-13824,0,-10000,8),('hdr1',1,-13824,.65,-10000,8),('odd3',3,-13821,0,-10000,1)]:
 span=256*spacing;files=[name+s for s in ['-vertex-cb.bin','-pixel-cb.bin','-material.rgba16f','-normal.rgba8','-radial.r32f','-prewrite.rgba32f']];controls.append(dict(name=name,side=256,spacing=spacing,center=[center,center],bounds=[center-span/2,center-span/2,center+span/2,center+span/2],camera=[center,10000,center],farClip=20000,wetness=wet,waterHeightRel=water,nativeBrightnessFixtureMultiplier=brightness,files={n:dict(bytes=(OUT/n).stat().st_size,sha256=sha(OUT/n))for n in files}))
log=(OUT/'warp-run.log').read_text();overlaps=[dict(shiftPixels=int(m[1]),samples=int(m[2]),maxHalf=float(m[3]),maxOriginalPrewrite=float(m[4]),maxNormalByte=int(m[5]))for m in re.finditer(r'OVERLAP shift(\d+)pixels samples(\d+) maxHalf(\S+) maxPrewrite(\S+) maxNormalByte(\d+)',log)]
summary=re.search(r'PASS (\d+) rawterrain WARP checks;(\d+) actual cases;(\d+) covered;maxFloat(\S+) maxHalf(\S+) maxRGB(\S+) gloss(\S+)\.\.(\S+) HDRcomponents(\d+)',log);assert summary and len(overlaps)==2
results=dict(warpChecks=int(summary[1]),cases=int(summary[2]),covered=int(summary[3]),maxFloat=float(summary[4]),maxHalf=float(summary[5]),maxRGB=float(summary[6]),minGloss=float(summary[7]),maxGloss=float(summary[8]),hdrComponents=int(summary[9]),overlaps=overlaps)
(OUT/'controls.json').write_text(json.dumps(dict(scope='native-derived RGB material control only; no Sun/display/game claim',compactFreeze=provenance['compactFreezeSha256'],shaderContractSha256=sha(OUT/'shader-contract.json'),geometrySha256=sha(HERE.parent/'fixed-geometry/FixedTerrainGeometry.h'),formats=['RGBA16F native linear RGB/gloss','RGBA8 native world-normal/alpha0','R32F distance/farClip'],results=results,controls=controls),indent=2)+'\n')
print('PASS isolated raw terrain suite; compact source unchanged; no game/native calls')
