"""Actual compiled source/owner controls; a compilation failure is not a kill."""
from pathlib import Path
import os,subprocess
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1');FXC=Path('C:/Program Files (x86)/Windows Kits/10/bin/10.0.26100.0/x64/fxc.exe')
env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
header=(HERE/'RawMaterial.h').read_text();test=(HERE/'warp_tests.cpp').read_text()
def change(s,a,b,n=1):assert s.count(a)==n,(a,s.count(a));return s.replace(a,b,n)
def buildrun(name,h=header,body=test,shader=None,green=False):
 hp=OUT/(name+'.h');hp.write_text(h.replace('"../../atlas-view/AtlasRelight.h"','"'+(HERE.parents[1]/'atlas-view/AtlasRelight.h').as_posix()+'"'))
 body=body.replace('"RawMaterial.h"','"'+hp.as_posix()+'"').replace('"../fixed-geometry/FixedTerrainGeometry.h"','"'+(HERE.parent/'fixed-geometry/FixedTerrainGeometry.h').as_posix()+'"').replace('"../../atlas-view/ContextFixture.h"','"'+(HERE.parents[1]/'atlas-view/ContextFixture.h').as_posix()+'"')
 if shader:
  sp=OUT/(name+'-shader.h');pre=(OUT/'Prewrite.cso').read_bytes();sp.write_text('#pragma once\nnamespace TerrainRaw {static const unsigned char RgbShader[]={'+','.join(str(v)for v in shader)+'};static const unsigned char PrewriteShader[]={'+','.join(str(v)for v in pre)+'};}\n');body=body.replace('"_generated/RawShaders.h"','"'+sp.as_posix()+'"')
 body=body.replace('"_generated/','"'+OUT.as_posix()+'/');cpp=OUT/(name+'.cpp');cpp.write_text(body);exe=OUT/(name+'.exe')
 p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/O2','/MT',str(cpp),'/Fo'+str(OUT/(name+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib','advapi32.lib'],env=env,cwd=OUT,capture_output=True,text=True);(OUT/(name+'-build.log')).write_text(p.stdout+p.stderr);assert not p.returncode,(name,'compile failure is not a kill',p.stdout,p.stderr)
 p=subprocess.run([str(exe)],cwd=OUT,capture_output=True,text=True);(OUT/(name+'-run.log')).write_text(p.stdout+p.stderr);assert (p.returncode==0 if green else p.returncode!=0 and 'FAIL ' in p.stdout+p.stderr),(name,p.returncode,p.stdout,p.stderr);print(('PASS injected failure: 'if green else 'Expected RED: ')+name+' '+(p.stdout+p.stderr).strip()[-180:],flush=True)
owners=[('lost-raw-budget','other<=Limit-PrivateBytes&&other<=AtlasView::GpuCap-PrivateBytes','true'),('lost-raw-charge','device_.p?PrivateBytes:0','device_.p?PackedBytes:0'),('packed-material-target','i==0?DXGI_FORMAT_R16G16B16A16_FLOAT','i==0?DXGI_FORMAT_R8G8B8A8_UNORM'),('retained-clear-view','view_[i].Reset();','')]
for n,a,b in owners:buildrun(n,change(header,a,b))
rgb=(OUT/'Rgb.hlsl').read_text()
variants=[('packed-writer','buffer.buf0.rgb = biome.albedo.rgb;','writeAlbedo   ( buffer, biome.albedo.rgb, fragCoord.xy );'),('zero-metal-clobber','/* Native zero metalness is not written over full RGB. */','writeMetalness( buffer, 0.0f );'),('lost-gloss','writeGloss    ( buffer, biome.albedo.a );','writeGloss    ( buffer, 0.0f );'),('invented-normal-alpha','writeNormal   ( buffer, worldNormal );','writeNormal   ( buffer, worldNormal );buffer.buf1.a=1;'),('wrong-radial','writeDepth    ( buffer, distance / farClip );','writeDepth    ( buffer, distance / farClip * 0.99 );'),('lost-native-wet','makeWet(biome.albedo, wetness, absorb, waterHeightRel - worldPos.y);','/* omitted native wetness */'),('clipped-HDR','buffer.buf0.rgb = biome.albedo.rgb;','buffer.buf0.rgb = saturate(biome.albedo.rgb);')]
for name,a,b in variants:
 assert a in rgb;source=rgb.replace(a,b,1);path=OUT/(name+'.hlsl');path.write_text(source);cso=OUT/(name+'.cso');p=subprocess.run([str(FXC),'/nologo','/T','ps_4_0','/E','main_fs','/O3','/Gec','/D','DX11=1','/Fo',str(cso),str(path)],capture_output=True,text=True);assert not p.returncode,(name,p.stdout,p.stderr);buildrun(name,shader=cso.read_bytes())
fault=r'''#include "RawMaterial.h"
#include "../../atlas-view/ContextFixture.h"
#include <cstdio>
int main(){ID3D11Device*d=NULL;ID3D11DeviceContext*c=NULL;if(FAILED(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d,NULL,&c)))return 1;ULONG before=d->AddRef();d->Release();{TerrainRaw::Targets p;AtlasViewTest::ContextFixture a(c);if(SUCCEEDED(p.Init(d,0))||p.OwnedGpuBytes()||p.Texture(0)||p.View(0)||p.Target(0)||p.Depth()){std::puts("FAIL partial raw init published");return 2;}AtlasViewTest::ContextFixture b(c);if(!a.Equal(b)){std::puts("FAIL failed init altered state");return 3;}ULONG inside=d->AddRef();d->Release();if(inside!=before){std::puts("FAIL failed init retained COM resources");return 4;}}ULONG after=d->AddRef();d->Release();if(after!=before)return 5;c->Release();d->Release();std::puts("PASS failed raw target creation rolls back all refs/state/publication");}
'''
for i in range(4):buildrun('texture-failure'+str(i),change(header,'hr=d->CreateTexture2D','if(i==%d)t.Format=DXGI_FORMAT_UNKNOWN;hr=d->CreateTexture2D'%i),fault,green=True)
for name,a in [('srv','d->CreateShaderResourceView(tex_[i].p,NULL,&view_[i].p)'),('rtv','d->CreateRenderTargetView(tex_[i].p,NULL,&rt_[i].p)'),('dsv','d->CreateDepthStencilView(tex_[3].p,NULL,&depth_.p)'),('post-create-health','d->GetDeviceRemovedReason()')]:buildrun(name+'-failure',change(header,a,'E_FAIL'),fault,green=True)
bad=change(header,'if(FAILED(hr)){Clear();return hr;}','if(FAILED(hr)){return hr;}');bad=change(bad,'hr=d->CreateTexture2D','if(i==1)t.Format=DXGI_FORMAT_UNKNOWN;hr=d->CreateTexture2D');buildrun('lost-partial-cleanup',bad,fault)
print('PASS12 compiled functional RGBterrain mutants and8 allocation/view/health failure controls')
