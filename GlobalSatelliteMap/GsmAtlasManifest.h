#pragma once
#include "atlas-view/AtlasReader.h"
#include "SatellitePolicy.h"
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <set>
namespace GsmAtlas {
struct Manifest {
    unsigned char header[16][32];
    unsigned char captureMetadata[32];
    double minX,minZ,maxX,maxZ;
    Manifest():minX(0),minZ(0),maxX(0),maxZ(0){std::memset(header,0,sizeof(header));std::memset(captureMetadata,0,32);}
};
inline bool Hex(const std::string&s,unsigned char*out){if(!GSM::Hash(s))return false;for(unsigned i=0;i<32;++i){unsigned a=s[i*2],b=s[i*2+1];out[i]=static_cast<unsigned char>(((a<='9'?a-'0':a-'a'+10)<<4)|(b<='9'?b-'0':b-'a'+10));}return true;}
inline bool Json(const std::string&s,boost::property_tree::ptree&tree){
    if(s.empty()||s.size()>65536)return false;unsigned depth=0;bool quote=false,escape=false;
    for(size_t i=0;i<s.size();++i){char c=s[i];if(!c)return false;if(quote){if(escape)escape=false;else if(c=='\\')escape=true;else if(c=='"')quote=false;}else if(c=='"')quote=true;else if(c=='{'||c=='['){if(++depth>32)return false;}else if(c=='}'||c==']'){if(!depth)return false;--depth;}}
    if(quote||depth)return false;
    try{std::istringstream in(s);boost::property_tree::read_json(in,tree);return true;}catch(...){return false;}
}
inline bool Unique(const boost::property_tree::ptree&node){std::set<std::string>keys;for(boost::property_tree::ptree::const_iterator i=node.begin();i!=node.end();++i){if((!i->first.empty()&&!keys.insert(i->first).second)||!Unique(i->second))return false;}return true;}
inline bool Ready(const std::string&text,Manifest&out){
    boost::property_tree::ptree t;if(!Json(text,t)||!Unique(t))return false;
    try{
        if(t.get<std::string>("format")!="GST2"||t.get<unsigned>("version")!=2||t.get<std::string>("complete")!="true"||t.get<unsigned>("grid")!=4||t.get<unsigned>("baseSide")!=8192||t.get<unsigned>("levels")!=6||t.get<std::string>("rowOrder")!="minZ-first"||t.get<std::string>("columnOrder")!="minX-first"||t.get<std::string>("mipFilter")!="global-aligned-2x2-box-coverage-weighted-v1")return false;
        const boost::property_tree::ptree&planes=t.get_child("planes");
        const char*names[3]={"materialRGBA16FloatLE-linearRGB-glossA","normalCoverageRGBA8-worldXYZ-coverageA","worldHeightR32floatLE"};unsigned p=0;
        for(boost::property_tree::ptree::const_iterator i=planes.begin();i!=planes.end();++i){if(p>=3||!i->first.empty()||i->second.get_value<std::string>()!=names[p++])return false;}if(p!=3)return false;
        bool seen[16]={false};unsigned count=0;Manifest m;if(!Hex(t.get<std::string>("captureMetadataSha256"),m.captureMetadata))return false;
        const boost::property_tree::ptree&tiles=t.get_child("tiles");
        for(boost::property_tree::ptree::const_iterator i=tiles.begin();i!=tiles.end();++i){const boost::property_tree::ptree&v=i->second;unsigned x=v.get<unsigned>("x"),z=v.get<unsigned>("z");if(!i->first.empty()||x>=4||z>=4||seen[z*4+x]||v.get<AtlasView::U64>("bytes")!=AtlasView::FileBytes())return false;std::ostringstream n;n<<"tile_"<<x<<'_'<<z<<".gst";if(v.get<std::string>("file")!=n.str()||!Hex(v.get<std::string>("headerSha256"),m.header[z*4+x]))return false;seen[z*4+x]=true;++count;}
        if(count!=16)return false;out=m;return true;
    }catch(...){return false;}
}
inline bool Small(const std::string&path,std::string&out,bool allowEmpty=false){
    int wn=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path.data(),static_cast<int>(path.size()),NULL,0);if(wn<=0||wn>=MAX_PATH)return false;std::wstring wide(wn,0);if(MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,path.data(),static_cast<int>(path.size()),&wide[0],wn)!=wn)return false;HANDLE f=CreateFileW(wide.c_str(),GENERIC_READ,FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);if(f==INVALID_HANDLE_VALUE)return false;LARGE_INTEGER size;bool ok=GetFileSizeEx(f,&size)&&size.QuadPart>=0&&(allowEmpty||size.QuadPart>0)&&size.QuadPart<=65536;
    if(ok){out.resize(static_cast<size_t>(size.QuadPart));if(!out.empty()){DWORD done=0;ok=ReadFile(f,&out[0],static_cast<DWORD>(out.size()),&done,NULL)&&done==out.size();}}if(!CloseHandle(f))ok=false;return ok;
}
inline int Level(unsigned count,unsigned display){unsigned l=2;while(l<5&&(AtlasView::Side(l)>display*2||!AtlasView::Fits(count*AtlasView::CellBytes(l),AtlasView::Side(l))))++l;return int(l);}
}
