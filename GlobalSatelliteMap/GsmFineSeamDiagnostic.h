// Private, explicit developer request. Never runs a capture just because Debug
// is enabled. The file is checked at most once/second while the map is open.
DWORD seamCheckAt=0,seamLastId=0,seamCaptureId=0;
FineWindows::Key seamKeys[2];unsigned seamShaded=0;
bool FineSeamWrite(const std::wstring&path,const std::string&metadata){
 HANDLE f=CreateFileW(path.c_str(),GENERIC_WRITE,FILE_SHARE_READ,NULL,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,NULL);if(f==INVALID_HANDLE_VALUE)return false;
 const char magic[8]={'G','S','M','S','E','A','M','1'};const DWORD size=DWORD(metadata.size());DWORD written=0;
 bool ok=WriteFile(f,magic,sizeof(magic),&written,NULL)!=FALSE&&written==sizeof(magic);
 if(ok)ok=WriteFile(f,&size,sizeof(size),&written,NULL)!=FALSE&&written==sizeof(size);
 if(ok)ok=WriteFile(f,metadata.data(),size,&written,NULL)!=FALSE&&written==size;
 if(ok)ok=WriteFile(f,seamBlob.Data(0),GsmFineSeamReadback::PayloadBytes,&written,NULL)!=FALSE&&written==GsmFineSeamReadback::PayloadBytes;
 CloseHandle(f);return ok;
}
void FineSeamSave(){
 std::ostringstream name;name<<Folder()<<"fine-seam-"<<seamCaptureId;
 std::ostringstream info;info<<"{\n  \"id\": "<<seamCaptureId<<", \"side\":256, \"format\":\"RGBA8\", \"planes\":2,\n  \"source\":"<<seamKeys[0].stamp.source<<", \"material\":"<<seamKeys[0].stamp.material<<", \"shadeEpoch\":"<<seamShaded<<",\n  \"spacing\":"<<seamKeys[0].spacing<<", \"x\":"<<seamKeys[0].x<<", \"upperZ\":"<<seamKeys[0].z<<", \"lowerZ\":"<<seamKeys[1].z<<"\n}\n";
 const bool ok=FineSeamWrite(GsmAtlas::Wide(name.str()+".bin"),info.str());
 if(ok)Diagnostic(std::string("fine seam final-output readback saved: ")+name.str());else ErrorLog("GlobalSatelliteMap: fine seam diagnostic write failed; existing outputs were not overwritten");
 seamBlob.Clear();seamReadback.Clear();seamCaptureId=0;
}
void FineSeamTick(){
 // Poll a submitted copy independently of visibility/Debug, so closing a menu
 // or disabling diagnostics cannot strand staging resources. No blocking Map.
 if(seamReadback.Active()){
  if(seamReadback.Failed())return;
  const AtlasView::U64 charged=seamReadback.CpuBytes()+seamBlob.OwnedBytes();FineStore::Ledger cpu=FineCpuLedger();
  HRESULT hr=seamReadback.Poll(seamBlob,cpu.other>=charged?cpu.other-charged:AtlasView::CpuCap);
  if(hr==S_OK)FineSeamSave();else if(FAILED(hr)){std::ostringstream why;why<<"GlobalSatelliteMap: fine seam output readback failed hr="<<std::hex<<hr;ErrorLog(why.str());if(!seamReadback.Failed()){seamBlob.Clear();seamReadback.Clear();seamCaptureId=0;}}
  return;
 }
 if(!debug||!FineContext()||finePhase!=4||fineCenter||fineSpeculative||!fineOwner||!fineOwner->Ready()||ProducerGpuBytes()||job||completed)return;
 const DWORD now=GetTickCount();if(now-seamCheckAt<1000)return;seamCheckAt=now;
 const std::wstring path=GsmAtlas::Wide(Folder()+"fine-seam-request.txt");HANDLE f=CreateFileW(path.c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);if(f==INVALID_HANDLE_VALUE)return;
 char line[96]={0};DWORD got=0;const bool read=ReadFile(f,line,sizeof(line)-1,&got,NULL)!=FALSE;CloseHandle(f);if(!read||!got||got>=sizeof(line)-1)return;
 unsigned id=0,spacing=0,x=0,z=0;std::istringstream input(std::string(line,got));if(!(input>>id>>spacing>>x>>z)||!id||id==seamLastId)return;input>>std::ws;if(!input.eof())return;
 FineWindows::Key a={spacing,x,z,fineDemand.view.stamp},b=a;if(z==~0U)return;++b.z;if(!FineWindows::Valid(a)||!FineWindows::Valid(b))return;
 const int ia=FineFind(a),ib=FineFind(b);if(ia<0||ib<0)return;Entry&upper=fineCache[ia].image;Entry&lower=fineCache[ib].image;
 if(!upper.shaded||upper.shaded!=lower.shaded||upper.modelStamp!=lower.modelStamp||upper.localStamp!=lower.localStamp||upper.waterStamp!=lower.waterStamp)return;
 if(!AdmitGpu(2*GsmFineSeamReadback::PayloadBytes))return;
 seamLastId=id;HRESULT hr=seamReadback.Begin(nativeLight.device.p,upper.output.p,lower.output.p,AggregateGpuBytes(),FineCpuLedger().other);
 if(FAILED(hr)){std::ostringstream why;why<<"GlobalSatelliteMap: fine seam request refused hr="<<std::hex<<hr;ErrorLog(why.str());return;}
 seamKeys[0]=a;seamKeys[1]=b;seamShaded=upper.shaded;seamCaptureId=id;Diagnostic("fine seam final-output copy submitted (two256px tiles)");
}
