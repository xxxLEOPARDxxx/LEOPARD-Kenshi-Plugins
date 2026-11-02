// Included inside the capture namespace. The worker touches only copied pixels
// and owned cache files, never Ogre, the world, the camera or the log service.
#include "AtlasCapturePolicy.h"
#include "AtlasPixels.h"
#include "../../../GlobalSatelliteMap/atlas-cache/Gst2Writer.h"
struct AtlasRun {
 struct Pixels {
  std::vector<unsigned> packed,normal;
  std::vector<float> depth;
  AtlasCapture::Camera camera;AtlasCapture::Block block;
  Gst2::Writer* writer;std::string controlPath;bool ok;unsigned valid,invalid;char error[256];
  Pixels():writer(NULL),ok(false),valid(0),invalid(0){error[0]=0;}
 };
 bool active,cancelled,quarantined,control;unsigned index,limit;void* world;
 std::string directory;HANDLE thread;Pixels* pixels;
 Gst2::Win32File* file;Gst2::Writer* writer;
 AtlasRun():active(false),cancelled(false),quarantined(false),control(false),index(0),limit(256),world(NULL),thread(NULL),pixels(NULL),file(NULL),writer(NULL){}
 bool Block(AtlasCapture::Block& out)const{return control?AtlasCapture::OverlapControl(out):AtlasCapture::Select(index,out);}
 static std::wstring Wide(const std::string& text){
  int n=MultiByteToWideChar(CP_ACP,0,text.c_str(),-1,NULL,0);if(n<=1)throw std::runtime_error("atlas output path conversion failed");
  std::vector<wchar_t> p(n);if(MultiByteToWideChar(CP_ACP,0,text.c_str(),-1,&p[0],n)!=n)throw std::runtime_error("atlas output path conversion failed");return &p[0];
 }
 void CloseFile(){delete writer;writer=NULL;bool closed=!file||file->Close();delete file;file=NULL;if(!closed){quarantined=true;active=false;throw std::runtime_error("atlas file close failed; job quarantined");}}
 void Begin(const std::string& folder,unsigned token,void* identity,unsigned blocks=256,bool overlapControl=false){
  if(active||thread||pixels||writer||file||quarantined||!identity||!token||(blocks!=1&&blocks!=16&&blocks!=256))throw std::runtime_error("atlas job already owns resources/invalid identity");
  const unsigned __int64 needed=overlapControl?128ULL*1024*1024:Gst2::FileBytes()*16+1024ULL*1024*1024;
  ULARGE_INTEGER available={0};if(!GetDiskFreeSpaceExA(folder.c_str(),&available,NULL,NULL)||available.QuadPart<needed)
   throw std::runtime_error(overlapControl?"overlap control needs128MiB free output space":"atlas needs23GiB free output space including reserve");
  std::ostringstream path;path<<folder<<(overlapControl?"overlap-":"atlas-")<<token;
  if(!CreateDirectoryA(path.str().c_str(),NULL))throw std::runtime_error("atlas output directory must be new; existing data never overwritten");
  directory=path.str();index=0;limit=overlapControl?1:blocks;control=overlapControl;cancelled=false;world=identity;active=true;
 }
 void PrepareFile(){
  if(control)return;
  if(writer)return;
  AtlasCapture::Block b;if(!active||!AtlasCapture::Select(index,b)||index%16)throw std::runtime_error("atlas primary file cursor invalid");
  std::ostringstream path;path<<directory<<"/tile_"<<b.tileX<<'_'<<b.tileZ<<".gst";
  file=new Gst2::Win32File();writer=new Gst2::Writer();
  if(!file->OpenNew(Wide(path.str()).c_str())||!writer->Begin(*file,b.tileX,b.tileZ,true))throw std::runtime_error("atlas output create/prepare failed");
 }
 static DWORD WINAPI WritePixels(void* value){
  Pixels* p=static_cast<Pixels*>(value);
  try{
   const unsigned count=2048u*2048u;
   if((!p->writer&&p->controlPath.empty())||p->packed.size()!=count||p->normal.size()!=count||p->depth.size()!=count)throw std::runtime_error("atlas copied pixel bounds differ");
   for(unsigned z=0;z<2048;++z)for(unsigned x=0;x<2048;++x){
    unsigned i=z*2048+x,n=p->normal[i];float h=0;
    bool valid=AtlasCapture::Height(p->camera,2048,x,z,p->depth[i],h);
    if(valid)++p->valid;else ++p->invalid;
    p->normal[i]=((n>>16)&255)|(n&0xff00)|((n&255)<<16)|(valid?0xff000000u:0);p->depth[i]=valid?h:0;
   }
   // This whole-terrain footprint is expected to cover every pixel. Do not turn
   // bad radial data, clear buffers or NaNs into a seemingly complete black tile.
   if(p->invalid||p->valid!=count)throw std::runtime_error("atlas geometry has invalid/nonfinite/missing radial samples");
   if(!p->controlPath.empty()){
    // Compact evidence only: raw packed material, normalized RGBA normal/coverage,
    // and world-height R32. No sparse 8192 tile or mip/index/cache publication.
    unsigned char header[32]={0};memcpy(header,"GSOV1\r\n",8);
    unsigned side=2048;float minX=p->block.centerX-p->block.span*.5f,minZ=p->block.centerZ-p->block.span*.5f,step=9;
    memcpy(header+8,&side,4);memcpy(header+12,&minX,4);memcpy(header+16,&minZ,4);memcpy(header+20,&step,4);
    FILE* output=NULL;if(fopen_s(&output,p->controlPath.c_str(),"wb")||!output)throw std::runtime_error("overlap evidence create failed");
    bool ok=fwrite(header,1,32,output)==32;
    ok=fwrite(&p->packed[0],4,count,output)==count&&ok;
    ok=fwrite(&p->normal[0],4,count,output)==count&&ok;
    ok=fwrite(&p->depth[0],4,count,output)==count&&ok;
    const bool closed=fclose(output)==0;
    if(!ok||!closed)throw std::runtime_error("overlap evidence incomplete");
    p->ok=true;return 0;
   }
   {std::vector<unsigned> material(count*2,0);for(unsigned i=0;i<count;++i)material[i*2]=p->packed[i];
    if(!p->writer->WriteBlock(0,p->block.x,p->block.z,&material[0],Gst2::BlockBytes(0),Gst2::BlockPitch(0)))throw std::runtime_error("atlas raw material write failed");}
   std::vector<unsigned>().swap(p->packed);
   if(!p->writer->WriteBlock(1,p->block.x,p->block.z,&p->normal[0],Gst2::BlockBytes(1),Gst2::BlockPitch(1)))throw std::runtime_error("atlas normal/coverage write failed");
   if(!p->writer->WriteBlock(2,p->block.x,p->block.z,&p->depth[0],Gst2::BlockBytes(2),Gst2::BlockPitch(2)))throw std::runtime_error("atlas worldheight write failed");
   p->ok=true;
  }catch(const std::exception& e){_snprintf_s(p->error,sizeof(p->error),_TRUNCATE,"%s; valid=%u invalid=%u",e.what(),p->valid,p->invalid);}
   catch(...){strcpy_s(p->error,sizeof(p->error),"atlas file worker failed");}
  return 0;
 }
 void Launch(const Ogre::CompositorChannel& gb,Ogre::Camera* camera){
  if(!active||thread||pixels||Side!=2048||gb.textures.size()!=3||!camera)throw std::runtime_error("atlas readback state invalid");
  PrepareFile();Pixels* p=new Pixels();pixels=p;
  if(!Block(p->block))throw std::runtime_error("atlas block cursor overflow");p->writer=writer;
  if(control)p->controlPath=directory+"/control.bin";
  Ogre::Quaternion q=camera->getDerivedOrientation();Ogre::Vector3 right=q*Ogre::Vector3::UNIT_X,up=q*Ogre::Vector3::UNIT_Y,back=q*Ogre::Vector3::UNIT_Z;
  p->camera.width=camera->getOrthoWindowWidth();p->camera.height=camera->getOrthoWindowHeight();p->camera.farClip=camera->getFarClipDistance();
  p->camera.y=camera->getDerivedPosition().y;p->camera.rightY=right.y;p->camera.upY=up.y;p->camera.backY=back.y;
  p->packed.resize(2048u*2048u);p->normal.resize(2048u*2048u);p->depth.resize(2048u*2048u);
  Ogre::PixelBox material(2048,2048,1,Ogre::PF_A8R8G8B8,&p->packed[0]);gb.textures[0]->getBuffer()->blitToMemory(material);
  Ogre::PixelBox normal(2048,2048,1,Ogre::PF_A8R8G8B8,&p->normal[0]);gb.textures[1]->getBuffer()->blitToMemory(normal);
  Ogre::PixelBox depth(2048,2048,1,Ogre::PF_FLOAT32_R,&p->depth[0]);gb.textures[2]->getBuffer()->blitToMemory(depth);
  thread=CreateThread(NULL,0,WritePixels,p,0,NULL);if(!thread)throw std::runtime_error("atlas file worker could not start");
 }
 void Cancel(){cancelled=true;}
 bool Poll(){
  if(quarantined)return false;
  if(thread){
   DWORD result=WaitForSingleObject(thread,0);if(result==WAIT_TIMEOUT)return false;
   if(result!=WAIT_OBJECT_0){quarantined=true;active=false;throw std::runtime_error("atlas worker wait failed; resources retained, no retry");}
   if(!CloseHandle(thread)){quarantined=true;active=false;throw std::runtime_error("atlas worker close failed; retained without retry");}
   thread=NULL;bool ok=pixels->ok;std::string error=pixels->error;delete pixels;pixels=NULL;
   if(!ok){cancelled=true;active=false;CloseFile();throw std::runtime_error(error);}
   ++index;
   if(index%16==0){if(!writer->BaseComplete())throw std::runtime_error("atlas primary has missing blocks");CloseFile();}
   std::ostringstream progress;progress<<(control?"overlap control progress=":"atlas base progress=")<<index<<'/'<<limit<<" output="<<directory;Log(progress.str());
  }
  // Failed readback/CreateThread leaves no running worker, so copied storage is safe to reclaim.
  if(cancelled){delete pixels;pixels=NULL;CloseFile();active=false;return false;}
  if(index==limit){CloseFile();active=false;Log(control?"overlap control complete: one2048 frame,48MiB evidence; installed atlas untouched":limit==256?"atlas base complete:16 genuine8192 tiles; offline decode/mips/finalization pending":"atlas bounded control complete; partial outputs are not a ready atlas");return false;}
  return active;
 }
} atlasRun;
