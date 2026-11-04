#pragma once
#include "atlas-models/loaded-instances/NativeInstances.h"
#include "atlas-models/ModelRegion.h"
#include <memory>
#include <sstream>
namespace GsmLoadedTown {
// One already loaded town. A successful capture owns only copied values and
// COM references. Never load zones or keep a native Town/Building pointer.
class State {
    std::auto_ptr<LoadedInstances::Snapshot> snapshot_;
    AtlasModelRegion::Bounds bounds_;
    unsigned town_,revision_;LONG generation_;const void*scene_;
    State(const State&);State&operator=(const State&);
    void Dirty(){if(++revision_==0)revision_=1;}
    static bool Bounds(const LoadedInstances::Packet&p,AtlasModelRegion::Bounds&out){
        if(!p.Count())return false;
        for(unsigned i=0;i<p.Count();++i){AtlasModelRegion::Bounds b;
            if(!AtlasModelRegion::WorldBounds(p.RawSceneWorld(i),p.SourceSceneOrigin(),p.LocalBounds(i),b))return false;
            for(unsigned k=0;k<3;++k){if(!i||b.lo[k]<out.lo[k])out.lo[k]=b.lo[k];if(!i||b.hi[k]>out.hi[k])out.hi[k]=b.hi[k];}
        }return true;
    }
public:
    State():town_(0),revision_(1),generation_(0),scene_(NULL){}
    void Clear(){bool had=snapshot_.get()!=NULL;snapshot_.reset();bounds_=AtlasModelRegion::Bounds();town_=0;generation_=0;scene_=NULL;if(had)Dirty();}
    bool Valid(unsigned generation,const void*scene,const float*origin)const{
        return snapshot_.get()&&generation_==LONG(generation)&&scene_==scene&&snapshot_->Geometry().Generation()==generation&&origin&&!std::memcmp(snapshot_->Geometry().SourceSceneOrigin(),origin,12);
    }
    unsigned Revision()const{return revision_;}unsigned TownId()const{return town_;}
    const LoadedInstances::Packet*Geometry()const{return snapshot_.get()?&snapshot_->Geometry():NULL;}
    const LoadedInstances::Stats*Stats()const{return snapshot_.get()?&snapshot_->Values():NULL;}
    const AtlasModelRegion::Bounds&WorldBounds()const{return bounds_;}
    AtlasView::U64 BorrowedBuffers()const{return snapshot_.get()?snapshot_->Geometry().BorrowedBuffers():0;}
    AtlasView::U64 BorrowedTextures()const{return snapshot_.get()?snapshot_->Geometry().BorrowedTextures():0;}
    HRESULT Refresh(const GsmTownValues::Result&values,const float*rawCamera,const float*origin,
            Ogre::SceneManager*scene,const void*king,LONG generation,volatile LONG*live,
            bool verified,ID3D11Device*device,AtlasView::U64 otherBuffers,
            AtlasView::U64 otherTextures,std::string&error){
        error.clear();
        if(!verified||!scene||!king||!device||!live||generation<=0||values.generation!=generation||
            !rawCamera||!origin||!AtlasModels::Finite(rawCamera,3)||!AtlasModels::Finite(origin,3)||
            values.count>GsmTownDiagnostic::Limit||!GsmTownValues::Generation(live,generation)){
            Clear();return E_ABORT;
        }
        if(!Valid(unsigned(generation),scene,origin))Clear();
        // A fixed shortlist avoids a native capture for every world town.
        unsigned chosen[4];double distances[4];for(unsigned k=0;k<4;++k){chosen[k]=GsmTownDiagnostic::Limit;distances[k]=8192.0*8192.0;}
        for(unsigned i=0;i<values.count;++i){const GsmTownValues::TownValue&v=values.towns[i];
            if(!AtlasModels::Finite(v.storedXYZ,3))continue;
            double x=double(v.storedXYZ[0])-double(rawCamera[0])-origin[0],z=double(v.storedXYZ[2])-double(rawCamera[2])-origin[2],d=x*x+z*z;
            if(!_finite(d))continue;for(unsigned k=0;k<4;++k)if(d<distances[k]){for(unsigned j=3;j>k;--j){chosen[j]=chosen[j-1];distances[j]=distances[j-1];}chosen[k]=i;distances[k]=d;break;}
        }
        try{
            for(unsigned k=0;k<4&&chosen[k]!=GsmTownDiagnostic::Limit;++k){
                const unsigned id=values.towns[chosen[k]].id;
                Town*town=GsmModels::Resolve(king,GetModuleHandleA(NULL),id,generation,live);if(!town)continue;
                std::auto_ptr<LoadedInstances::Snapshot>fresh(new LoadedInstances::Snapshot);
                const LoadedInstances::Packet*previous=town_==id&&Valid(unsigned(generation),scene,origin)?Geometry():NULL;
                // Include the old packet while both generations coexist.
                HRESULT hr=fresh->Read(true,town,scene,generation,live,true,device,
                    otherBuffers+BorrowedBuffers(),otherTextures+BorrowedTextures(),3,previous);
                if(!GsmTownValues::Generation(live,generation)){Clear();return E_ABORT;}
                // A successful fallback must not erase the first nearer town's
                // refusal. Keep one bounded reason for the caller's deduplicated
                // error log; expected empty E_ABORT remains a quiet retry.
                if(FAILED(hr)){
                    const char*why=fresh->LastError();
                    if(error.empty()&&(hr!=E_ABORT||(why&&*why))){std::ostringstream row;row<<"town="<<id<<" hr="<<std::hex<<unsigned(hr)<<" "<<(why?why:"");error=row.str();}
                    continue;
                }
                if(hr==S_FALSE&&fresh->Unchanged()){
                    if(previous&&town_==id&&Valid(unsigned(generation),scene,origin))return S_FALSE;
                    Clear();return E_ABORT;
                }
                if(hr!=S_OK||!fresh->Geometry().Count())continue;
                AtlasModelRegion::Bounds b;if(!Bounds(fresh->Geometry(),b)||std::memcmp(fresh->Geometry().SourceSceneOrigin(),origin,12))continue;
                if(town_==id&&snapshot_.get()&&snapshot_->Geometry().SameContents(fresh->Geometry()))return S_FALSE;
                snapshot_=fresh;bounds_=b;town_=id;generation_=generation;scene_=scene;Dirty();return S_OK;
            }
            Clear();return S_FALSE;
        }catch(const std::exception&e){error=e.what();Clear();return E_FAIL;}
        catch(...){error="loaded town snapshot failed";Clear();return E_FAIL;}
    }
};
}
