#pragma once
#include "ModelPolicy.h"
#include "GraphicsState.h"
namespace AtlasModels {
struct RawDraw {Shape shape;ID3D11Buffer*vertex[16];ID3D11Buffer*index;ID3D11ShaderResourceView*diffuse;ID3D11SamplerState*sampler;unsigned cullMode;RawDraw():index(NULL),diffuse(NULL),sampler(NULL),cullMode(2){std::memset(&shape,0,sizeof(shape));std::memset(vertex,0,sizeof(vertex));}};
class Packet {
    struct Draw {Shape shape;AtlasView::Com<ID3D11Buffer>vertex[16],index;AtlasView::Com<ID3D11ShaderResourceView>diffuse;AtlasView::Com<ID3D11SamplerState>sampler;AtlasView::Com<ID3D11InputLayout>layout;AtlasView::Com<ID3D11RasterizerState>raster;};
    Draw draw_[16];unsigned count_,generation_;float rawSceneWorld_[16],sourceSceneOrigin_[3];
    Packet(const Packet&);Packet&operator=(const Packet&);
    friend class Raster;
public:Packet():count_(0),generation_(0){std::memset(rawSceneWorld_,0,sizeof(rawSceneWorld_));std::memset(sourceSceneOrigin_,0,sizeof(sourceSceneOrigin_));}
    void Clear(){for(unsigned n=0;n<16;++n){for(unsigned i=0;i<16;++i)draw_[n].vertex[i].Reset();draw_[n].index.Reset();draw_[n].diffuse.Reset();draw_[n].sampler.Reset();draw_[n].layout.Reset();draw_[n].raster.Reset();}count_=generation_=0;std::memset(rawSceneWorld_,0,sizeof(rawSceneWorld_));std::memset(sourceSceneOrigin_,0,sizeof(sourceSceneOrigin_));}
    unsigned Count()const{return count_;}unsigned Generation()const{return generation_;}const float*RawSceneWorld()const{return rawSceneWorld_;}const float*SourceSceneOrigin()const{return sourceSceneOrigin_;}
    HRESULT Adopt(ID3D11Device*d,const RawDraw*raw,unsigned count,const float*world,const float*origin,unsigned generation,const void*vs,SIZE_T bytes){
        if(!d||!raw||!count||count>16||count_||!world||!origin||!generation||!vs||!bytes||!Finite(world,16)||!Finite(origin,3))return E_INVALIDARG;
        ID3D11Buffer*unique[272]={0};unsigned uniqueCount=0;U64 total=0,vertices=0,indices=0;
        for(unsigned n=0;n<count;++n){const RawDraw&r=raw[n];if(!ValidShape(r.shape)||!r.index||!r.diffuse||!r.sampler||r.cullMode<1||r.cullMode>3)return E_INVALIDARG;
            for(unsigned k=r.shape.streamCount;k<16;++k)if(r.vertex[k])return E_INVALIDARG;
            if(!AtlasView::LightView(r.diffuse,d,false,1))return E_INVALIDARG;
            if(!AtlasView::SameDevice(r.diffuse,d)||!AtlasView::SameDevice(r.sampler,d))return E_INVALIDARG;
            D3D11_SHADER_RESOURCE_VIEW_DESC sv;r.diffuse->GetDesc(&sv);AtlasView::Com<ID3D11Resource>res;r.diffuse->GetResource(&res.p);AtlasView::Com<ID3D11Texture2D>texture;
            if(sv.ViewDimension!=D3D11_SRV_DIMENSION_TEXTURE2D||FAILED(res.p->QueryInterface(__uuidof(ID3D11Texture2D),reinterpret_cast<void**>(&texture.p))))return E_INVALIDARG;
            D3D11_TEXTURE2D_DESC td;texture.p->GetDesc(&td);if(td.ArraySize!=1||td.SampleDesc.Count!=1||td.SampleDesc.Quality||td.MiscFlags&D3D11_RESOURCE_MISC_TEXTURECUBE)return E_INVALIDARG;
            D3D11_SAMPLER_DESC sd;r.sampler->GetDesc(&sd);if(sd.ComparisonFunc!=D3D11_COMPARISON_NEVER||!_finite(sd.MipLODBias)||!_finite(sd.MinLOD)||!_finite(sd.MaxLOD))return E_INVALIDARG;
            vertices+=r.shape.vertexCount;indices+=r.shape.indexCount;if(vertices>MaxVertices||indices>MaxIndices)return E_INVALIDARG;
            for(unsigned i=0;i<=r.shape.streamCount;++i){ID3D11Buffer*b=i==r.shape.streamCount?r.index:r.vertex[i];if(!b){if(i==r.shape.streamCount||r.shape.bytes[i])return E_INVALIDARG;continue;}
                if(!AtlasView::SameDevice(b,d))return E_INVALIDARG;D3D11_BUFFER_DESC desc;b->GetDesc(&desc);unsigned expected=i==r.shape.streamCount?r.shape.indexBytes:r.shape.bytes[i];unsigned bind=i==r.shape.streamCount?D3D11_BIND_INDEX_BUFFER:D3D11_BIND_VERTEX_BUFFER;
                if(desc.ByteWidth!=expected||desc.Usage==D3D11_USAGE_STAGING||!(desc.BindFlags&bind))return E_INVALIDARG;
                bool seen=false;for(unsigned k=0;k<uniqueCount;++k)if(unique[k]==b)seen=true;if(!seen){unique[uniqueCount++]=b;total+=desc.ByteWidth;if(total>BorrowCap)return E_OUTOFMEMORY;}
            }
        }
        for(unsigned n=0;n<count;++n){const RawDraw&r=raw[n];Draw&out=draw_[n];D3D11_INPUT_ELEMENT_DESC e[4]={};for(unsigned i=0;i<4;++i){e[i].SemanticName=Name(r.shape.element[i].semantic);e[i].Format=r.shape.element[i].format;e[i].InputSlot=r.shape.element[i].slot;e[i].AlignedByteOffset=r.shape.element[i].offset;}
            HRESULT hr=d->CreateInputLayout(e,4,vs,bytes,&out.layout.p);D3D11_RASTERIZER_DESC rs={};rs.FillMode=D3D11_FILL_SOLID;rs.CullMode=r.cullMode==1?D3D11_CULL_NONE:r.cullMode==2?D3D11_CULL_BACK:D3D11_CULL_FRONT;rs.FrontCounterClockwise=TRUE;rs.DepthClipEnable=TRUE;rs.MultisampleEnable=TRUE;
            if(SUCCEEDED(hr))hr=d->CreateRasterizerState(&rs,&out.raster.p);if(FAILED(hr)){Clear();return hr;}
            out.shape=r.shape;for(unsigned i=0;i<16;++i)if(r.vertex[i]){out.vertex[i].p=r.vertex[i];r.vertex[i]->AddRef();}out.index.p=r.index;r.index->AddRef();out.diffuse.p=r.diffuse;r.diffuse->AddRef();out.sampler.p=r.sampler;r.sampler->AddRef();
        }
        std::memcpy(rawSceneWorld_,world,64);std::memcpy(sourceSceneOrigin_,origin,12);generation_=generation;count_=count;return S_OK;
    }
};
class Raster {
    AtlasView::Com<ID3D11Device>device_;AtlasView::Com<ID3D11VertexShader>vs_;AtlasView::Com<ID3D11PixelShader>ps_;AtlasView::Com<ID3D11Buffer>vcb_,pcb_;AtlasView::Com<ID3D11Texture2D>texture_[3],depth_;AtlasView::Com<ID3D11ShaderResourceView>view_[3];AtlasView::Com<ID3D11RenderTargetView>rt_[3];AtlasView::Com<ID3D11DepthStencilView>dsv_;AtlasView::Com<ID3D11DepthStencilState>depthState_;AtlasView::Com<ID3D11BlendState>blend_;
    Raster(const Raster&);Raster&operator=(const Raster&);
public:Raster(){}void Clear(){blend_.Reset();depthState_.Reset();dsv_.Reset();depth_.Reset();for(unsigned i=0;i<3;++i){rt_[i].Reset();view_[i].Reset();texture_[i].Reset();}pcb_.Reset();vcb_.Reset();ps_.Reset();vs_.Reset();device_.Reset();}
    HRESULT Init(ID3D11Device*d,const void*vs,SIZE_T vsBytes,const void*ps,SIZE_T psBytes,U64 caller){
        if(!d||device_.p||!vs||!vsBytes||!ps||!psBytes)return E_INVALIDARG;if(!Fits(caller))return E_OUTOFMEMORY;
        HRESULT hr=d->CreateVertexShader(vs,vsBytes,NULL,&vs_.p);if(SUCCEEDED(hr))hr=d->CreatePixelShader(ps,psBytes,NULL,&ps_.p);
        for(unsigned i=0;i<3&&SUCCEEDED(hr);++i){D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=256;td.ArraySize=td.MipLevels=1;td.Format=i==2?DXGI_FORMAT_R32_FLOAT:DXGI_FORMAT_R8G8B8A8_UNORM;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;hr=d->CreateTexture2D(&td,NULL,&texture_[i].p);if(SUCCEEDED(hr))hr=d->CreateShaderResourceView(texture_[i].p,NULL,&view_[i].p);if(SUCCEEDED(hr))hr=d->CreateRenderTargetView(texture_[i].p,NULL,&rt_[i].p);}
        D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=256;td.ArraySize=td.MipLevels=1;td.Format=DXGI_FORMAT_D32_FLOAT;td.SampleDesc.Count=1;td.BindFlags=D3D11_BIND_DEPTH_STENCIL;if(SUCCEEDED(hr))hr=d->CreateTexture2D(&td,NULL,&depth_.p);if(SUCCEEDED(hr))hr=d->CreateDepthStencilView(depth_.p,NULL,&dsv_.p);
        D3D11_BUFFER_DESC b={};b.ByteWidth=128;b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;if(SUCCEEDED(hr))hr=d->CreateBuffer(&b,NULL,&vcb_.p);b.ByteWidth=16;if(SUCCEEDED(hr))hr=d->CreateBuffer(&b,NULL,&pcb_.p);
        D3D11_DEPTH_STENCIL_DESC ds={};ds.DepthEnable=TRUE;ds.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;ds.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;if(SUCCEEDED(hr))hr=d->CreateDepthStencilState(&ds,&depthState_.p);D3D11_BLEND_DESC blend={};for(unsigned i=0;i<8;++i)blend.RenderTarget[i].RenderTargetWriteMask=15;if(SUCCEEDED(hr))hr=d->CreateBlendState(&blend,&blend_.p);
        if(FAILED(hr)){Clear();return hr;}device_.p=d;d->AddRef();return S_OK;
    }
    HRESULT Render(bool enabled,const Packet&p,const View&v,U64 caller){
        if(!enabled)return S_FALSE;if(!device_.p||!p.count_||v.generation!=p.generation_||!Finite(v.viewProjection,16)||!Finite(v.cameraScene,3)||!_finite(v.farClip)||v.farClip<=0)return E_INVALIDARG;if(!Fits(caller))return E_OUTOFMEMORY;
        AtlasView::Com<ID3D11DeviceContext>c;device_.p->GetImmediateContext(&c.p);if(!c.p||!NoStreamOutput(c.p))return E_INVALIDARG;
        for(unsigned i=0;i<3;++i)if(AtlasView::Bound(c.p,texture_[i].p,true))return E_INVALIDARG;if(AtlasView::Bound(c.p,depth_.p,true))return E_INVALIDARG;
        for(unsigned n=0;n<p.count_;++n){const Packet::Draw&d=p.draw_[n];if(!AtlasView::SameDevice(d.index.p,device_.p))return E_INVALIDARG;AtlasView::Com<ID3D11Resource>r;d.diffuse.p->GetResource(&r.p);for(unsigned k=0;k<3;++k)if(AtlasView::Identity(r.p,texture_[k].p))return E_INVALIDARG;if(AtlasView::Identity(r.p,depth_.p)||AtlasView::Bound(c.p,r.p,true)||AtlasView::Bound(c.p,d.index.p,true))return E_INVALIDARG;for(unsigned k=0;k<16;++k)if(d.vertex[k].p&&AtlasView::Bound(c.p,d.vertex[k].p,true))return E_INVALIDARG;}
        float vertex[32],pixel[4]={v.cameraScene[0],v.cameraScene[1],v.cameraScene[2],v.farClip};MatrixConstants(p.rawSceneWorld_,v,vertex);if(!Finite(vertex,32))return E_INVALIDARG;
        AtlasView::SavedOutputReads reads0(c.p,texture_[0].p),reads1(c.p,texture_[1].p),reads2(c.p,texture_[2].p);
        GraphicsState saved(c.p);c.p->SetPredication(NULL,FALSE);c.p->UpdateSubresource(vcb_.p,0,NULL,vertex,0,0);c.p->UpdateSubresource(pcb_.p,0,NULL,pixel,0,0);
        ID3D11UnorderedAccessView*empty[8]={0};c.p->OMSetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,0,8,empty,NULL);ID3D11RenderTargetView*targets[3]={rt_[0].p,rt_[1].p,rt_[2].p};c.p->OMSetRenderTargets(3,targets,dsv_.p);const float zero[4]={0};for(unsigned i=0;i<3;++i)c.p->ClearRenderTargetView(rt_[i].p,zero);c.p->ClearDepthStencilView(dsv_.p,D3D11_CLEAR_DEPTH,1,0);
        c.p->VSSetShader(vs_.p,NULL,0);c.p->HSSetShader(NULL,NULL,0);c.p->DSSetShader(NULL,NULL,0);c.p->GSSetShader(NULL,NULL,0);c.p->PSSetShader(ps_.p,NULL,0);c.p->VSSetConstantBuffers(0,1,&vcb_.p);c.p->PSSetConstantBuffers(0,1,&pcb_.p);D3D11_VIEWPORT vp={0,0,256,256,0,1};c.p->RSSetViewports(1,&vp);c.p->RSSetScissorRects(0,NULL);c.p->OMSetBlendState(blend_.p,NULL,~0u);c.p->OMSetDepthStencilState(depthState_.p,0);c.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
        for(unsigned n=0;n<p.count_;++n){const Packet::Draw&d=p.draw_[n];ID3D11Buffer*vb[16];for(unsigned i=0;i<16;++i)vb[i]=d.vertex[i].p;c.p->IASetVertexBuffers(0,16,vb,d.shape.stride,d.shape.offset);c.p->IASetIndexBuffer(d.index.p,d.shape.indexFormat,0);c.p->IASetInputLayout(d.layout.p);c.p->RSSetState(d.raster.p);c.p->PSSetShaderResources(0,1,&d.diffuse.p);c.p->PSSetSamplers(0,1,&d.sampler.p);c.p->DrawIndexed(d.shape.indexCount,d.shape.indexStart,INT(d.shape.vertexStart));}
        return S_OK;
    }
    ID3D11Texture2D*Texture(unsigned i)const{return i<3?texture_[i].p:NULL;}ID3D11ShaderResourceView*Output(unsigned i)const{return i<3?view_[i].p:NULL;}
};
}
