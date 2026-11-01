#pragma once
#include <Windows.h>
#include <objidl.h>
#pragma warning(push)
#pragma warning(disable:4005) // SDK 7.1 intsafe.h repeats VC10 stdint limit macros.
#include <wincodec.h>
#pragma warning(pop)
#include <vector>
#include <string>
#include <cstring>
#include "SavePreviewPolicy.h"
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"oleaut32.lib")

namespace SavePreviewCodec
{
    struct FileHandle
    {
        HANDLE value;explicit FileHandle(HANDLE h):value(h){}~FileHandle(){if(value!=INVALID_HANDLE_VALUE)CloseHandle(value);}
    private:FileHandle(const FileHandle&);FileHandle& operator=(const FileHandle&);
    };
    inline std::wstring Wide(const std::string& value)
    {
        if(value.empty())return std::wstring();
        UINT cp=CP_UTF8;int count=MultiByteToWideChar(cp,MB_ERR_INVALID_CHARS,value.c_str(),(int)value.size(),NULL,0);
        if(!count){cp=CP_ACP;count=MultiByteToWideChar(cp,0,value.c_str(),(int)value.size(),NULL,0);}
        if(!count)return std::wstring();std::wstring result(count,L'\0');
        MultiByteToWideChar(cp,0,value.c_str(),(int)value.size(),&result[0],count);return result;
    }
    inline std::string Utf8(const std::wstring& value)
    {
        if(value.empty())return std::string();
        int count=WideCharToMultiByte(CP_UTF8,0,value.c_str(),(int)value.size(),NULL,0,NULL,NULL);
        std::string result(count,'\0');if(count)WideCharToMultiByte(CP_UTF8,0,value.c_str(),(int)value.size(),&result[0],count,NULL,NULL);return result;
    }
    template<class T> struct Com
    {
        T* p;Com():p(NULL){}~Com(){if(p)p->Release();}
        T** Out(){return &p;}T* operator->()const{return p;}operator T*()const{return p;}
    private:Com(const Com&);Com& operator=(const Com&);
    };
    struct Apartment
    {
        HRESULT result;Apartment():result(CoInitializeEx(NULL,COINIT_MULTITHREADED)){}
        ~Apartment(){if(SUCCEEDED(result))CoUninitialize();}
        bool Ready()const{return SUCCEEDED(result)||result==RPC_E_CHANGED_MODE;}
    };
    // The native main-file revision binds the optional file to an overwrite.
    // No absolute path is embedded: copying a complete save carries its preview.
#pragma pack(push,1)
    struct Header
    {
        char magic[8];DWORD version,timeLow,timeHigh,sizeLow,sizeHigh,width,height,encodedBytes;
    };
#pragma pack(pop)
    inline bool Stamp(const std::string& directory,Header& header)
    {
        WIN32_FILE_ATTRIBUTE_DATA d;
        if(!GetFileAttributesExW((Wide(directory)+L"/quick.save").c_str(),GetFileExInfoStandard,&d)||
           (d.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))return false;
        header.timeLow=d.ftLastWriteTime.dwLowDateTime;header.timeHigh=d.ftLastWriteTime.dwHighDateTime;
        header.sizeLow=d.nFileSizeLow;header.sizeHigh=d.nFileSizeHigh;return true;
    }
    inline bool SameRevision(const Header& a,const Header& b)
    {return a.timeLow==b.timeLow&&a.timeHigh==b.timeHigh&&a.sizeLow==b.sizeLow&&a.sizeHigh==b.sizeHigh;}
    inline bool Valid(const Header& h,unsigned fileBytes)
    {
        return !memcmp(h.magic,"UITPREV1",8)&&h.version==1&&
            SavePreviewPolicy::ValidDimensions(h.width,h.height)&&
            SavePreviewPolicy::ValidEncodedSize(h.encodedBytes)&&
            fileBytes==sizeof(Header)+h.encodedBytes;
    }
    inline bool Encode(const std::vector<unsigned char>& bgr,unsigned w,unsigned h,std::vector<unsigned char>& result)
    {
        if(!SavePreviewPolicy::ValidDimensions(w,h)||bgr.size()!=size_t(w)*h*3)return false;
        Apartment apartment;if(!apartment.Ready())return false;
        Com<IWICImagingFactory> factory;Com<IWICBitmapEncoder> encoder;Com<IWICBitmapFrameEncode> frame;
        Com<IPropertyBag2> properties;Com<IStream> stream;
        if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,NULL,CLSCTX_INPROC_SERVER,IID_IWICImagingFactory,(void**)factory.Out()))||
           FAILED(CreateStreamOnHGlobal(NULL,TRUE,stream.Out()))||
           FAILED(factory->CreateEncoder(GUID_ContainerFormatPng,NULL,encoder.Out()))||
           FAILED(encoder->Initialize(stream,WICBitmapEncoderNoCache))||
           FAILED(encoder->CreateNewFrame(frame.Out(),properties.Out())))return false;
        if(FAILED(frame->Initialize(properties))||
           FAILED(frame->SetSize(w,h)))return false;
        WICPixelFormatGUID format=GUID_WICPixelFormat24bppBGR;
        if(FAILED(frame->SetPixelFormat(&format))||!IsEqualGUID(format,GUID_WICPixelFormat24bppBGR)||
           FAILED(frame->WritePixels(h,w*3,(UINT)bgr.size(),const_cast<BYTE*>(&bgr[0])))||
           FAILED(frame->Commit())||FAILED(encoder->Commit()))return false;
        STATSTG stat={};if(FAILED(stream->Stat(&stat,STATFLAG_NONAME))||stat.cbSize.HighPart||
           !SavePreviewPolicy::ValidEncodedSize(stat.cbSize.LowPart))return false;
        result.resize(stat.cbSize.LowPart);LARGE_INTEGER zero={};ULONG read=0;
        return SUCCEEDED(stream->Seek(zero,STREAM_SEEK_SET,NULL))&&
            SUCCEEDED(stream->Read(&result[0],(ULONG)result.size(),&read))&&read==result.size();
    }
    inline bool ResizeBgra(const std::vector<unsigned char>& bgra,unsigned w,unsigned h,
        unsigned targetW,unsigned targetH,std::vector<unsigned char>& result)
    {
        if(!SavePreviewPolicy::AllowedReadback(w,h)||bgra.size()!=size_t(w)*h*4||
           !SavePreviewPolicy::ValidDimensions(targetW,targetH)||targetW>w||targetH>h)return false;
        Apartment apartment;if(!apartment.Ready())return false;
        Com<IWICImagingFactory> factory;Com<IWICBitmap> bitmap;Com<IWICBitmapScaler> scaler;
        Com<IWICFormatConverter> converter;
        if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,NULL,CLSCTX_INPROC_SERVER,IID_IWICImagingFactory,(void**)factory.Out()))||
           // Backbuffer alpha is not image transparency. BGRx preserves its
           // RGB even when unused alpha bytes are zero, before Fant filtering.
           FAILED(factory->CreateBitmapFromMemory(w,h,GUID_WICPixelFormat32bppBGR,w*4,(UINT)bgra.size(),const_cast<BYTE*>(&bgra[0]),bitmap.Out()))||
           FAILED(factory->CreateBitmapScaler(scaler.Out()))||
           FAILED(scaler->Initialize(bitmap,targetW,targetH,WICBitmapInterpolationModeFant))||
           FAILED(factory->CreateFormatConverter(converter.Out()))||
           FAILED(converter->Initialize(scaler,GUID_WICPixelFormat24bppBGR,WICBitmapDitherTypeNone,NULL,0,WICBitmapPaletteTypeCustom)))return false;
        std::vector<unsigned char> filtered(size_t(targetW)*targetH*3);
        if(FAILED(converter->CopyPixels(NULL,targetW*3,(UINT)filtered.size(),&filtered[0])))return false;
        result.swap(filtered);return true;
    }
    inline bool Decode(const unsigned char* bytes,unsigned count,unsigned width,unsigned height,std::vector<unsigned char>& rgba)
    {
        if(!bytes||!SavePreviewPolicy::ValidEncodedSize(count)||!SavePreviewPolicy::ValidDimensions(width,height))return false;
        Apartment apartment;if(!apartment.Ready())return false;
        Com<IWICImagingFactory> factory;Com<IWICStream> stream;Com<IWICBitmapDecoder> decoder;
        Com<IWICBitmapFrameDecode> frame;Com<IWICFormatConverter> converter;
        if(FAILED(CoCreateInstance(CLSID_WICImagingFactory,NULL,CLSCTX_INPROC_SERVER,IID_IWICImagingFactory,(void**)factory.Out()))||
           FAILED(factory->CreateStream(stream.Out()))||FAILED(stream->InitializeFromMemory(const_cast<BYTE*>(bytes),count))||
           FAILED(factory->CreateDecoderFromStream(stream,NULL,WICDecodeMetadataCacheOnDemand,decoder.Out()))||
           FAILED(decoder->GetFrame(0,frame.Out())))return false;
        UINT w=0,h=0;if(FAILED(frame->GetSize(&w,&h))||w!=width||h!=height)return false;
        if(FAILED(factory->CreateFormatConverter(converter.Out()))||
           FAILED(converter->Initialize(frame,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,NULL,0,WICBitmapPaletteTypeCustom)))return false;
        rgba.resize(size_t(w)*h*4);
        return SUCCEEDED(converter->CopyPixels(NULL,w*4,(UINT)rgba.size(),&rgba[0]));
    }
    inline bool Read(const std::string& directory,Header& header,std::vector<unsigned char>& rgba,std::string* error=NULL)
    {
        HANDLE file=CreateFileW((Wide(directory)+L"/uitweaks-preview.bin").c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_DELETE,
                                NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
        if(file==INVALID_HANDLE_VALUE){DWORD e=GetLastError();if(error&&e!=ERROR_FILE_NOT_FOUND&&e!=ERROR_PATH_NOT_FOUND)*error="cannot read preview file";return false;}
        FileHandle owned(file);
        LARGE_INTEGER size={};DWORD n=0;bool ok=GetFileSizeEx(file,&size)!=FALSE&&size.HighPart==0&&
            size.LowPart>=sizeof(Header)&&size.LowPart<=sizeof(Header)+SavePreviewPolicy::MaxEncodedBytes&&
            ReadFile(file,&header,sizeof(header),&n,NULL)&&n==sizeof(header)&&Valid(header,size.LowPart);
        if(!ok&&error)*error="invalid save preview format";
        Header current={};ok=ok&&Stamp(directory,current)&&SameRevision(header,current);
        std::vector<unsigned char> bytes;
        if(ok){bytes.resize(header.encodedBytes);ok=ReadFile(file,&bytes[0],header.encodedBytes,&n,NULL)&&n==header.encodedBytes;if(!ok&&error)*error="cannot read preview image";}
        if(!ok)return false;
        ok=Decode(&bytes[0],(unsigned)bytes.size(),header.width,header.height,rgba);
        if(!ok&&error)*error="invalid save preview image";
        return ok;
    }
}
