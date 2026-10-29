// Temporary diagnostics; not part of a public mod package.
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <Psapi.h>
#include <cstdio>
#include <cstring>
#include <string>
#include <Debug.h>
#include <core/Functions.h>
#include <mygui/MyGUI_ILayer.h>
#include <mygui/MyGUI_ILayerNode.h>
#pragma comment(lib,"psapi.lib")
namespace {
struct Counts {unsigned __int64 visits,textured,nonzero,attempts,forced,vertices,entries,dirty;};
struct Layer {char name[48];Counts n;};
struct Caller {void* via;void* from;unsigned count;};
Layer layers[32];Caller callers[128];unsigned layerCount=1,callerCount=0,dropped=0;
unsigned frames=0,forced=0,samples=0,sequence=0,budget=0;
DWORD reportAt=0,pollAt=0,budgetAt=0,owner=0;volatile LONG foreign=0;
bool ready=false,enabled=false,trace=false;char ini[MAX_PATH],phase[64]="startup";
char *guiLo=NULL,*guiHi=NULL,*selfLo=NULL,*selfHi=NULL;
__declspec(thread) int current=-1;
__declspec(thread) unsigned depth=0;
void (*oldLM)(void*,void*,bool)=NULL;void (*oldShared)(void*,void*,bool)=NULL;
void (*oldOver)(void*,void*,bool)=NULL;void (*oldRI)(void*,void*,bool)=NULL;void (*oldOOD)(void*,void*)=NULL;
void Reset(DWORD now){for(unsigned i=0;i<32;++i)ZeroMemory(&layers[i].n,sizeof(Counts));ZeroMemory(callers,sizeof(callers));callerCount=dropped=samples=frames=forced=0;InterlockedExchange(&foreign,0);reportAt=now;}
unsigned Index(const MyGUI::ILayer* layer){if(!layer)return 0;const char* name=layer->getName().c_str();for(unsigned i=1;i<layerCount;++i)if(strcmp(layers[i].name,name)==0)return i;if(layerCount==32)return 0;unsigned i=layerCount++;strncpy_s(layers[i].name,name,_TRUNCATE);return i;}
unsigned NodeIndex(void* node){__try{return Index(static_cast<MyGUI::ILayerNode*>(node)->getLayer());}__except(EXCEPTION_EXECUTE_HANDLER){return 0;}}
std::string Where(void* at){HMODULE m=NULL;char path[MAX_PATH]="?",line[MAX_PATH+40];if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCSTR)at,&m))return "?";GetModuleFileNameA(m,path,sizeof(path));const char* p=strrchr(path,'\\');_snprintf_s(line,_TRUNCATE,"%s+0x%llX",p?p+1:path,(unsigned __int64)((char*)at-(char*)m));return line;}
void Report(DWORD now){char line[512];const double n=frames?frames:1;
_snprintf_s(line,_TRUNCATE,"UILite phase=%s ms=%u frames=%u forced=%u trace=%d samples=%u foreign=%ld dropped=%u",phase,now-reportAt,frames,forced,trace?1:0,samples,foreign,dropped);DebugLog(line);
for(unsigned i=0;i<layerCount;++i){const Counts& c=layers[i].n;if(!c.visits&&!c.dirty)continue;_snprintf_s(line,_TRUNCATE,"UILite layer=%s visits=%.2f tex=%.2f nonzero=%.2f attempts=%.2f forced=%.2f verts=%.0f entries=%.1f dirty=%.2f",layers[i].name,c.visits/n,c.textured/n,c.nonzero/n,c.attempts/n,c.forced/n,c.vertices/n,c.entries/n,c.dirty/n);DebugLog(line);}
bool used[128]={};for(unsigned rank=0;rank<16;++rank){unsigned best=128;for(unsigned i=0;i<callerCount;++i)if(!used[i]&&(best==128||callers[i].count>callers[best].count))best=i;if(best==128)break;used[best]=true;_snprintf_s(line,_TRUNCATE,"UILite Middle samples=%u via=%s from=%s",callers[best].count,Where(callers[best].via).c_str(),Where(callers[best].from).c_str());DebugLog(line);}Reset(now);}
void Poll(DWORD now){if(pollAt&&now-pollAt<1000)return;pollAt=now;bool e=GetPrivateProfileIntA("Probe","Enabled",0,ini)!=0,t=GetPrivateProfileIntA("Probe","TraceMiddle",0,ini)!=0;char p[64];GetPrivateProfileStringA("Probe","Phase","unlabelled",p,sizeof(p),ini);if(e!=enabled||t!=trace||strcmp(p,phase)){if(enabled&&frames)Report(now);enabled=e;trace=t;strcpy_s(phase,p);Reset(now);}}
void LM(void* self,void* target,bool update){if(!ready){oldLM(self,target,update);return;}DWORD tid=GetCurrentThreadId();if(!owner)owner=tid;if(tid!=owner){oldLM(self,target,update);return;}DWORD now=GetTickCount();if(!depth)Poll(now);if(!enabled){oldLM(self,target,update);return;}int previous=current;current=0;++depth;__try{++frames;if(update)++forced;oldLM(self,target,update);}__finally{--depth;current=previous;}if(!depth&&now-reportAt>=10000)Report(now);}
void Shared(void* self,void* target,bool update){if(!enabled||!depth){oldShared(self,target,update);return;}int previous=current;current=Index((MyGUI::ILayer*)self);__try{oldShared(self,target,update);}__finally{current=previous;}}
void Over(void* self,void* target,bool update){if(!enabled||!depth){oldOver(self,target,update);return;}int previous=current;current=Index((MyGUI::ILayer*)self);__try{oldOver(self,target,update);}__finally{current=previous;}}
void RI(void* self,void* target,bool update){if(!enabled||current<0){oldRI(self,target,update);return;}const char* p=(const char*)self;bool tex=*(void* const*)(p+8)!=NULL,dirty=*(const bool*)(p+0x18);oldRI(self,target,update);Counts& c=layers[current].n;++c.visits;if(tex){++c.textured;size_t v=*(const size_t*)(p+0x40);if(v)++c.nonzero;if(dirty||update){++c.attempts;if(!dirty)++c.forced;c.vertices+=v;const char* a=*(char* const*)(p+0x20);const char* b=*(char* const*)(p+0x28);if(b>=a&&size_t(b-a)<=16*1024*1024)c.entries+=size_t(b-a)/16;}}}
void OOD(void* self,void* item){if(enabled){if(GetCurrentThreadId()!=owner)InterlockedIncrement(&foreign);else{unsigned i=NodeIndex(self);++layers[i].n.dirty;if(trace&&strcmp(layers[i].name,"Middle")==0&&((++sequence&63)==0)){DWORD now=GetTickCount();if(now-budgetAt>=1000){budgetAt=now;budget=0;}if(budget<64){++budget;++samples;void* stack[16];USHORT count=CaptureStackBackTrace(1,16,stack,NULL);void *via=NULL,*from=NULL;for(USHORT j=0;j<count;++j){char* at=(char*)stack[j];if(at>=selfLo&&at<selfHi)continue;if(at>=guiLo&&at<guiHi)via=at;else{from=at;break;}}unsigned k=0;for(;k<callerCount;++k)if(callers[k].via==via&&callers[k].from==from)break;if(k<128){if(k==callerCount){++callerCount;callers[k].via=via;callers[k].from=from;}++callers[k].count;}else ++dropped;}}}}oldOOD(self,item);}
void* Export(HMODULE lib,const char* name){unsigned char* p=(unsigned char*)GetProcAddress(lib,name);for(unsigned i=0;p&&p[0]==0xe9&&i<4;++i)p=p+5+*(int*)(p+1);return p;}
bool Hook(HMODULE lib,const char* name,void* fn,void** old){void* at=Export(lib,name);if(!at||KenshiLib::AddHook(at,fn,old)!=KenshiLib::SUCCESS){ErrorLog(std::string("UILite missing hook: ")+name);return false;}return true;}
bool Leaf(HMODULE lib,const char* name,const unsigned char* pattern,unsigned n){void* p=Export(lib,name);return p&&memcmp(p,pattern,n)==0;}
}
__declspec(dllexport) void startPlugin(){HMODULE lib=GetModuleHandleA("MyGUIEngine_x64.dll"),self=NULL;MODULEINFO a,b;if(!lib||!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,(LPCSTR)&startPlugin,&self)||!GetModuleInformation(GetCurrentProcess(),lib,&a,sizeof(a))||!GetModuleInformation(GetCurrentProcess(),self,&b,sizeof(b))){ErrorLog("UILite module discovery failed");return;}guiLo=(char*)a.lpBaseOfDll;guiHi=guiLo+a.SizeOfImage;selfLo=(char*)b.lpBaseOfDll;selfHi=selfLo+b.SizeOfImage;
GetModuleFileNameA(self,ini,sizeof(ini));char* slash=strrchr(ini,'\\');if(!slash)return;strcpy_s(slash+1,sizeof(ini)-(slash+1-ini),"LayerProbeLite.ini");
const unsigned char tex[]={0x48,0x8b,0x41,0x08},dirty[]={0x0f,0xb6,0x41,0x18},vertices[]={0x48,0x8b,0x41,0x40};
if(!Leaf(lib,"?getTexture@RenderItem@MyGUI@@QEAAPEAVITexture@2@XZ",tex,sizeof(tex))||!Leaf(lib,"?isOutOfDate@RenderItem@MyGUI@@QEBA_NXZ",dirty,sizeof(dirty))||!Leaf(lib,"?getVertexCount@RenderItem@MyGUI@@QEBA_KXZ",vertices,sizeof(vertices))){ErrorLog("UILite RenderItem ABI mismatch; no hooks installed");return;}
strcpy_s(layers[0].name,"?");bool ok=true;
ok=Hook(lib,"?renderToTarget@LayerManager@MyGUI@@QEAAXPEAVIRenderTarget@2@_N@Z",(void*)LM,(void**)&oldLM)&&ok;
ok=Hook(lib,"?renderToTarget@SharedLayer@MyGUI@@UEAAXPEAVIRenderTarget@2@_N@Z",(void*)Shared,(void**)&oldShared)&&ok;
ok=Hook(lib,"?renderToTarget@OverlappedLayer@MyGUI@@UEAAXPEAVIRenderTarget@2@_N@Z",(void*)Over,(void**)&oldOver)&&ok;
ok=Hook(lib,"?renderToTarget@RenderItem@MyGUI@@QEAAXPEAVIRenderTarget@2@_N@Z",(void*)RI,(void**)&oldRI)&&ok;
ok=Hook(lib,"?outOfDate@LayerNode@MyGUI@@UEAAXPEAVRenderItem@2@@Z",(void*)OOD,(void**)&oldOOD)&&ok;
ready=ok;DebugLog(ok?"UILite installed: fixed counters, optional Middle trace <=64 stacks/sec":"UILite incomplete hooks: collection disabled");}
