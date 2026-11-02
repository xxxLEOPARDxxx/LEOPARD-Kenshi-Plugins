#include "WaterPipeline.h"
#include "../atlas-view/AtlasWindowPolicy.h"
#include <cstdio>
static unsigned checks=0,bad=0;
#define CHECK(x) do{++checks;if(!(x)){++bad;std::printf("FAIL %u %s\n",__LINE__,#x);}}while(0)
int main(){
 CHECK(sizeof(AtlasView::Uniforms)==176&&sizeof(AtlasWater::Uniforms)==192);
 CHECK(AtlasWater::PipelinePrivateBytes==3408256ULL);
 CHECK(AtlasView::WindowGpuBytes()==189006000ULL);
 CHECK(AtlasView::WindowGpuBytes()+AtlasWater::PipelinePrivateBytes==192414256ULL);
 CHECK(AtlasWater::Cap==AtlasView::GpuCap);
 CHECK(AtlasWater::PipelineFits(AtlasView::GpuCap-AtlasWater::PipelinePrivateBytes));
 CHECK(!AtlasWater::PipelineFits(AtlasView::GpuCap-AtlasWater::PipelinePrivateBytes+1));
 CHECK(AtlasView::RelighterFits(AtlasView::GpuCap-AtlasWater::HdrTerrainPrivateBytes,256,DXGI_FORMAT_R16G16B16A16_FLOAT));
 CHECK(!AtlasView::RelighterFits(AtlasView::GpuCap-AtlasWater::HdrTerrainPrivateBytes+1,256,DXGI_FORMAT_R16G16B16A16_FLOAT));
 CHECK(!AtlasView::RelighterFits(0,512,DXGI_FORMAT_R16G16B16A16_FLOAT));
 CHECK(!AtlasView::RelighterFits(0,256,DXGI_FORMAT_R32G32B32A32_FLOAT));
 D3D11_TEXTURE2D_DESC d={};d.Width=d.Height=256;d.MipLevels=d.ArraySize=1;d.SampleDesc.Count=1;d.Format=DXGI_FORMAT_R16G16B16A16_FLOAT;d.Usage=D3D11_USAGE_DEFAULT;d.BindFlags=D3D11_BIND_SHADER_RESOURCE;
 CHECK(AtlasView::HdrOutputShape(d,256));CHECK(!AtlasView::HdrOutputShape(d,512));
 D3D11_TEXTURE2D_DESC v=d;v.MiscFlags=1;CHECK(!AtlasView::HdrOutputShape(v,256));
 v=d;v.BindFlags|=D3D11_BIND_RENDER_TARGET;CHECK(!AtlasView::HdrOutputShape(v,256));
 v=d;v.MipLevels=2;CHECK(!AtlasView::HdrOutputShape(v,256));
 v=d;v.SampleDesc.Count=2;CHECK(!AtlasView::HdrOutputShape(v,256));
 v=d;v.CPUAccessFlags=D3D11_CPU_ACCESS_READ;CHECK(!AtlasView::HdrOutputShape(v,256));
 v=d;v.Usage=D3D11_USAGE_STAGING;CHECK(!AtlasView::HdrOutputShape(v,256));
 v=d;v.Format=DXGI_FORMAT_R8G8B8A8_UNORM;v.MiscFlags=1;v.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;CHECK(AtlasView::OutputShape(v,256));
 AtlasWater::Pipeline off;AtlasView::Cell cell;AtlasView::Uniforms terrain={};AtlasWater::Uniforms water={};CHECK(off.Render(cell,terrain,NULL,NULL,water,NULL,NULL,NULL,~0ULL)==S_FALSE);
 CHECK(off.RenderWithStage(cell,terrain,NULL,NULL,water,NULL,NULL,NULL,~0ULL,NULL)==S_FALSE);
 CHECK(off.RenderTerrainWithStage(cell,terrain,NULL,NULL,NULL,~0ULL,NULL)==S_FALSE);
 AtlasWater::HdrStage invalid={};CHECK(off.RenderTerrainWithStage(cell,terrain,NULL,NULL,NULL,~0ULL,&invalid)==E_INVALIDARG);
 const AtlasWater::U64 outsideStage=526416ULL;
 CHECK(AtlasWater::PipelineFits(AtlasView::GpuCap-AtlasWater::PipelinePrivateBytes-outsideStage+outsideStage));
 CHECK(!AtlasWater::PipelineFits(AtlasView::GpuCap-AtlasWater::PipelinePrivateBytes-outsideStage+outsideStage+1));
 std::printf("Pipeline policy PASS %u checks,%u failures\n",checks,bad);return bad?1:0;
}
