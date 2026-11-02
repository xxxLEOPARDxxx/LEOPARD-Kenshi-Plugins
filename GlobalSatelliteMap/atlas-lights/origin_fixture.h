#pragma once
// Test-only exact native getter copy, reading synthetic owned SceneManager/root data.
class OriginFixture {
    void*code_;unsigned __int64 scene_[0x600/8],root0_[0x60/8],root1_[0x60/8];float position0_[12],position1_[12];
    OriginFixture(const OriginFixture&);OriginFixture&operator=(const OriginFixture&);
public:
    typedef void*(__fastcall*Getter)(const void*,float*);Getter function;
    OriginFixture(const char*path):code_(NULL),function(NULL){
        FILE*f=NULL;Check(!fopen_s(&f,path,"rb")&&f,"private native origin fixture open");BYTE bytes[132];Check(fread(bytes,1,sizeof(bytes),f)==sizeof(bytes)&&fgetc(f)==EOF,"private native origin fixture exact length");fclose(f);
        code_=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);Check(code_!=NULL,"private native origin fixture allocation");std::memcpy(code_,bytes,sizeof(bytes));DWORD old;Check(VirtualProtect(code_,4096,PAGE_EXECUTE_READ,&old)!=FALSE&&FlushInstructionCache(GetCurrentProcess(),code_,sizeof(bytes)),"private native origin fixture RX transition");function=reinterpret_cast<Getter>(code_);
        std::memset(scene_,0,sizeof(scene_));std::memset(root0_,0,sizeof(root0_));std::memset(root1_,0,sizeof(root1_));std::memset(position0_,0,sizeof(position0_));std::memset(position1_,0,sizeof(position1_));
        const void*r0=root0_,*r1=root1_,*p0=position0_,*p1=position1_;std::memcpy(reinterpret_cast<BYTE*>(scene_)+0x5f0,&r0,8);std::memcpy(reinterpret_cast<BYTE*>(scene_)+0x5f8,&r1,8);std::memcpy(reinterpret_cast<BYTE*>(root0_)+0x50,&p0,8);std::memcpy(reinterpret_cast<BYTE*>(root1_)+0x50,&p1,8);reinterpret_cast<BYTE*>(root0_)[0x38]=2;
        Set(1024,12,-2048);position1_[0]=-900;position1_[4]=-800;position1_[8]=-700;
    }
    ~OriginFixture(){if(code_)VirtualFree(code_,0,MEM_RELEASE);}
    const void*Scene()const{return scene_;}
    void Set(float x,float y,float z){position0_[2]=-x;position0_[6]=-y;position0_[10]=-z;}
};
