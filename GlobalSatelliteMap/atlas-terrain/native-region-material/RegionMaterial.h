#pragma once
#include "../world-raster/WorldTerrainRaster.h"
#include "../source-receipts/ReceiptPolicy.h"
#include <stdexcept>
namespace NativeRegionMaterial {
typedef SourceReceipts::U64 U64;
struct Snapshot {SourceReceipts::Scope scope;WorldRaster::Material material;BYTE parameterSha[32],distortionSha[32];};
namespace Internal {
struct Helpers {
template<class Record> static float Float(const Record *data,const char *key)
    {
        const auto i=data->fdata.find(key);
        const float value=i==data->fdata.end()?0:i->second;
        if(!_finite(value))throw std::runtime_error("nonfinite private biome parameter");
        return value;
    }
template<class Record> static std::string File(const Record *data,const std::string &key)
    {
        const auto i=data->filesdata.find(key);
        if(i!=data->filesdata.end()&&i->second.size()>=MAX_PATH)throw std::runtime_error("regional source path length refused");
        return i==data->filesdata.end()?std::string():i->second;
    }
static std::string Canonical(const std::string &path)
    {
        char resolved[MAX_PATH]={0};DWORD n=GetFullPathNameA(path.c_str(),MAX_PATH,resolved,NULL);
        if(!n||n>=MAX_PATH)throw std::runtime_error("private DDS path length refused");
        return std::string(resolved,n);
    }
static std::string Hash(const std::vector<BYTE> &bytes)
    {
        HCRYPTPROV provider=0;HCRYPTHASH hash=0;BYTE digest[32];DWORD n=32;
        bool ok=CryptAcquireContextA(&provider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT)!=FALSE;
        ok=ok&&CryptCreateHash(provider,CALG_SHA_256,0,0,&hash)!=FALSE;
        ok=ok&&CryptHashData(hash,&bytes[0],static_cast<DWORD>(bytes.size()),0)!=FALSE;
        ok=ok&&CryptGetHashParam(hash,HP_HASHVAL,digest,&n,0)!=FALSE;
        if(hash)CryptDestroyHash(hash);if(provider)CryptReleaseContext(provider,0);
        if(!ok)throw std::runtime_error("private DDS checksum failed");
        std::string value;const char *hex="0123456789abcdef";
        for(unsigned i=0;i<32;++i){value+=hex[digest[i]>>4];value+=hex[digest[i]&15];}
        return value;
    }
};
template<class Access> struct Builder {
 typedef typename Access::Record Record;Access&access;const SatelliteRegion::Region*selectedRegion;float parameters[900],distortion[60];
 explicit Builder(Access&a):access(a),selectedRegion(NULL){std::memset(parameters,0,sizeof(parameters));std::memset(distortion,0,sizeof(distortion));}
void FillRecord(Record* data,unsigned b,bool nativeFadeRequest)
    {
        static const char* keys[10][4]={{"tiling X 1","tiling Y 1","tiling X 2","tiling Y 2"},{"tiling X 0","tiling Y 0","tiling X grass","tiling Y grass"},
            {"tiling X dirt","tiling Y dirt","tiling X road","tiling Y road"},{"slope min 1","slope min 2","slope min 3","slope min grass"},
            {"slope max 1","slope max 2","slope max 3","slope max grass"},{"slope fade 1","slope fade 2","slope fade 3","slope fade grass"},
            {"overlay mult vertical","overlay mult grass","overlay mult dirt","overlay mult road"},{NULL,NULL,NULL,NULL},
            {"absorbance 0","absorbance 1","absorbance 2","absorbance grass"},{"absorbance dirt","absorbance road",NULL,NULL}};
        for(unsigned row=0;row<10;++row)for(unsigned c=0;c<4;++c)
            if(keys[row][c])parameters[b*60+row*4+c]=Helpers::Float(data,keys[row][c])*((row>=3&&row<=5)?0.01f:1.0f);
        parameters[b*60+40]=Helpers::Float(data,"brightness fix");
        // Native fade row: this exact adapter is extracted by route tests.
        float fade[4];if(!SatelliteGeography::FadeRow(nativeFadeRequest,data->idata,data->fdata,fade))
            throw std::runtime_error("geography native fade fields/derived coefficient refused before allocation");
        std::memcpy(parameters+b*60+28,fade,sizeof(fade));
        // Native fade row end.
        float values[4];if(!SatelliteGeography::Distortion(Helpers::Float(data,"distort wavelength"),Helpers::Float(data,"distort amplitude"),values))
            throw std::runtime_error("geography nonfinite derived distortion refused before allocation");
        std::memcpy(distortion+b*4,values,sizeof(values));
    }
void ParametersRegion(const std::string& root,bool nativeFadeRequest)
    {
        const SatelliteRegion::Region& region=*selectedRegion;
        Record* records[57]={0};bool needed[57]={0};
        std::memset(parameters,0,sizeof(parameters));std::memset(distortion,0,sizeof(distortion));
        for(unsigned c=0;c<16;++c)for(unsigned slot=0;slot<5;++slot)
            if(region.cells[c].slots[slot])needed[region.cells[c].slots[slot]-1]=true;
        for(unsigned source=0;source<57;++source)if(needed[source])
        {
            const char* sid=SatelliteRegion::Data.biomes[source].sid;Record* data=access.GetData(sid);
            if(!data||!access.IsBiome(data)||data->stringID!=sid)throw std::runtime_error("geography regional native SID identity missing");records[source]=data;
        }
        static const char* roles[]={"base","slope","vertical","grass","dirt","road"};
        for(unsigned b=0;b<region.biomeCount;++b)
        {
            const SatelliteRegion::Binding& binding=region.biomes[b];Record* data=records[binding.source];
            if(!data)throw std::runtime_error("geography regional referenced source absent");
            for(unsigned kind=0;kind<2;++kind)for(unsigned role=0;role<6;++role)
            {
                const std::string suffix=kind?" normal":"";
                std::string path=Helpers::File(data,"texture "+std::string(roles[role])+suffix);
                if(path.empty())path=Helpers::File(data,"texture base"+suffix);
                const unsigned source=SatelliteRegion::Data.biomes[binding.source].sources[kind][role];
                if(path.empty()||_stricmp(Helpers::Canonical(root+"/"+path).c_str(),
                    Helpers::Canonical(root+"/"+SatelliteRegion::Data.assets[source].path).c_str()))
                    throw std::runtime_error("geography regional native asset path differs");
            }
            FillRecord(data,b,nativeFadeRequest);
            for(unsigned role=0;role<6;++role){parameters[b*60+44+role]=float(binding.layers[0][role]);parameters[b*60+52+role]=float(binding.layers[1][role]);}
        }
        for(unsigned c=0;c<16;++c)
        {
            void* ids[5];BYTE tree[2048];unsigned count=0;
            if(!access.Cell(region,c,ids,tree,count)||!count||count>sizeof(tree))throw std::runtime_error("geography regional guarded native cell read refused");
            for(unsigned slot=0;slot<5;++slot)
                if(ids[slot]!=(region.cells[c].slots[slot]?records[region.cells[c].slots[slot]-1]:NULL))throw std::runtime_error("geography regional ordered native source slots differ");
            const std::vector<BYTE> mask(tree,tree+count);
            if(count!=region.cells[c].treeBytes||Helpers::Hash(mask)!=region.cells[c].treeSha256)
                throw std::runtime_error("geography regional native mask identity differs");
        }
    }
};
inline bool RootValid(const char*root){return root&&SourceReceipts::Ascii(root,SourceReceipts::PathLimit)&&SourceReceipts::Length(root,SourceReceipts::PathLimit)>=3&&((root[0]>='A'&&root[0]<='Z')||(root[0]>='a'&&root[0]<='z'))&&root[1]==':'&&(root[2]=='/'||root[2]=='\\');}
}
// Access substitutes only engine record/profile/cell reads in host controls.
// Caller supplies its current Main-owned world/load scope and immutable base
// material identity. This adapter cannot independently detect same-ou reloads.
template<class Access> inline bool CaptureWith(Access&access,const SourceReceipts::Scope&scope,unsigned region,const char*gameRoot,U64 generation,bool nativeFade,Snapshot&out){
 std::memset(&out,0,sizeof(out));if(!SourceReceipts::ScopeValid(scope)||GetCurrentThreadId()!=scope.thread||region>=64||!generation||!Internal::RootValid(gameRoot)||!access.Current(scope))return false;
 try {
  WorldMaterial::Planner planner;if(!planner.Init(scope.sourceEpoch))return false;
  Internal::Builder<Access> b(access);b.selectedRegion=&SatelliteRegion::Data.regions[region];b.ParametersRegion(gameRoot,nativeFade);
  Snapshot result={};result.scope=scope;result.material.source.value=scope.sourceEpoch;result.material.source.profile=WorldBank::PinnedBaseSnapshot;result.material.generation=generation;result.material.region=region;
  std::memcpy(result.material.parameters,b.parameters,sizeof(b.parameters));std::memcpy(result.material.distortion,b.distortion,sizeof(b.distortion));
  if(!SourceReceipts::Hash(reinterpret_cast<const BYTE*>(result.material.parameters),sizeof(result.material.parameters),result.parameterSha)||!SourceReceipts::Hash(reinterpret_cast<const BYTE*>(result.material.distortion),sizeof(result.material.distortion),result.distortionSha)||!access.Current(scope))return false;
  out=result;return true;
 }catch(...){return false;}
}
}
