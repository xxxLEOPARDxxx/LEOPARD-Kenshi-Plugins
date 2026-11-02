// Standalone owned WARP diagnostic, NEVER called on the game's D3D context.
#define NOMINMAX
#include <Windows.h>
#include <d3d11.h>
#include <cstdio>
#include <cstring>
#include <vector>
#include <cmath>
#include <stdexcept>
#include "../../../GlobalSatelliteMap/atlas-terrain/fixed-geometry/FixedTerrainGeometry.h"
#include "_generated/shaders.h"
template<class T>struct Com {T*p;Com():p(NULL){}~Com(){if(p)p->Release();}private:Com(const Com&);Com&operator=(const Com&);};
static unsigned checks=0;
static void Check(bool ok,const char*why){++checks;if(!ok)throw std::runtime_error(why);}
static void Hr(HRESULT hr,const char*why){Check(SUCCEEDED(hr),why);}
struct Pixel {float normal[3],height;};
struct View {float x,z,spacing;};
struct Fixture {
 std::vector<unsigned short> data;FixedTerrain::Samples samples;unsigned baseX,baseZ;
 Fixture(const char*path){struct File{FILE*p;File():p(NULL){}~File(){if(p)fclose(p);}} f;Check(fopen_s(&f.p,path,"rb")==0,"open real source fixture");unsigned h[4];Check(fread(h,4,4,f.p)==4,"fixture header");bool inside=h[0]==7358&&h[1]==7358&&h[2]==133&&h[3]==133,oldEdge=h[0]==7102&&h[1]==7358&&h[2]==196&&h[3]==134;Check(inside||oldEdge,"pinned fixture shape");baseX=inside?7360:7104;baseZ=7360;data.resize(h[2]*h[3]);size_t n=fread(&data[0],2,data.size(),f.p);int end=fgetc(f.p);Check(n==data.size()&&end==EOF,"exact fixture payload");samples.minX=h[0];samples.minZ=h[1];samples.width=h[2];samples.height=h[3];samples.stride=h[2];samples.data=&data[0];samples.sampleCount=data.size();}
};
class Raster {
 Com<ID3D11Device>d;Com<ID3D11DeviceContext>c;Com<ID3D11VertexShader>vs;Com<ID3D11PixelShader>ps;Com<ID3D11InputLayout>layout;Com<ID3D11Buffer>vb[4],ib,cb;Com<ID3D11Texture2D>target,depth,staging;Com<ID3D11RenderTargetView>rt;Com<ID3D11DepthStencilView>dv;Com<ID3D11RasterizerState>rs;Com<ID3D11DepthStencilState>ds;
public:
 explicit Raster(const Fixture&f){
  D3D_FEATURE_LEVEL level;Hr(D3D11CreateDevice(NULL,D3D_DRIVER_TYPE_WARP,NULL,0,NULL,0,D3D11_SDK_VERSION,&d.p,&level,&c.p),"owned WARP device");
  Hr(d.p->CreateVertexShader(VertexShader,sizeof(VertexShader),NULL,&vs.p),"diagnostic VS");Hr(d.p->CreatePixelShader(PixelShader,sizeof(PixelShader),NULL,&ps.p),"diagnostic PS");
  D3D11_INPUT_ELEMENT_DESC e[3]={{"POSITION",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,16,D3D11_INPUT_PER_VERTEX_DATA,0},{"BINORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,28,D3D11_INPUT_PER_VERTEX_DATA,0}};
  Hr(d.p->CreateInputLayout(e,3,VertexShader,sizeof(VertexShader),&layout.p),"literal native40B declaration");
  std::vector<FixedTerrain::Vertex> vertices(FixedTerrain::Vertices);
  for(unsigned k=0;k<4;++k){Check(FixedTerrain::Build(f.samples,f.baseX+(k&1)*64,f.baseZ+(k>>1)*64,&vertices[0],vertices.size()),"real fixed patch");Buffer(&vertices[0],unsigned(vertices.size()*sizeof(vertices[0])),D3D11_BIND_VERTEX_BUFFER,vb[k]);}
  unsigned short indices[FixedTerrain::IndexCount];Check(FixedTerrain::Indices(indices,FixedTerrain::IndexCount),"literal native strip");Buffer(indices,sizeof(indices),D3D11_BIND_INDEX_BUFFER,ib);Buffer(NULL,16,D3D11_BIND_CONSTANT_BUFFER,cb);
  D3D11_TEXTURE2D_DESC td={};td.Width=td.Height=256;td.ArraySize=td.MipLevels=td.SampleDesc.Count=1;td.Format=DXGI_FORMAT_R32G32B32A32_FLOAT;td.BindFlags=D3D11_BIND_RENDER_TARGET;Hr(d.p->CreateTexture2D(&td,NULL,&target.p),"height-normal target");Hr(d.p->CreateRenderTargetView(target.p,NULL,&rt.p),"target view");td.Usage=D3D11_USAGE_STAGING;td.BindFlags=0;td.CPUAccessFlags=D3D11_CPU_ACCESS_READ;Hr(d.p->CreateTexture2D(&td,NULL,&staging.p),"owned readback");td.Usage=D3D11_USAGE_DEFAULT;td.CPUAccessFlags=0;td.Format=DXGI_FORMAT_D32_FLOAT;td.BindFlags=D3D11_BIND_DEPTH_STENCIL;Hr(d.p->CreateTexture2D(&td,NULL,&depth.p),"depth");Hr(d.p->CreateDepthStencilView(depth.p,NULL,&dv.p),"depth view");
  D3D11_RASTERIZER_DESC r={};r.FillMode=D3D11_FILL_SOLID;r.CullMode=D3D11_CULL_NONE;r.DepthClipEnable=TRUE;Hr(d.p->CreateRasterizerState(&r,&rs.p),"raster");D3D11_DEPTH_STENCIL_DESC z={};z.DepthEnable=TRUE;z.DepthWriteMask=D3D11_DEPTH_WRITE_MASK_ALL;z.DepthFunc=D3D11_COMPARISON_LESS_EQUAL;Hr(d.p->CreateDepthStencilState(&z,&ds.p),"depth state");
 }
 ~Raster(){if(c.p)c.p->ClearState();}
 void Buffer(const void*data,unsigned bytes,unsigned bind,Com<ID3D11Buffer>&out){D3D11_BUFFER_DESC b={};b.ByteWidth=bytes;b.BindFlags=bind;D3D11_SUBRESOURCE_DATA s={};s.pSysMem=data;Hr(d.p->CreateBuffer(&b,data?&s:NULL,&out.p),"owned mesh/constant buffer");}
 std::vector<Pixel> Render(const View&v){
  Check(_finite(v.x)&&_finite(v.z)&&_finite(v.spacing)&&v.spacing>0,"finite diagnostic view");
  // Entire context belongs to this standalone process. No foreign game state.
  c.p->ClearState();float constants[4]={v.x,v.z,v.spacing*256,0};c.p->UpdateSubresource(cb.p,0,NULL,constants,0,0);
  const float clear[4]={0,0,0,-1};c.p->ClearRenderTargetView(rt.p,clear);c.p->ClearDepthStencilView(dv.p,D3D11_CLEAR_DEPTH,1,0);c.p->OMSetRenderTargets(1,&rt.p,dv.p);c.p->OMSetDepthStencilState(ds.p,0);c.p->RSSetState(rs.p);
  D3D11_VIEWPORT viewport={0,0,256,256,0,1};c.p->RSSetViewports(1,&viewport);c.p->IASetInputLayout(layout.p);c.p->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);c.p->IASetIndexBuffer(ib.p,DXGI_FORMAT_R16_UINT,0);c.p->VSSetShader(vs.p,NULL,0);c.p->PSSetShader(ps.p,NULL,0);c.p->VSSetConstantBuffers(0,1,&cb.p);
  const UINT stride=sizeof(FixedTerrain::Vertex),offset=0;
  for(unsigned k=0;k<4;++k){c.p->IASetVertexBuffers(0,1,&vb[k].p,&stride,&offset);c.p->DrawIndexed(FixedTerrain::IndexCount,0,0);}
  c.p->OMSetRenderTargets(0,NULL,NULL);c.p->CopyResource(staging.p,target.p);D3D11_MAPPED_SUBRESOURCE mapped;Hr(c.p->Map(staging.p,0,D3D11_MAP_READ,0,&mapped),"diagnostic readback");std::vector<Pixel>out(256*256);for(unsigned z=0;z<256;++z)std::memcpy(&out[z*256],static_cast<const BYTE*>(mapped.pData)+z*mapped.RowPitch,256*sizeof(Pixel));c.p->Unmap(staging.p,0);return out;
 }
};
static float ExpectedHeight(const Fixture&f,double x,double z){
 double sx=(x+147456)/18,sz=(z+147456)/18;int gx=int(std::floor(sx)),gz=int(std::floor(sz));float u=float(sx-gx),v=float(sz-gz);const FixedTerrain::Samples&s=f.samples;
 float a=FixedTerrain::Height(s,gx,gz),b=FixedTerrain::Height(s,gx+1,gz),q=FixedTerrain::Height(s,gx,gz+1),e=FixedTerrain::Height(s,gx+1,gz+1);
 return u+v<=1?a+u*(b-a)+v*(q-a):e+(1-u)*(q-e)+(1-v)*(b-e);
}
// Independent raster oracle: literal native morph0 input mapping, then the
// anti-diagonal barycentric interpolation. Never call ShaderNormal here.
static FixedTerrain::Vec3 OracleUnit(FixedTerrain::Vec3 n){float l=std::sqrt((n.x*n.x+n.y*n.y)+n.z*n.z);Check(l>1e-8f,"nondegenerate native oracle normal");n.x/=l;n.y/=l;n.z/=l;return n;}
static FixedTerrain::Vec3 OracleVertexNormal(const Fixture&f,const std::vector<FixedTerrain::Vertex>(&v)[4],unsigned x,unsigned z){
 unsigned px=(x-f.baseX)/64,pz=(z-f.baseZ)/64;if(px>1)px=1;if(pz>1)pz=1;
 const FixedTerrain::Vertex&q=v[pz*2+px][(z-f.baseZ-pz*64)*65+x-f.baseX-px*64];FixedTerrain::Vec3 n={q.value[6],q.value[4],q.value[5]};return OracleUnit(n);
}
static FixedTerrain::Vec3 ExpectedNormal(const Fixture&f,const std::vector<FixedTerrain::Vertex>(&vertices)[4],double x,double z){
 double sx=(x+147456)/18,sz=(z+147456)/18;unsigned gx=unsigned(std::floor(sx)),gz=unsigned(std::floor(sz));float u=float(sx-gx),v=float(sz-gz);
 FixedTerrain::Vec3 a=OracleVertexNormal(f,vertices,gx,gz),b=OracleVertexNormal(f,vertices,gx+1,gz),c=OracleVertexNormal(f,vertices,gx,gz+1),d=OracleVertexNormal(f,vertices,gx+1,gz+1),n;
 if(u+v<=1){n.x=a.x+u*(b.x-a.x)+v*(c.x-a.x);n.y=a.y+u*(b.y-a.y)+v*(c.y-a.y);n.z=a.z+u*(b.z-a.z)+v*(c.z-a.z);}
 else {n.x=d.x+(1-u)*(c.x-d.x)+(1-v)*(b.x-d.x);n.y=d.y+(1-u)*(c.y-d.y)+(1-v)*(b.y-d.y);n.z=d.z+(1-u)*(c.z-d.z)+(1-v)*(b.z-d.z);}return OracleUnit(n);
}
static void Validate(const Fixture&f,const View&v,const std::vector<Pixel>&p){
 std::vector<FixedTerrain::Vertex> reference[4];for(unsigned k=0;k<4;++k){reference[k].resize(FixedTerrain::Vertices);Check(FixedTerrain::Build(f.samples,f.baseX+(k&1)*64,f.baseZ+(k>>1)*64,&reference[k][0],reference[k].size()),"independent normal-oracle packet");}
 for(unsigned z=0;z<256;++z)for(unsigned x=0;x<256;++x){const Pixel&q=p[z*256+x];double wx=v.x+(double(x)-127.5)*v.spacing,wz=v.z+(double(z)-127.5)*v.spacing;float expected=ExpectedHeight(f,wx,wz);Check(std::fabs(q.height-expected)<.003f,"native anti-diagonal interpolated height");float l=0;for(unsigned k=0;k<3;++k){Check(_finite(q.normal[k])!=0,"finite normal");l+=q.normal[k]*q.normal[k];}Check(std::fabs(l-1)<2e-6f&&q.normal[1]>0,"unit upward geometric normal");FixedTerrain::Vec3 want=ExpectedNormal(f,reference,wx,wz);const float n[3]={want.x,want.y,want.z};for(unsigned k=0;k<3;++k)Check(std::fabs(q.normal[k]-n[k])<5e-5f,"native morph0 interpolated normal");}
}
static void Compare(const Pixel&a,const Pixel&b){Check(std::fabs(a.height-b.height)<.003f,"same world height independent of window");for(unsigned k=0;k<3;++k)Check(std::fabs(a.normal[k]-b.normal[k])<5e-5f,"same world normal independent of window");}
int main(int argc,char**argv){try{Check(argc==2,"one exact height fixture argument");Fixture f(argv[1]);Raster r(f);float x=-147456+float((f.baseX+64)*18),z=-147456+float((f.baseZ+64)*18);View a={x,z,3},b={x+192,z+192,3},coarse={x,z,9};std::vector<Pixel>pa=r.Render(a),pb=r.Render(b),pc=r.Render(coarse);Validate(f,a,pa);Validate(f,b,pb);Validate(f,coarse,pc);
 for(unsigned z=64;z<256;++z)for(unsigned x=64;x<256;++x)Compare(pa[z*256+x],pb[(z-64)*256+x-64]);
 for(unsigned z=85;z<=170;++z)for(unsigned x=85;x<=170;++x)Compare(pc[z*256+x],pa[(3*z-255)*256+3*x-255]);
 std::vector<Pixel>repeat=r.Render(a);Check(std::memcmp(&repeat[0],&pa[0],pa.size()*sizeof(Pixel))==0,"repeated geometry capture identical");std::printf("PASS %u owned fixed-terrain WARP checks;real4patches;3m-shift and9m shared samples\n",checks);return 0;
 }catch(const std::exception&e){std::fprintf(stderr,"FAIL %s\n",e.what());return 1;}catch(...){return 2;}}
