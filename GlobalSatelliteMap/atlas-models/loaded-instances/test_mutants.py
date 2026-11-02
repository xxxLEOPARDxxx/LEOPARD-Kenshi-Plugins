"""Compiled functional negative controls. A compile failure is not a killed mutant."""
from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');GAME=Path('D:/SteamLibrary/steamapps/common/Kenshi');FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include;{HERE}';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
policy=(HERE/'InstancePolicy.h').read_text();raster=(HERE/'InstanceRaster.h').read_text();native=(HERE/'NativeInstances.h').read_text();route=(HERE/'InstanceAdmission.h').read_text()
def replace(s,needle,value,n=1):
 assert s.count(needle)==n,(needle,s.count(needle));return s.replace(needle,value)
def header(name,p,r):
 pp=OUT/(name+'-policy.h');pp.write_text(p.replace('"../ModelPolicy.h"','"'+(HERE.parent/'ModelPolicy.h').as_posix()+'"'))
 rp=OUT/(name+'-raster.h');rp.write_text(r.replace('"InstancePolicy.h"','"'+pp.as_posix()+'"').replace('"../GraphicsState.h"','"'+(HERE.parent/'GraphicsState.h').as_posix()+'"'));return rp
def compile_run(name,header_path=None,shader=None,kind=0,test='warp',admission_path=None):
 exe=OUT/(name+'.exe');cmd=[str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT']
 if header_path:cmd+=['/DINSTANCE_RASTER_HEADER="'+header_path.as_posix()+'"']
 if shader:cmd+=['/DINSTANCE_MUTANT_SHADER="'+shader.as_posix()+'"','/DINSTANCE_MUTANT_KIND='+str(kind)]
 source=HERE/(test+'_tests.cpp')
 if admission_path:
  source=OUT/(name+'.cpp');source.write_text((HERE/'admission_tests.cpp').read_text().replace('"InstanceAdmission.h"','"'+admission_path.as_posix()+'"'))
 cmd += [str(source),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib']
 c=subprocess.run(cmd,env=env,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(c.stdout+c.stderr)
 if c.returncode:raise RuntimeError(name+' failed to compile: '+c.stdout+c.stderr)
 r=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(r.stdout+r.stderr)
 assert r.returncode and 'FAIL ' in r.stdout+r.stderr,(name,r.returncode,r.stdout,r.stderr)
 print('Expected RED:',name,(r.stdout+r.stderr).strip()[:150])
mutants=[
 ('lost-metal-border','policy','((s.AddressU!=D3D11_TEXTURE_ADDRESS_BORDER&&s.AddressV!=D3D11_TEXTURE_ADDRESS_BORDER)||s.BorderColor[0]==0)','true'),
 ('double-world','policy','for(unsigned r=0;r<4;++r)for(unsigned c=0;c<4;++c){vc.matrix[c*4+r]=v.viewProjection[r*4+c];vc.matrix[16+c*4+r]=r==c?1.f:0.f;}','AtlasModels::MatrixConstants(world,v,vc.matrix);'),
 ('lost-translation','raster','c.p->UpdateSubresource(instance_.p,0,NULL,d.rawWorld,0,0);','c.p->UpdateSubresource(instance_.p,0,NULL,vc.matrix+16,0,0);'),
 ('per-vertex-instance','raster','e[i].InputSlotClass=D3D11_INPUT_PER_INSTANCE_DATA;','e[i].InputSlotClass=D3D11_INPUT_PER_VERTEX_DATA;'),
 ('abgr-swap','raster','i==4?DXGI_FORMAT_R8G8B8A8_UNORM','i==4?DXGI_FORMAT_B8G8R8A8_UNORM'),
 ('lost-tangent-sign','raster','i==3&&layout==Plain56?DXGI_FORMAT_R32G32B32_FLOAT','i==3?DXGI_FORMAT_R32G32B32_FLOAT'),
 ('lost-threshold','policy','pc.dustThreshold[3]=kind==Alpha48?s.threshold:0;','pc.dustThreshold[3]=0;'),
 ('lost-wetness','policy','pc.waterWetGloss[1]=s.wetness;','pc.waterWetGloss[1]=0;'),
 ('lost-dust','policy','std::memcpy(pc.dustThreshold,s.dustAmount,12);','std::memset(pc.dustThreshold,0,12);'),
 ('swapped-dual-views','raster','views[k]=d.texture[k].p;','views[k]=d.texture[d.kind==Dual43?(k==0?3:k==3?0:k):k].p;'),
 ('native-predicate','raster','c.p->SetPredication(NULL,FALSE);',''),
 ('lost-extra-ps-state','raster','c_->PSSetShaderResources(1,6,old_);',''),
 ('reversed-native-winding','raster','rs.FrontCounterClockwise=TRUE;','rs.FrontCounterClockwise=FALSE;'),
 ('output-read-alias','raster','if(AtlasView::Identity(r.p,tex_[j].p))return E_INVALIDARG;','if(false)return E_INVALIDARG;'),
 ('lost-gpu-bound','policy','caller<=AtlasView::GpuCap-PrivateBytes','true'),
 ('lost-borrowed-buffer-bound','raster','if(bbytes>BufferCap-otherBuffers)return E_OUTOFMEMORY;',''),
 ('lost-borrowed-texture-bound','raster','if(tbytes>TextureCap-otherTextures)return E_OUTOFMEMORY;',''),
 ('alpha-decode-route','policy','return k==Alpha48;','return false;'),
]
for name,which,needle,value in mutants:
 p=replace(policy,needle,value) if which=='policy' else policy;r=replace(raster,needle,value) if which=='raster' else raster;compile_run(name,header(name,p,r))
route_mutants=[('off-queries','if(!enabled)return S_FALSE;',''),('global-floor-limit','if(counts[k]>EntryLimit-physicalEntries)','if(counts[k]>EntryLimit)'),('post-owner','||!ops.End()','')]
for name,needle,value in route_mutants:
 path=OUT/(name+'-admission.h');path.write_text(replace(route,needle,value).replace('"InstancePolicy.h"','"'+(HERE/'InstancePolicy.h').as_posix()+'"'));compile_run(name,test='admission',admission_path=path)
native_mutants=[('native-current-shared','&&SharedCurrent()',''),('native-retained-shared','return sharedAfter_.get()==shared_.get();','return shared_.get()!=NULL;'),('native-visible','&&reinterpret_cast<Bool>(Leaf(0x1dbd70))(e)',''),('native-skeleton','&&!Pointer(static_cast<const BYTE*>(e)+0x188)',''),('native-material-name','&&!std::strcmp(a.name,b.name)',''),('native-parameter-owner','&&a.parameters[0].get()==b.parameters[0].get()',''),('native-sampler-revision','&&!std::memcmp(&a.sampler,&b.sampler,sizeof(a.sampler))',''),('native-world','shou==king_&&',''),('native-source','||std::memcmp(src.data(),InstanceSource,sizeof(InstanceSource))',''),('native-defines','||StringAt(program,0x320)!=defines',''),('native-compile-errors','||flags[2]!=0',''),('native-profile-window','std::memcmp(reinterpret_cast<const BYTE*>(m[p.module])+p.rva,p.bytes,24)','false'),('native-physical-owner','Pointer(reinterpret_cast<const BYTE*>(p.part)+8)==p.entity&&',''),('native-manager-replacement','if(Pointer(now.data+i)!=managerValues_[i])return false;',''),('native-duplicate-members','&&!(lo+1<b.firstMember+b.members.count&&members_[lo+1].entity==entity)',''),('native-off-allocation','if(!enabled)return S_FALSE;',''),('native-kind-mask','if(!kindMask||(kindMask&~3u))return E_INVALIDARG;','')]
for name,needle,value in native_mutants:
 path=OUT/(name+'-native.h');path.write_text(replace(native,needle,value));r=subprocess.run([sys.executable,str(HERE/'test_native_extracted.py'),'--header',str(path),'--label',name],capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(r.stdout+r.stderr)
 assert r.returncode and 'FAIL ' in r.stdout+r.stderr,(name,r.returncode,r.stdout,r.stderr);print('Expected RED:',name,(r.stdout+r.stderr).strip()[:150])
source=(GAME/'data/materials/deferred/objects.hlsl').read_text()
shader_mutants=[('diffuse-alpha-clip','clip(normalTex.a - threshold);','clip(diffuse.a - threshold);',1,1),('lost-translucency','buffer.buf1.a = translucency * 0.5;','buffer.buf1.a = 0;',1,1),('dual-blend-reversed','colour.a);','1.0-colour.a);',0,3)]
for name,needle,value,kind,count in shader_mutants:
 path=OUT/(name+'.hlsl');path.write_text(replace(source,needle,value,count));cso=OUT/(name+'.cso');defines=['DX11','DUST']+(['DUAL_TEXTURE','COLOURING'] if kind==0 else ['TRANSPARENCY']);cmd=[str(FXC),'/nologo','/T','ps_4_0','/E','main_ps','/O3']
 for define in defines:cmd+=['/D',define+'=1']
 r=subprocess.run(cmd+['/I',str(GAME/'data/materials/common'),'/I',str(GAME/'data/materials/deferred'),'/Fo',str(cso),str(path)],capture_output=True,text=True);(OUT/(name+'-shader.log')).write_text(r.stdout+r.stderr);assert not r.returncode,(name,r.stdout,r.stderr)
 array=OUT/(name+'-shader.h');array.write_text('static const unsigned char MutantShader[]={'+','.join(str(x)for x in cso.read_bytes())+'};\n');compile_run(name,shader=array,kind=kind)
print('PASS %d compiled functional instance mutants rejected'%(len(mutants)+len(route_mutants)+len(native_mutants)+len(shader_mutants)))
