#pragma once
// Resolve registered resources only. Hash the resulting base files in the
// existing worker; no file reads and no model loading on the render thread.
#include <OgreResourceGroupManager.h>
#include <OgreArchive.h>
#include "atlas-models/loaded-instances/_generated/InstanceAssetPins.h"
namespace GsmInstanceSources {
inline std::string Basename(const std::string&path){size_t at=path.find_last_of("/\\");return at==std::string::npos?path:path.substr(at+1);}
inline bool SamePath(const std::string&a,const std::string&b){
    if(a.size()!=b.size())return false;
    for(size_t i=0;i<a.size();++i){unsigned x=static_cast<unsigned char>(a[i]),y=static_cast<unsigned char>(b[i]);if(x=='/')x='\\';if(y=='/')y='\\';if(x>='A'&&x<='Z')x+='a'-'A';if(y>='A'&&y<='Z')y+='a'-'A';if(x!=y)return false;}return true;
}
inline bool ActiveBaseSources(const std::string&game,std::string&error){
    try{
        Ogre::ResourceGroupManager&groups=Ogre::ResourceGroupManager::getSingleton();
        for(unsigned i=0;i<sizeof(InstanceAssetPins)/sizeof(InstanceAssetPins[0]);++i){
            const InstanceAssetPin&pin=InstanceAssetPins[i];const std::string name=Basename(pin.path);
            const Ogre::String group=groups.findGroupContainingResource(name);
            Ogre::FileInfoListPtr files=groups.findResourceFileInfo(group,name,false);
            // An ambiguous archive is not evidence that our pinned image is
            // the active native resource. Retain the existing LOD fallback.
            if(files.isNull()||files->size()!=1||!(*files)[0].archive||(*files)[0].archive->getType()!="FileSystem"){
                error="loaded town resource resolution is absent or ambiguous: "+name;return false;
            }
            const Ogre::FileInfo&f=(*files)[0];std::string path=f.archive->getName();
            if(path.empty()){error="loaded town resource archive has no path: "+name;return false;}
            if(path[path.size()-1]!='/'&&path[path.size()-1]!='\\')path+='\\';path+=f.filename;
            // Ogre may register a relative base archive. Resolve it against
            // the same game directory used by the file-validation worker.
            if(path.size()<3||path[1]!=':')path=game+path;
            char absolute[MAX_PATH]={};DWORD n=GetFullPathNameA(path.c_str(),MAX_PATH,absolute,NULL);
            if(!n||n>=MAX_PATH||!SamePath(std::string(absolute,n),game+pin.path)){
                error="loaded town resource is replaced by an unsupported source: "+name;return false;
            }
        }
        error.clear();return true;
    }catch(const std::exception&e){error=e.what();return false;}
    catch(...){error="loaded town resource resolution failed";return false;}
}
}
