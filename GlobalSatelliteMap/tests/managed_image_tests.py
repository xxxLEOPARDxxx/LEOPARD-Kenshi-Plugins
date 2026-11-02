"""Compile extracted production managed-image completion and Upload call order."""
from pathlib import Path
import os,re,subprocess

BASE=Path(__file__).resolve().parents[1]
OUT=BASE/'build/managed-image-tests'
VC=Path('C:/Program Files (x86)/Microsoft Visual Studio 10.0/VC')
SDK=Path('C:/Program Files/Microsoft SDKs/Windows/v7.1')

def function(text,marker):
    if marker not in text:return ''
    start=text.index(marker);begin=text.index('{',start);depth=0
    for end in range(begin,len(text)):
        if text[end]=='{':depth+=1
        elif text[end]=='}':
            depth-=1
            if not depth:return text[start:end+1]
    raise ValueError('unbalanced production function')

def run():
    native=(BASE/'GsmNativeLight.h').read_text()
    helper=function(native,'inline void CompleteManagedImage(')
    # The initial RED run uses the actual old Upload segment, which credits a
    # successful manager load but leaves Loaded false before its first SRV read.
    if not helper:helper='inline void CompleteManagedImage(Ogre::Texture*){}'
    source=(BASE/'GlobalSatelliteMap.cpp').read_text()
    begin=source.index('e.texture=Ogre::TextureManager::getSingleton().loadImage(')
    end=source.index('AtlasView::Com<ID3D11Resource>resource',begin)
    upload=source[begin:end]
    fixture=r'''
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
typedef unsigned long DWORD;
struct ID3D11ShaderResourceView{};
static unsigned checks=0,failures=0,loads=0,credits=0,sets=0,srvs=0,profileCalls=0,typeCalls=0,creatorCalls=0,loadedCalls=0;
static bool profile=true,type=true,inert=false,throwLoad=false,alreadyLoaded=false;
static void Check(bool x,const char*why){++checks;if(!x){++failures;std::printf("FAIL %s\n",why);}}
namespace Ogre {
struct ResourceManager{};
struct Resource {unsigned state;ResourceManager*creator;Resource():state(0),creator(NULL){}};
const unsigned TEX_TYPE_2D=2,PF_A8B8G8R8=28;
struct Texture:Resource {unsigned getFormat(){return 28;}unsigned getNumMipmaps(){return 0;}};
struct TexturePtr {Texture*p;TexturePtr(Texture*t=NULL):p(t){}bool isNull(){return !p;}Texture*get(){return p;}Texture*operator->(){return p;}};
struct Image{};
static Texture imageTexture;static ResourceManager foreign;
struct TextureManager:ResourceManager {
 static TextureManager&getSingleton(){static TextureManager manager;return manager;}
 TexturePtr loadImage(const std::string&,const char*,Image&,unsigned,unsigned){++loads;if(throwLoad)throw std::runtime_error("injected manager failure");++credits;imageTexture.state=alreadyLoaded?2:0;imageTexture.creator=this;return TexturePtr(&imageTexture);}
};
}
static Ogre::ResourceManager*__fastcall Creator(Ogre::Resource*r){++creatorCalls;return r->creator;}
static bool __fastcall Loaded(const Ogre::Resource*r){++loadedCalls;return r->state==2;}
static void __fastcall SetLoaded(Ogre::Resource*r){++sets;if(!inert)r->state=2;}
static const char*GsmPrivateResourceCreatorName="Creator",*GsmPrivateResourceLoadedName="Loaded",*GsmPrivateResourceSetLoadedName="SetLoaded";
static const DWORD GsmPrivateResourceCreatorRva=0x666d0,GsmPrivateResourceLoadedRva=0x66750,GsmPrivateResourceSetLoadedRva=0x66740;
namespace GsmNative {
static bool Profile(){++profileCalls;return profile;}
static bool TextureType(Ogre::Texture*t){++typeCalls;return t&&type;}
static void*Export(const char*name,DWORD rva){
 if(!std::strcmp(name,"Creator")&&rva==0x666d0)return reinterpret_cast<void*>(&Creator);
 if(!std::strcmp(name,"Loaded")&&rva==0x66750)return reinterpret_cast<void*>(&Loaded);
 if(!std::strcmp(name,"SetLoaded")&&rva==0x66740)return reinterpret_cast<void*>(&SetLoaded);
 throw std::runtime_error("unexpected export (including forbidden extra notify)");
}
HELPER
static ID3D11ShaderResourceView*Srv(Ogre::Texture*t){++srvs;static ID3D11ShaderResourceView v;return &v;}
}
static void Reset(){loads=credits=sets=srvs=profileCalls=typeCalls=creatorCalls=loadedCalls=0;profile=type=true;inert=throwLoad=alreadyLoaded=false;Ogre::imageTexture.state=0;Ogre::imageTexture.creator=&Ogre::TextureManager::getSingleton();}
static void ActualUpload(){struct Entry{Ogre::TexturePtr texture;std::string name;}e;e.name="owned";const char*Group="private";Ogre::Image image;UPLOAD}
static bool Refused(){try{GsmNative::CompleteManagedImage(&Ogre::imageTexture);return false;}catch(const std::exception&){return true;}}
int main(){
 Reset();ActualUpload();Check(Ogre::imageTexture.state==2,"successful actual manager Upload must settle Loaded before adoption");Check(loads==1&&credits==1&&sets==1&&srvs==1,"one manager credit,one setter,one SRV; no extra notify");
 Reset();alreadyLoaded=true;ActualUpload();Check(credits==1&&sets==0&&srvs==1,"already loaded manager image does not set or notify again");
 Reset();throwLoad=true;bool caught=false;try{ActualUpload();}catch(const std::exception&){caught=true;}Check(caught&&credits==0&&sets==0&&srvs==0,"failed manager load has no completion/adoption");
 Reset();profile=false;Check(Refused()&&sets==0&&creatorCalls==0&&typeCalls==0,"profile failure precedes native calls");
 Reset();type=false;Check(Refused()&&sets==0&&creatorCalls==0,"wrong texture type precedes resource calls");
 Reset();Ogre::imageTexture.creator=&Ogre::foreign;Check(Refused()&&sets==0&&loadedCalls==0,"foreign creator never settled");
 Reset();inert=true;Check(Refused()&&sets==1&&loadedCalls==2,"inert setter rejected by final Loaded check");
 Reset();Check(!Refused()&&sets==1&&credits==0,"unloaded completion sets once without accounting");Check(!Refused()&&sets==1&&credits==0,"repeated completion does not set again or credit");
 std::printf("Extracted managed image PASS %u checks,%u failures\n",checks,failures);return failures?1:0;
}
'''.replace('HELPER',helper).replace('UPLOAD',upload)
    OUT.mkdir(parents=True,exist_ok=True);cpp=OUT/'fixture.cpp';cpp.write_text(fixture)
    env=os.environ.copy();env['INCLUDE']=f'{VC}/include;{SDK}/Include';env['LIB']=f'{VC}/lib/amd64;{SDK}/Lib/x64'
    exe=OUT/'fixture.exe'
    subprocess.run([str(VC/'bin/amd64/cl.exe'),'/nologo','/EHsc','/MD','/O2','/W3',f'/Fo{OUT}/fixture.obj',f'/Fe{exe}',str(cpp)],check=True,env=env)
    subprocess.run([str(exe)],check=True)
    assert 'NotifyLoaded' not in helper and '_notifyResourceLoaded' not in helper
    print('PASS extracted actual Upload ordering; no resource notify/extra credit')

if __name__=='__main__':run()
