#ifndef SATELLITE_MIP_COMPARISON_H
#define SATELLITE_MIP_COMPARISON_H
#include <d3d11.h>
#include <wincrypt.h>
#include <stddef.h>
#include <cstring>
#include <vector>
#include "MipComparisonPolicy.h"
namespace ProbeMipComparison
{
enum State {Idle,CopyNative,MapNative,CopyArray,MapArray,Equal,Different,Cancelled,Failed};
struct LevelResult
{
    unsigned width,height;
    unsigned char nativeHash[32],arrayHash[32],maxError[4];
    unsigned __int64 mismatches[4],sumError[4];
};
class Comparison
{
    ID3D11Texture2D *nativeTexture,*arrayTexture,*staging;
    ID3D11Device *device;ID3D11DeviceContext *context;HCRYPTPROV provider;
    std::vector<unsigned char> snapshot;
    State state;HRESULT error;
    unsigned side,levels,layer,mip,count,attempts,readbacks;
    unsigned long started,timeout;
    unsigned __int64 compared;
    DXGI_FORMAT format;bool different;
    LevelResult results[12];

    template<class T> struct Scoped
    {
        T* p;Scoped():p(NULL){}~Scoped(){if(p)p->Release();}
        T* Detach(){T* value=p;p=NULL;return value;}
    private:Scoped(const Scoped&);Scoped& operator=(const Scoped&);
    };
    struct Hash
    {
        HCRYPTHASH value;Hash():value(0){}~Hash(){if(value)CryptDestroyHash(value);}
    private:Hash(const Hash&);Hash& operator=(const Hash&);
    };
    struct Mapping
    {
        ID3D11DeviceContext *context;ID3D11Texture2D *resource;
        Mapping(ID3D11DeviceContext *c,ID3D11Texture2D *r):context(c),resource(r){}
        ~Mapping(){context->Unmap(resource,0);}
    private:Mapping(const Mapping&);Mapping& operator=(const Mapping&);
    };
    bool Active() const{return state>=CopyNative&&state<=MapArray;}
    void Release()
    {
        if(staging){staging->Release();staging=NULL;}
        if(nativeTexture){nativeTexture->Release();nativeTexture=NULL;}
        if(arrayTexture){arrayTexture->Release();arrayTexture=NULL;}
        if(context){context->Release();context=NULL;}
        if(device){device->Release();device=NULL;}
        if(provider){CryptReleaseContext(provider,0);provider=0;}
        std::vector<unsigned char>().swap(snapshot);
    }
    State Fail(HRESULT hr){error=hr;Release();state=Failed;return state;}
    static bool Shape(const D3D11_TEXTURE2D_DESC& d,unsigned s,unsigned n,unsigned slices,DXGI_FORMAT f)
    {
        return d.Width==s&&d.Height==s&&d.MipLevels==n&&d.ArraySize==slices&&d.Format==f&&
            d.SampleDesc.Count==1&&d.SampleDesc.Quality==0&&d.Usage==D3D11_USAGE_DEFAULT&&d.CPUAccessFlags==0&&
            d.BindFlags==(D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET)&&d.MiscFlags==D3D11_RESOURCE_MISC_GENERATE_MIPS;
    }
    HRESULT Read(bool nativeRead)
    {
        D3D11_MAPPED_SUBRESOURCE mapped={};++attempts;
        HRESULT hr=context->Map(staging,0,D3D11_MAP_READ,D3D11_MAP_FLAG_DO_NOT_WAIT,&mapped);
        if(FAILED(hr))return hr;
        Mapping unmap(context,staging); // Destroy before Tick can release COM.
        LevelResult& result=results[mip];const size_t rowBytes=size_t(result.width)*4;
        if(!mapped.pData||mapped.RowPitch<rowBytes||
            (result.height>1&&size_t(mapped.RowPitch)>size_t(-1)/(result.height-1)))return E_FAIL;
        Hash hash;
        if(!CryptCreateHash(provider,CALG_SHA_256,0,0,&hash.value))return HRESULT_FROM_WIN32(GetLastError());
        for(unsigned y=0;y<result.height;++y)
        {
            const unsigned char* row=static_cast<const unsigned char*>(mapped.pData)+size_t(mapped.RowPitch)*y;
            unsigned char* reference=&snapshot[size_t(y)*rowBytes];
            if(!CryptHashData(hash.value,row,static_cast<DWORD>(rowBytes),0))return HRESULT_FROM_WIN32(GetLastError());
            if(nativeRead)std::memcpy(reference,row,rowBytes);
            else for(size_t x=0;x<rowBytes;++x)
            {
                unsigned channel=unsigned(x&3);int delta=int(reference[x])-int(row[x]);if(delta<0)delta=-delta;
                if(delta){++result.mismatches[channel];result.sumError[channel]+=unsigned(delta);
                    if(delta>result.maxError[channel])result.maxError[channel]=static_cast<unsigned char>(delta);}
            }
        }
        DWORD bytes=32;
        if(!CryptGetHashParam(hash.value,HP_HASHVAL,nativeRead?result.nativeHash:result.arrayHash,&bytes,0)||bytes!=32)
            return E_FAIL;
        ++readbacks;return S_OK;
    }
    Comparison(const Comparison&);Comparison& operator=(const Comparison&);
public:
    Comparison():nativeTexture(NULL),arrayTexture(NULL),staging(NULL),device(NULL),context(NULL),provider(0),
        state(Idle),error(S_OK),side(0),levels(0),layer(0),mip(0),count(0),attempts(0),readbacks(0),
        started(0),timeout(0),compared(0),format(DXGI_FORMAT_UNKNOWN),different(false)
    {std::memset(results,0,sizeof(results));}
    ~Comparison(){Release();}

    // COM only, render-thread serialized, no engine/native methods. Views are
    // borrowed during Begin only; their textures acquire two owned references.
    // One pair runs at a time. Caller must exclude other capture work and count
    // every other retained texel allocation in retainedTexelBytes (default0).
    // Native TexturePtr/accounting ownership belongs to the caller, which must
    // outlive this job; COM cleanup never substitutes for native completion.
    HRESULT Begin(ID3D11ShaderResourceView* nativeView,ID3D11ShaderResourceView* arrayView,
        unsigned expectedSide,unsigned expectedLevels,unsigned expectedLayer,DXGI_FORMAT expectedFormat,
        unsigned long now,unsigned long deadlineMs,size_t retainedTexelBytes=0)
    {
        if(Active())return E_UNEXPECTED;
        Budget budget;
        if(!nativeView||!arrayView||expectedLayer>=4||!deadlineMs||deadlineMs>120000||
            !Plan(expectedSide,expectedLevels,retainedTexelBytes,budget)||
            (expectedFormat!=DXGI_FORMAT_R8G8B8A8_UNORM&&expectedFormat!=DXGI_FORMAT_B8G8R8A8_UNORM))return E_INVALIDARG;
        D3D11_SHADER_RESOURCE_VIEW_DESC nv,av;nativeView->GetDesc(&nv);arrayView->GetDesc(&av);
        if(nv.Format!=expectedFormat||nv.ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2D||
            nv.Texture2D.MostDetailedMip!=0||nv.Texture2D.MipLevels!=expectedLevels||
            av.Format!=expectedFormat||av.ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2DARRAY||
            av.Texture2DArray.MostDetailedMip!=0||av.Texture2DArray.MipLevels!=expectedLevels||
            av.Texture2DArray.FirstArraySlice!=0||av.Texture2DArray.ArraySize!=4)return E_INVALIDARG;
        Scoped<ID3D11Resource> nr,ar;nativeView->GetResource(&nr.p);arrayView->GetResource(&ar.p);
        if(!nr.p||!ar.p||nr.p==ar.p)return E_INVALIDARG;
        Scoped<ID3D11Texture2D> nt,at;
        HRESULT hr=nr.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&nt.p));if(FAILED(hr))return hr;
        hr=ar.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&at.p));if(FAILED(hr))return hr;
        D3D11_TEXTURE2D_DESC nd,ad;nt.p->GetDesc(&nd);at.p->GetDesc(&ad);
        if(!Shape(nd,expectedSide,expectedLevels,1,expectedFormat)||
           !Shape(ad,expectedSide,expectedLevels,4,expectedFormat))return E_INVALIDARG;
        Scoped<ID3D11Device> ndv,adv;nt.p->GetDevice(&ndv.p);at.p->GetDevice(&adv.p);
        if(!ndv.p||ndv.p!=adv.p)return E_INVALIDARG;
        hr=ndv.p->GetDeviceRemovedReason();if(FAILED(hr))return hr;
        Scoped<ID3D11DeviceContext> ctx;ndv.p->GetImmediateContext(&ctx.p);if(!ctx.p)return E_FAIL;
        HCRYPTPROV nextProvider=0;
        if(!CryptAcquireContextA(&nextProvider,NULL,NULL,PROV_RSA_AES,CRYPT_VERIFYCONTEXT))return HRESULT_FROM_WIN32(GetLastError());
        Release();nativeTexture=nt.Detach();arrayTexture=at.Detach();device=ndv.Detach();context=ctx.Detach();provider=nextProvider;
        side=expectedSide;levels=expectedLevels;layer=expectedLayer;format=expectedFormat;started=now;timeout=deadlineMs;
        mip=count=attempts=readbacks=0;compared=0;different=false;error=S_OK;std::memset(results,0,sizeof(results));state=CopyNative;
        return S_OK;
    }

    // Exactly one copy OR one nonblocking Map attempt per Tick. No sleeps,
    // Flush/Present, shader/OM/viewport bindings or mapped pointer survives it.
    State Tick(unsigned long now)
    {
        if(!Active())return state;
        if(static_cast<unsigned long>(now-started)>=timeout||attempts>=65536)return Fail(HRESULT_FROM_WIN32(ERROR_TIMEOUT));
        HRESULT hr=device->GetDeviceRemovedReason();if(FAILED(hr))return Fail(hr);
        const unsigned extent=side>>mip;
        D3D11_BOX box={0,0,0,extent,extent,1};
        if(state==CopyNative)
        {
            if(staging){staging->Release();staging=NULL;}
            D3D11_TEXTURE2D_DESC d={};d.Width=d.Height=extent;d.MipLevels=d.ArraySize=1;d.Format=format;
            d.SampleDesc.Count=1;d.Usage=D3D11_USAGE_STAGING;d.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
            hr=device->CreateTexture2D(&d,NULL,&staging);if(FAILED(hr))return Fail(hr);
            try{snapshot.resize(size_t(extent)*extent*4);}catch(...){return Fail(E_OUTOFMEMORY);}
            results[mip].width=results[mip].height=extent;
            context->CopySubresourceRegion(staging,0,0,0,0,nativeTexture,mip,&box);
            state=MapNative;return state;
        }
        if(state==CopyArray)
        {
            context->CopySubresourceRegion(staging,0,0,0,0,arrayTexture,D3D11CalcSubresource(mip,layer,levels),&box);
            state=MapArray;return state;
        }
        const bool nativeRead=state==MapNative;
        hr=Read(nativeRead);
        if(hr==DXGI_ERROR_WAS_STILL_DRAWING)return state;
        if(FAILED(hr))return Fail(hr);
        if(nativeRead){state=CopyArray;return state;}
        ++count;compared+=size_t(extent)*extent*4;
        for(unsigned c=0;c<4;++c)if(results[mip].mismatches[c])different=true;
        if(++mip==levels){state=different?Different:Equal;Release();return state;}
        state=CopyNative;return state;
    }
    void Cancel(){if(Active()){Release();state=Cancelled;error=E_ABORT;}}
    State Current() const{return state;}
    unsigned Count() const{return count;}
    unsigned MapAttempts() const{return attempts;}
    unsigned Readbacks() const{return readbacks;}
    unsigned __int64 ComparedBytes() const{return compared;}
    // Only indices below Count are completed/valid. Invalid indices return an
    // immutable empty value without exposing out-of-bounds internal storage.
    const LevelResult& Result(unsigned index) const
    {static const LevelResult empty={};return index<count?results[index]:empty;}
    HRESULT Error() const{return error;}
};
}
#endif
