// Included inside the private caller's namespace, after the checked device and
// context helpers. No filesystem creation/source-byte IO is performed on Main.
SourceReceipts::Scope WorldBuildNow()
{
    SourceReceipts::Scope now=worldBuildScope;now.thread=GetCurrentThreadId();now.world=reinterpret_cast<SourceReceipts::U64>(ou);
    if(InterlockedCompareExchange(&worldFrameLoadGeneration,0,0)!=worldBuildCapturedLoad)now.sourceEpoch^=1ULL<<63;
    return now;
}
void WorldBuildFinished(int result)
{
    if(!result)return;
    if(result==1)
    {
        ++worldBuildDone;
        if(worldBuildDone==worldBuildCount){if(!worldBuild->Complete())throw std::runtime_error("world build final consumer close refused");}
        if(worldBuildDone==worldBuildCount||worldBuildDone%16==0){std::ostringstream s;s<<"world build cohorts="<<worldBuildDone<<"/"<<worldBuildCount<<" windows="<<worldBuild->Windows();Log(s.str());}
        return;
    }
    std::ostringstream s;s<<"world build token="<<worldBuildToken<<" result="<<result<<" hr=0x"<<std::hex<<unsigned(worldBuild->Error())<<std::dec<<" quarantined="<<worldBuild->Quarantined()<<" windows="<<worldBuild->Windows()<<" expected="<<worldBuildCount*4<<" directory="<<worldBuildDirectory<<"; closed private base data only, no mip/index/atlas publication";
    if(result!=2||worldBuild->Windows()!=worldBuildCount*4||worldBuildDone!=worldBuildCount)ErrorLog("SatelliteCaptureProbe: "+s.str());else Log(s.str());
    cleanup=true;
}
void PrepareWorldBuild(unsigned token)
{
    worldBuildCase=GetPrivateProfileIntA("Probe","WorldBuildCase",0,requestIni.c_str());
    if(worldBuildCase>2)throw std::runtime_error("world build case outside explicit0/1/2 control");
    char executable[MAX_PATH]={0};DWORD n=GetModuleFileNameA(NULL,executable,MAX_PATH);if(!n||n>=MAX_PATH)throw std::runtime_error("world build executable path unavailable");
    worldBuildRoot.assign(executable,n);worldBuildRoot=worldBuildRoot.substr(0,worldBuildRoot.find_last_of("/\\"));
    if(worldBuildRoot.size()>=9&&!_stricmp(worldBuildRoot.substr(worldBuildRoot.size()-9).c_str(),"RE_Kenshi"))worldBuildRoot=worldBuildRoot.substr(0,worldBuildRoot.find_last_of("/\\"));
    if(++worldFrameEpoch==0)++worldFrameEpoch;worldBuildCapturedLoad=InterlockedCompareExchange(&worldFrameLoadGeneration,0,0);
    worldBuildScope.thread=GetCurrentThreadId();worldBuildScope.world=reinterpret_cast<SourceReceipts::U64>(ou);worldBuildScope.sourceEpoch=(SourceReceipts::U64(GetCurrentProcessId())<<32)|worldFrameEpoch;worldBuildScope.job=(5ULL<<32)|token;
    worldBuildToken=token;worldBuildCount=worldBuildCase==1?1024:1;worldBuildDone=0;worldBuildStop=false;worldBuildMaterialRegion=64;
    char suffix[100];std::sprintf(suffix,"world-build-%u-%u-%u",GetCurrentProcessId(),token,GetTickCount());std::string parent=Folder();if(!parent.empty())parent.erase(parent.size()-1);worldBuildDirectory=parent+"/"+suffix;
    if(parent.size()>220||worldBuildDirectory.size()>220)throw std::runtime_error("world build output path exceeds fixed capacity");
    wchar_t wideParent[221]={0},wideDirectory[221]={0};
    for(size_t i=0;i<parent.size();++i){if(static_cast<unsigned char>(parent[i])>=128)throw std::runtime_error("world build non-ASCII parent path unsupported");wideParent[i]=static_cast<unsigned char>(parent[i]);}
    for(size_t i=0;i<worldBuildDirectory.size();++i){if(static_cast<unsigned char>(worldBuildDirectory[i])>=128)throw std::runtime_error("world build non-ASCII output path unsupported");wideDirectory[i]=static_cast<unsigned char>(worldBuildDirectory[i]);}
    WorldBuildDisk::Config config;if(!WorldBuildDisk::Configure(config,wideParent,wideDirectory))throw std::runtime_error("world build new output directory configuration refused");
    if(!worldBuild)worldBuild=new NativeWorldBuild;
    WorldWindow::Ledger extra;extra.other=512*1024; // Conservative copied caller/job/material/path/scratch allowance.
    if(!worldBuild->Begin(worldBuildScope,(7ULL<<32)|token,config,extra))throw std::runtime_error("world build producer/writer initialization refused");
}
void PrepareWorldBuildJob()
{
    const unsigned region=worldBuildCase==1?worldBuildDone/16:worldBuildCase==2?2:27,cell=worldBuildCase==1?worldBuildDone%16:worldBuildCase==2?0:10;
    WorldLayout::Placement placement[4];if(!WorldLayout::BaseCell((region%8)*4+cell%4,(region/8)*4+cell/4,placement))throw std::runtime_error("world build exact base placement refused");
    if(worldBuildMaterialRegion!=region)
    {
        PrivateGeography geography;geography.selectedRegion=&SatelliteRegion::Data.regions[region];geography.ParametersRegion(worldBuildRoot,false);
        std::memset(&worldBuildMaterial,0,sizeof(worldBuildMaterial));worldBuildMaterial.source.value=worldBuildScope.sourceEpoch;worldBuildMaterial.source.profile=WorldBank::PinnedBaseSnapshot;worldBuildMaterial.region=region;worldBuildMaterial.generation=worldBuildScope.sourceEpoch^(SourceReceipts::U64(region+1)<<48);
        std::memcpy(worldBuildMaterial.parameters,geography.parameters,sizeof(worldBuildMaterial.parameters));std::memcpy(worldBuildMaterial.distortion,geography.distortion,sizeof(worldBuildMaterial.distortion));worldBuildMaterialRegion=region;
        BYTE parameterDigest[32],distortionDigest[32];
        if(!AtlasView::Digest(worldBuildMaterial.parameters,sizeof(worldBuildMaterial.parameters),parameterDigest)||!AtlasView::Digest(worldBuildMaterial.distortion,sizeof(worldBuildMaterial.distortion),distortionDigest))throw std::runtime_error("world build material digest failed");
        static const char hex[]="0123456789abcdef";char parameterHex[65]={0},distortionHex[65]={0};for(unsigned i=0;i<32;++i){parameterHex[i*2]=hex[parameterDigest[i]>>4];parameterHex[i*2+1]=hex[parameterDigest[i]&15];distortionHex[i*2]=hex[distortionDigest[i]>>4];distortionHex[i*2+1]=hex[distortionDigest[i]&15];}
        std::ostringstream snapshot;snapshot<<"world build material token="<<worldBuildToken<<" region="<<region<<" parameters900="<<parameterHex<<" distortion60="<<distortionHex;Log(snapshot.str());
        if(worldBuildCase==2)
        {
            // One isolated diagnostic only: preserve actual native float bits
            // in bounded lines, so an offline replay can identify the refusal.
            BYTE bytes[3840];std::memcpy(bytes,worldBuildMaterial.parameters,3600);std::memcpy(bytes+3600,worldBuildMaterial.distortion,240);
            for(unsigned at=0;at<sizeof(bytes);at+=32){char value[65]={0};for(unsigned i=0;i<32;++i){value[i*2]=hex[bytes[at+i]>>4];value[i*2+1]=hex[bytes[at+i]&15];}std::ostringstream line;line<<"world build material bytes token="<<worldBuildToken<<" offset="<<at<<" hex="<<value;Log(line.str());}
        }
    }
    WorldCohort::Job&j=worldBuildJob;std::memset(&j,0,sizeof(j));j.scope=worldBuildScope;j.material=worldBuildMaterial;j.count=4;
    j.shaders.vertex.data=CompactMaterial::VertexShader;j.shaders.vertex.size=sizeof(CompactMaterial::VertexShader);j.shaders.partial.data=StagedMaterial::PartialShader;j.shaders.partial.size=sizeof(StagedMaterial::PartialShader);j.shaders.tail.data=StagedMaterial::TailShader;j.shaders.tail.size=sizeof(StagedMaterial::TailShader);
    const std::string height=worldBuildRoot+"/data/newland/land/fullmap.tif";if(height.size()>=259)throw std::runtime_error("world build height path too long");for(size_t i=0;i<height.size();++i){if(static_cast<unsigned char>(height[i])>=128)throw std::runtime_error("world build non-ASCII height path unsupported");j.heightPath[i]=static_cast<unsigned char>(height[i]);}
    for(unsigned n=0;n<4;++n){const WorldLayout::Placement&p=placement[n];WorldRaster::Frame&f=j.frame[n];f.source=j.material.source;f.generation=j.scope.job;f.materialGeneration=j.material.generation;f.region=p.region;f.cell=p.cell;f.side=p.side;f.x=float(p.centreX);f.z=float(p.centreZ);f.spacing=float(p.spacing);f.cameraY=10000;f.farClip=20000;f.waterHeight=-10000;WorldRaster::Grid&g=j.grid[n];g.x=p.gridX;g.z=p.gridZ;g.columns=p.columns;g.rows=p.rows;g.sourceEpoch=j.scope.sourceEpoch;}
    WorldMaterial::Planner planner;if(!planner.Init(j.scope.sourceEpoch)||!planner.Build(region,cell,j.scope.sourceEpoch,0,j.plan))throw std::runtime_error("world build source plan refused");
}
