#include "GraphicsState.h"
#include "../atlas-view/ContextFixture.h"
#include <cstdio>
using AtlasView::Com;
static unsigned checks=0;
static void Check(bool b,const char*s){++checks;if(!b){std::fprintf(stderr,"FAIL %s\n",s);throw 1;}}
static void Hr(HRESULT h,const char*s){Check(SUCCEEDED(h),s);}
static void Perturb(ID3D11DeviceContext*c){
 ID3D11Buffer*b[16]={0};UINT v[16]={0};c->IASetVertexBuffers(0,16,b,v,v);c->IASetIndexBuffer(NULL,DXGI_FORMAT_R16_UINT,0);c->IASetInputLayout(NULL);c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
 c->VSSetShader(NULL,NULL,0);c->HSSetShader(NULL,NULL,0);c->DSSetShader(NULL,NULL,0);c->GSSetShader(NULL,NULL,0);c->PSSetShader(NULL,NULL,0);c->VSSetConstantBuffers(0,1,b);c->PSSetConstantBuffers(0,1,b);
 ID3D11ShaderResourceView*s=NULL;c->PSSetShaderResources(0,1,&s);ID3D11SamplerState*sam=NULL;c->PSSetSamplers(0,1,&sam);
 c->RSSetState(NULL);D3D11_VIEWPORT vp={0,0,256,256,0,1};c->RSSetViewports(1,&vp);c->RSSetScissorRects(0,NULL);c->OMSetBlendState(NULL,NULL,~0u);c->OMSetDepthStencilState(NULL,0);ID3D11UnorderedAccessView*u[8]={0};c->OMSetRenderTargetsAndUnorderedAccessViews(0,NULL,NULL,0,8,u,NULL);c->SetPredication(NULL,FALSE);
}
int main(){try{
 Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;D3D_FEATURE_LEVEL got;Hr(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,&got,&c.p),"WARP");
 Com<ID3D11Buffer>vb,ib,cb;D3D11_BUFFER_DESC b={};b.ByteWidth=128;b.BindFlags=D3D11_BIND_VERTEX_BUFFER;Hr(d.p->CreateBuffer(&b,NULL,&vb.p),"VB");b.BindFlags=D3D11_BIND_INDEX_BUFFER;Hr(d.p->CreateBuffer(&b,NULL,&ib.p),"IB");b.BindFlags=D3D11_BIND_CONSTANT_BUFFER;Hr(d.p->CreateBuffer(&b,NULL,&cb.p),"CB");UINT stride=32,offset=8;c.p->IASetVertexBuffers(3,1,&vb.p,&stride,&offset);c.p->IASetIndexBuffer(ib.p,DXGI_FORMAT_R32_UINT,12);c.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP);c.p->VSSetConstantBuffers(0,1,&cb.p);c.p->PSSetConstantBuffers(0,1,&cb.p);
 Com<ID3D11RasterizerState>rs;D3D11_RASTERIZER_DESC rd={};rd.FillMode=D3D11_FILL_WIREFRAME;rd.CullMode=D3D11_CULL_FRONT;rd.ScissorEnable=TRUE;Hr(d.p->CreateRasterizerState(&rd,&rs.p),"RS");c.p->RSSetState(rs.p);D3D11_VIEWPORT vp[2]={{3,7,111,99,.1f,.8f},{17,19,43,51,0,1}};c.p->RSSetViewports(2,vp);D3D11_RECT rect[2]={{1,2,10,20},{4,5,30,50}};c.p->RSSetScissorRects(2,rect);
 Com<ID3D11BlendState>bs;D3D11_BLEND_DESC blend={};blend.RenderTarget[0].RenderTargetWriteMask=3;Hr(d.p->CreateBlendState(&blend,&bs.p),"blend");float factor[4]={.1f,.2f,.3f,.4f};c.p->OMSetBlendState(bs.p,factor,0x3355u);Com<ID3D11DepthStencilState>ds;D3D11_DEPTH_STENCIL_DESC depth={};depth.DepthFunc=D3D11_COMPARISON_GREATER;depth.StencilReadMask=depth.StencilWriteMask=255;depth.FrontFace.StencilFunc=depth.BackFace.StencilFunc=D3D11_COMPARISON_ALWAYS;depth.FrontFace.StencilFailOp=depth.FrontFace.StencilDepthFailOp=depth.FrontFace.StencilPassOp=D3D11_STENCIL_OP_KEEP;depth.BackFace=depth.FrontFace;Hr(d.p->CreateDepthStencilState(&depth,&ds.p),"depth");c.p->OMSetDepthStencilState(ds.p,31);
 {AtlasViewTest::ContextFixture before(c.p);{AtlasModels::GraphicsState saved(c.p);Perturb(c.p);}AtlasViewTest::ContextFixture after(c.p);Check(before.Equal(after),"complete touched graphics state restored");}
 c.p->ClearState();std::printf("PASS %u graphics checks\n",checks);return 0;
 }catch(...){return 1;}}
