#pragma once
// Test-only exhaustive D3D11.0 immediate pipeline snapshot. Every Get reference
// is released. Values/slots not written by a Get remain zero for byte comparison.
namespace AtlasViewTest {
template<class T,unsigned N>void Drop(T*(&v)[N]){for(unsigned i=0;i<N;++i)if(v[i])v[i]->Release();}
struct ContextFixture {
    IUnknown*shader[6];ID3D11ClassInstance*instances[6][256];UINT classes[6];
    ID3D11ShaderResourceView*views[6][128];ID3D11Buffer*cb[6][14];ID3D11SamplerState*samplers[6][16];
    ID3D11UnorderedAccessView*csUav[8],*omUav[8];ID3D11RenderTargetView*rt[8];ID3D11DepthStencilView*ds;
    ID3D11BlendState*blend;FLOAT blendFactor[4];UINT mask;ID3D11DepthStencilState*depth;UINT stencil;
    ID3D11RasterizerState*raster;D3D11_VIEWPORT viewport[16];UINT viewportCount;D3D11_RECT rect[16];UINT rectCount;
    ID3D11InputLayout*layout;D3D11_PRIMITIVE_TOPOLOGY topology;ID3D11Buffer*vb[32];UINT strides[32],offsets[32];
    ID3D11Buffer*ib;DXGI_FORMAT indexFormat;UINT indexOffset;ID3D11Buffer*so[4];ID3D11Predicate*predicate;BOOL predicateValue;
    ContextFixture(ID3D11DeviceContext*c){
        std::memset(this,0,sizeof(*this));for(unsigned i=0;i<6;++i)classes[i]=256;
        ID3D11VertexShader*vs=NULL;ID3D11HullShader*hs=NULL;ID3D11DomainShader*ds_=NULL;ID3D11GeometryShader*gs=NULL;ID3D11PixelShader*ps=NULL;ID3D11ComputeShader*cs=NULL;
        c->VSGetShader(&vs,instances[0],&classes[0]);c->HSGetShader(&hs,instances[1],&classes[1]);c->DSGetShader(&ds_,instances[2],&classes[2]);c->GSGetShader(&gs,instances[3],&classes[3]);c->PSGetShader(&ps,instances[4],&classes[4]);c->CSGetShader(&cs,instances[5],&classes[5]);shader[0]=vs;shader[1]=hs;shader[2]=ds_;shader[3]=gs;shader[4]=ps;shader[5]=cs;
        c->VSGetShaderResources(0,128,views[0]);c->HSGetShaderResources(0,128,views[1]);c->DSGetShaderResources(0,128,views[2]);c->GSGetShaderResources(0,128,views[3]);c->PSGetShaderResources(0,128,views[4]);c->CSGetShaderResources(0,128,views[5]);
        c->VSGetConstantBuffers(0,14,cb[0]);c->HSGetConstantBuffers(0,14,cb[1]);c->DSGetConstantBuffers(0,14,cb[2]);c->GSGetConstantBuffers(0,14,cb[3]);c->PSGetConstantBuffers(0,14,cb[4]);c->CSGetConstantBuffers(0,14,cb[5]);
        c->VSGetSamplers(0,16,samplers[0]);c->HSGetSamplers(0,16,samplers[1]);c->DSGetSamplers(0,16,samplers[2]);c->GSGetSamplers(0,16,samplers[3]);c->PSGetSamplers(0,16,samplers[4]);c->CSGetSamplers(0,16,samplers[5]);
        c->CSGetUnorderedAccessViews(0,8,csUav);c->OMGetRenderTargets(8,rt,&ds);c->OMGetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,0,8,omUav);c->OMGetBlendState(&blend,blendFactor,&mask);c->OMGetDepthStencilState(&depth,&stencil);c->RSGetState(&raster);viewportCount=rectCount=16;c->RSGetViewports(&viewportCount,viewport);c->RSGetScissorRects(&rectCount,rect);c->IAGetInputLayout(&layout);c->IAGetPrimitiveTopology(&topology);c->IAGetVertexBuffers(0,32,vb,strides,offsets);c->IAGetIndexBuffer(&ib,&indexFormat,&indexOffset);c->SOGetTargets(4,so);c->GetPredication(&predicate,&predicateValue);
    }
    ~ContextFixture(){Drop(shader);for(unsigned i=0;i<6;++i){Drop(instances[i]);Drop(views[i]);Drop(cb[i]);Drop(samplers[i]);}Drop(csUav);Drop(omUav);Drop(rt);Drop(vb);Drop(so);if(ds)ds->Release();if(blend)blend->Release();if(depth)depth->Release();if(raster)raster->Release();if(layout)layout->Release();if(ib)ib->Release();if(predicate)predicate->Release();}
    bool Equal(const ContextFixture&other)const{return std::memcmp(this,&other,sizeof(*this))==0;}
private:ContextFixture(const ContextFixture&);ContextFixture&operator=(const ContextFixture&);
};
}
