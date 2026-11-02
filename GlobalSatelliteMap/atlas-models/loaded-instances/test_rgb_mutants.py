"""Compile production RGB-mode controls; only a runtime assertion kills a mutant."""
from pathlib import Path
import os, subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi');FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include;{HERE}';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
policy=(HERE/'InstancePolicy.h').read_text();raster=(HERE/'InstanceRaster.h').read_text();helper=(HERE/'InstanceRawRgbShaders.h').read_text()
def change(s,a,b,n=1):
 assert s.count(a)==n,(a,s.count(a));return s.replace(a,b)
def run(name,p=policy,r=raster,shader=None,kind=0):
 pp=OUT/(name+'-rgb-policy.h');pp.write_text(p.replace('"../ModelPolicy.h"','"'+(HERE.parent/'ModelPolicy.h').as_posix()+'"'))
 rp=OUT/(name+'-rgb-raster.h');rp.write_text(r.replace('"InstancePolicy.h"','"'+pp.as_posix()+'"').replace('"../GraphicsState.h"','"'+(HERE.parent/'GraphicsState.h').as_posix()+'"'))
 hp=OUT/(name+'-rgb-shaders.h');h=helper.replace('"InstanceRaster.h"','"'+rp.as_posix()+'"').replace('"_generated/','"'+OUT.as_posix()+'/')
 if shader:
  h=h.replace('namespace LoadedInstances {','static const unsigned char RgbMutant[]={'+','.join(str(b) for b in shader)+'};\nnamespace LoadedInstances {')
  needle='Bytecode(InstanceFP%dRgb,sizeof(InstanceFP%dRgb))'%((43,43) if kind==0 else (48,48))
  h=change(h,needle,'Bytecode(RgbMutant,sizeof(RgbMutant))')
 hp.write_text(h)
 source=OUT/(name+'-rgb.cpp');source.write_text((HERE/'rgb_warp_tests.cpp').read_text().replace('"warp_tests.cpp"','"'+(HERE/'warp_tests.cpp').as_posix()+'"').replace('"InstanceRawRgbShaders.h"','"'+hp.as_posix()+'"').replace('"_generated/PrewriteRgbOracle.h"','"'+(OUT/'PrewriteRgbOracle.h').as_posix()+'"'))
 exe=OUT/(name+'-rgb.exe');args=[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT','/DINSTANCE_RASTER_HEADER="'+rp.as_posix()+'"',str(source),'/Fo'+str(OUT/(name+'-rgb.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib']
 c=subprocess.run(args,env=env,capture_output=True,text=True);(OUT/(name+'-rgb-build.log')).write_text(c.stdout+c.stderr);assert not c.returncode,(name,'compile failure is not a kill',c.stdout,c.stderr)
 c=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/(name+'-rgb-run.log')).write_text(c.stdout+c.stderr);assert c.returncode and 'FAIL ' in c.stdout+c.stderr,(name,c.returncode,c.stdout,c.stderr)
 print('Expected RED:',name,(c.stdout+c.stderr).strip()[:150],flush=True)
mutants=[
 ('rgb-budget','policy','caller<=AtlasView::GpuCap-RawRgbPrivateBytes','Fits(caller)'),
 ('rgb-render-budget','raster','rawRgb_?FitsRawRgb(caller):Fits(caller)','Fits(caller)'),
 ('rgb-format','raster','i==0&&rgb?DXGI_FORMAT_R16G16B16A16_FLOAT:',''),
 ('rgb-mode','raster','rawRgb_=rgb;','rawRgb_=false;'),
 ('rgb-charge','raster','rawRgb_?RawRgbPrivateBytes:PrivateBytes','PrivateBytes'),
 ('rgb-partial-cleanup','raster','if(FAILED(hr)){Clear();return hr;}','if(FAILED(hr)){return hr;}'),
 ('rgb-predicate','raster','c.p->SetPredication(NULL,FALSE);',''),
 ('rgb-extra-state','raster','c_->PSSetShaderResources(1,6,old_);',''),
 ('rgb-source-alias','raster','if(AtlasView::Identity(r.p,tex_[j].p))return E_INVALIDARG;','if(false)return E_INVALIDARG;'),
 ('rgb-dust','policy','std::memcpy(pc.dustThreshold,s.dustAmount,12);','std::memset(pc.dustThreshold,0,12);'),
 ('rgb-wet','policy','pc.waterWetGloss[1]=s.wetness;','pc.waterWetGloss[1]=0;'),
]
for name,which,a,b in mutants:run(name,p=change(policy,a,b) if which=='policy' else policy,r=change(raster,a,b) if which=='raster' else raster)
src=(OUT/'InstanceRawRgb.hlsl').read_text()
shaders=[
 ('rgb-packed-writer','buffer.buf0.rgb = diffuse.rgb;','writeAlbedo   ( buffer, diffuse.rgb, fragCoord.xy );',0,1),
 ('rgb-metal-overwrite','/* Verified black metalness is not stored over full RGB. */','writeMetalness( buffer, metalness );',0,1),
 ('rgb-diffuse-clip','clip(normalTex.a - threshold);','clip(diffuse.a - threshold);',1,1),
 ('rgb-coverage-alpha','buffer.buf1.a = translucency * 0.5;','buffer.buf1.a = 1;',1,1),
 ('rgb-no-gloss','writeGloss    ( buffer, diffuse.a );','writeGloss    ( buffer, 0 );',1,1),
 ('rgb-reverse-dual','colour.a);','1.0-colour.a);',0,3),
]
for name,a,b,kind,count in shaders:
 path=OUT/(name+'.hlsl');path.write_text(change(src,a,b,count));cso=OUT/(name+'.cso');defines=['DX11','DUST']+(['DUAL_TEXTURE','COLOURING'] if kind==0 else ['TRANSPARENCY'])
 args=[str(FXC),'/nologo','/T','ps_4_0','/E','main_ps','/O3']
 for d in defines:args+=['/D',d+'=1']
 c=subprocess.run(args+['/I',str(GAME/'data/materials/common'),'/I',str(GAME/'data/materials/deferred'),'/Fo',str(cso),str(path)],capture_output=True,text=True);assert not c.returncode,(name,c.stdout,c.stderr)
 run(name,shader=cso.read_bytes(),kind=kind)
print('PASS %d compiled functional RGB mutants rejected'%(len(mutants)+len(shaders)))
