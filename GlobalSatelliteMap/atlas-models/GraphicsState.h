#pragma once
#include "../atlas-view/AtlasRelight.h"
namespace AtlasModels {
inline bool NoStreamOutput(ID3D11DeviceContext*c){ID3D11Buffer*b[4]={0};c->SOGetTargets(4,b);bool empty=true;for(unsigned i=0;i<4;++i)if(b[i]){empty=false;b[i]->Release();}return empty;}
class GraphicsState {
    ID3D11DeviceContext*c_;ID3D11InputLayout*layout_;ID3D11Buffer*vb_[16],*ib_,*vsCb_,*psCb_;UINT stride_[16],offset_[16],indexOffset_;DXGI_FORMAT indexFormat_;D3D11_PRIMITIVE_TOPOLOGY topology_;
    ID3D11VertexShader*vs_;ID3D11HullShader*hs_;ID3D11DomainShader*ds_;ID3D11GeometryShader*gs_;ID3D11PixelShader*ps_;ID3D11ClassInstance*classes_[5][256];UINT count_[5];
    ID3D11ShaderResourceView*view_;ID3D11SamplerState*sampler_;ID3D11RasterizerState*raster_;D3D11_VIEWPORT viewport_[16];D3D11_RECT rect_[16];UINT viewportCount_,rectCount_;
    ID3D11RenderTargetView*rt_[8];ID3D11DepthStencilView*depthView_;ID3D11UnorderedAccessView*uav_[8];ID3D11BlendState*blend_;FLOAT factor_[4];UINT mask_;ID3D11DepthStencilState*depth_;UINT stencil_;ID3D11Predicate*predicate_;BOOL predicateValue_;
    template<class T>static void Drop(T*p){if(p)p->Release();}
    GraphicsState(const GraphicsState&);GraphicsState&operator=(const GraphicsState&);
public:explicit GraphicsState(ID3D11DeviceContext*c):c_(c){
        std::memset(classes_,0,sizeof(classes_));for(unsigned i=0;i<5;++i)count_[i]=256;
        c_->IAGetInputLayout(&layout_);c_->IAGetVertexBuffers(0,16,vb_,stride_,offset_);c_->IAGetIndexBuffer(&ib_,&indexFormat_,&indexOffset_);c_->IAGetPrimitiveTopology(&topology_);
        c_->VSGetShader(&vs_,classes_[0],&count_[0]);c_->HSGetShader(&hs_,classes_[1],&count_[1]);c_->DSGetShader(&ds_,classes_[2],&count_[2]);c_->GSGetShader(&gs_,classes_[3],&count_[3]);c_->PSGetShader(&ps_,classes_[4],&count_[4]);
        c_->VSGetConstantBuffers(0,1,&vsCb_);c_->PSGetConstantBuffers(0,1,&psCb_);c_->PSGetShaderResources(0,1,&view_);c_->PSGetSamplers(0,1,&sampler_);
        c_->RSGetState(&raster_);viewportCount_=rectCount_=16;std::memset(viewport_,0,sizeof(viewport_));std::memset(rect_,0,sizeof(rect_));c_->RSGetViewports(&viewportCount_,viewport_);c_->RSGetScissorRects(&rectCount_,rect_);
        c_->OMGetRenderTargets(8,rt_,&depthView_);c_->OMGetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,0,8,uav_);c_->OMGetBlendState(&blend_,factor_,&mask_);c_->OMGetDepthStencilState(&depth_,&stencil_);c_->GetPredication(&predicate_,&predicateValue_);
    }
    ~GraphicsState(){
        ID3D11UnorderedAccessView*empty[8]={0};c_->OMSetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,0,8,empty,NULL);
        unsigned rtCount=8;while(rtCount&&!rt_[rtCount-1])--rtCount;UINT keep[8];for(unsigned i=0;i<8;++i)keep[i]=~0u;
        c_->OMSetRenderTargetsAndUnorderedAccessViews(rtCount,rt_,depthView_,rtCount,8-rtCount,rtCount<8?uav_+rtCount:NULL,keep);
        c_->OMSetBlendState(blend_,factor_,mask_);c_->OMSetDepthStencilState(depth_,stencil_);
        c_->IASetInputLayout(layout_);c_->IASetVertexBuffers(0,16,vb_,stride_,offset_);c_->IASetIndexBuffer(ib_,indexFormat_,indexOffset_);c_->IASetPrimitiveTopology(topology_);
        c_->VSSetShader(vs_,classes_[0],count_[0]);c_->HSSetShader(hs_,classes_[1],count_[1]);c_->DSSetShader(ds_,classes_[2],count_[2]);c_->GSSetShader(gs_,classes_[3],count_[3]);c_->PSSetShader(ps_,classes_[4],count_[4]);
        c_->VSSetConstantBuffers(0,1,&vsCb_);c_->PSSetConstantBuffers(0,1,&psCb_);c_->PSSetShaderResources(0,1,&view_);c_->PSSetSamplers(0,1,&sampler_);c_->RSSetState(raster_);c_->RSSetViewports(viewportCount_,viewport_);c_->RSSetScissorRects(rectCount_,rect_);c_->SetPredication(predicate_,predicateValue_);
        Drop(layout_);for(unsigned i=0;i<16;++i)Drop(vb_[i]);Drop(ib_);Drop(vsCb_);Drop(psCb_);Drop(vs_);Drop(hs_);Drop(ds_);Drop(gs_);Drop(ps_);for(unsigned i=0;i<5;++i)for(unsigned j=0;j<count_[i];++j)Drop(classes_[i][j]);Drop(view_);Drop(sampler_);Drop(raster_);for(unsigned i=0;i<8;++i){Drop(rt_[i]);Drop(uav_[i]);}Drop(depthView_);Drop(blend_);Drop(depth_);Drop(predicate_);
    }
};
}
