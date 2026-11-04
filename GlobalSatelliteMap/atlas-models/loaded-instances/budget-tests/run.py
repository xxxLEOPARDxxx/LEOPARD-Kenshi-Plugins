from pathlib import Path
import os,subprocess,sys
HERE=Path(__file__).resolve().parent;OUT=HERE/'_generated';OUT.mkdir(exist_ok=True)
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC');SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')
ENV=dict(os.environ,INCLUDE=f'{VC}/include;{SDK}/Include',LIB=f'{VC}/lib/amd64;{SDK}/Lib/x64')
# Compile the actual NativeCapture.Publish body against a bounded call recorder.
source=(HERE.parent/'NativeInstances.h').read_text();start=source.index('HRESULT Publish()');brace=source.index('{',start);depth=1;end=brace+1
while depth:
    depth += (source[end]=='{')-(source[end]=='}');end+=1
body=source[start:end]
publish=r"""
#include "../../InstancePolicy.h"
#include <cstdio>
#include <cstring>
using namespace LoadedInstances;
struct FakeDevice { HRESULT result;unsigned calls;FakeDevice():result(S_OK),calls(0){} HRESULT CreateSamplerState(const D3D11_SAMPLER_DESC*,int*p){++calls;if(SUCCEEDED(result))*p=7;return result;} };
struct Material {D3D11_SAMPLER_DESC sampler;struct Copied{int p;}copied;};struct Draw{int sampler;};struct Batch{unsigned material;};struct Selected{unsigned batch;};
struct FakePacket {unsigned calls;mutable unsigned matches;HRESULT result;bool exact,match;FakePacket():calls(0),matches(0),result(S_OK),exact(false),match(false){}bool MatchesRaw(FakeDevice*,Draw*,const D3D11_SAMPLER_DESC*,unsigned,const float*,unsigned)const{++matches;return match;}HRESULT Adopt(FakeDevice*,Draw*,unsigned,const float*,unsigned,U64,U64,const BorrowLimits&limits,const char**reason){++calls;exact=limits.freshBuffers==32ULL*1024*1024&&limits.freshTextures==128ULL*1024*1024&&limits.totalBuffers==64ULL*1024*1024&&limits.totalTextures==256ULL*1024*1024;if(FAILED(result))*reason="specific Adopt refusal";return result;}};
struct Capture{const char*stage_;FakeDevice device,*device_;Material materials_[2];unsigned materialCount_,selected_,generation_;Draw raw_[2];Batch batches_[2];Selected selected[2];float origin_[3];U64 otherBuffers_,otherTextures_;FakePacket packet_;const FakePacket*previous_;bool unchanged_;
Capture():stage_("untouched"),device_(&device),materialCount_(2),selected_(2),generation_(1),otherBuffers_(1),otherTextures_(2),previous_(NULL),unchanged_(false){std::memset(materials_,0,sizeof(materials_));std::memset(raw_,0,sizeof(raw_));batches_[0].material=0;batches_[1].material=1;selected[0].batch=0;selected[1].batch=1;}
@BODY@
};
int main(){Capture fail;fail.device.result=DXGI_ERROR_DEVICE_REMOVED;if(fail.Publish()!=DXGI_ERROR_DEVICE_REMOVED||fail.device.calls!=1||fail.packet_.calls||std::strcmp(fail.stage_,"instance copied sampler creation"))return 1;Capture cap;cap.packet_.result=E_OUTOFMEMORY;if(cap.Publish()!=E_OUTOFMEMORY||cap.device.calls!=2||cap.packet_.calls!=1||!cap.packet_.exact||std::strcmp(cap.stage_,"specific Adopt refusal"))return 2;Capture ok;if(ok.Publish()!=S_OK||ok.device.calls!=2||ok.packet_.calls!=1||!ok.packet_.exact||ok.raw_[0].sampler!=7||ok.raw_[1].sampler!=7)return 3;FakePacket held;held.match=true;Capture reuse;reuse.previous_=&held;if(reuse.Publish()!=S_FALSE||!reuse.unchanged_||reuse.device.calls||reuse.packet_.calls||held.matches!=1||held.calls)return 4;held.match=false;Capture changed;changed.previous_=&held;if(changed.Publish()!=S_OK||changed.unchanged_||changed.device.calls!=2||changed.packet_.calls!=1||held.matches!=2||held.calls)return 5;held.match=true;Capture over;over.previous_=&held;over.otherTextures_=LoadedLimits().totalTextures+1;over.packet_.result=E_OUTOFMEMORY;if(over.Publish()!=E_OUTOFMEMORY||over.unchanged_||over.packet_.calls!=1||held.matches!=2)return 6;std::puts("PASS 6 actual Publish sampler/adoption/unchanged paths");}
""".replace('@BODY@',body)
(OUT/'publish.cpp').write_text(publish)
for opt in (['Od'] if '--red'in sys.argv else ['Od','O2']):
    exe=OUT/('packet-'+opt+'.exe')
    p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/'+opt,'/MT',str(HERE/'packet.cpp'),'/Fo'+str(OUT/('packet-'+opt+'.obj')),'/Fe'+str(exe),'/link','d3d11.lib','dxgi.lib'],cwd=OUT,env=ENV,capture_output=True,text=True);(OUT/('build-'+opt+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');assert not p.returncode
    p=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/('run-'+opt+('-red'if '--red'in sys.argv else'')+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='')
    if '--red'in sys.argv:assert p.returncode and 'FAIL recorded Hub footprint' in p.stdout
    else:assert not p.returncode

    if '--red' not in sys.argv:
        exe=OUT/('publish-'+opt+'.exe')
        p=subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/'+opt,'/MT',str(OUT/'publish.cpp'),'/Fo'+str(OUT/('publish-'+opt+'.obj')),'/Fe'+str(exe)],cwd=OUT,env=ENV,capture_output=True,text=True);(OUT/('publish-build-'+opt+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');assert not p.returncode
        p=subprocess.run([str(exe)],capture_output=True,text=True);(OUT/('publish-run-'+opt+'.log')).write_text(p.stdout+p.stderr);print(p.stdout+p.stderr,end='');assert not p.returncode
