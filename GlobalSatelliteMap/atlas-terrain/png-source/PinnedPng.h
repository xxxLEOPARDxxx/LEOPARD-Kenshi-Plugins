#pragma once
#include "../world-bank/WorldSourceBank.h"
#include <wincodec.h>
namespace PinnedPng {
inline bool Cancelled(HANDLE event){return event&&WaitForSingleObject(event,0)!=WAIT_TIMEOUT;}
inline unsigned Big32(const BYTE*p){return unsigned(p[0])<<24|unsigned(p[1])<<16|unsigned(p[2])<<8|p[3];}
inline unsigned Side(const WorldBank::RgbaPin&p){if(!p.path||!p.encodedSha)return 0;for(unsigned i=0;i<129;++i){const WorldBank::RgbaPin&q=WorldBank::RgbaPins[i];if(p.bytes==q.bytes&&p.encodedBytes==q.encodedBytes&&!std::memcmp(p.sha,q.sha,32)&&!std::strcmp(p.path,q.path)&&!std::strcmp(p.encodedSha,q.encodedSha))return i>=64&&i<128?2048:1024;}return 0;}
// Create, use and release on one joined worker. No paths, game or native APIs.
class Decoder {
 IWICImagingFactory*factory_;DWORD owner_;bool com_;
 Decoder(const Decoder&);Decoder&operator=(const Decoder&);
public:
 Decoder():factory_(NULL),owner_(0),com_(false){}
 ~Decoder(){Clear();}
 HRESULT Clear(){if(owner_&&owner_!=GetCurrentThreadId())return RPC_E_WRONG_THREAD;if(factory_){factory_->Release();factory_=NULL;}if(com_){CoUninitialize();com_=false;}owner_=0;return S_OK;}
 HRESULT Init(){if(factory_||owner_)return E_UNEXPECTED;HRESULT hr=CoInitializeEx(NULL,COINIT_MULTITHREADED);if(FAILED(hr))return hr;com_=true;owner_=GetCurrentThreadId();hr=CoCreateInstance(CLSID_WICImagingFactory,NULL,CLSCTX_INPROC_SERVER,IID_IWICImagingFactory,reinterpret_cast<void**>(&factory_));if(FAILED(hr))Clear();return hr;}
 HRESULT Decode(const WorldBank::RgbaPin&pin,const void*encoded,size_t bytes,void*rgba,size_t capacity,HANDLE cancel){
  if(owner_!=GetCurrentThreadId())return RPC_E_WRONG_THREAD;
  const unsigned side=Side(pin);if(!factory_||!side||!encoded||!rgba||bytes!=pin.encodedBytes||!bytes||bytes>16*1024*1024||capacity!=pin.bytes||capacity!=size_t(side)*side*4||bytes<33)return E_INVALIDARG;
  const UINT_PTR input=UINT_PTR(encoded),output=UINT_PTR(rgba),last=~UINT_PTR(0);
  if(input>last-bytes||output>last-capacity||(input<output+capacity&&output<input+bytes))return E_INVALIDARG;
  if(Cancelled(cancel))return HRESULT_FROM_WIN32(ERROR_CANCELLED);
  const BYTE*b=static_cast<const BYTE*>(encoded);static const BYTE signature[8]={137,80,78,71,13,10,26,10};
  if(std::memcmp(b,signature,8)||Big32(b+8)!=13||std::memcmp(b+12,"IHDR",4)||Big32(b+16)!=side||Big32(b+20)!=side||b[24]!=8||b[25]!=6)return E_INVALIDARG;
  BYTE expected[32],actual[32];for(unsigned i=0;i<32;++i){unsigned hi=pin.encodedSha[i*2],lo=pin.encodedSha[i*2+1];hi=hi<='9'?hi-'0':hi-'a'+10;lo=lo<='9'?lo-'0':lo-'a'+10;expected[i]=BYTE(hi*16+lo);}
  if(!AtlasView::Digest(encoded,DWORD(bytes),actual)||std::memcmp(actual,expected,32))return E_INVALIDARG;
  if(Cancelled(cancel))return HRESULT_FROM_WIN32(ERROR_CANCELLED);
  AtlasView::Com<IWICStream>stream;AtlasView::Com<IWICBitmapDecoder>decoder;AtlasView::Com<IWICBitmapFrameDecode>frame;AtlasView::Com<IWICFormatConverter>converter;
  HRESULT hr=factory_->CreateStream(&stream.p);if(SUCCEEDED(hr))hr=stream.p->InitializeFromMemory(const_cast<BYTE*>(b),DWORD(bytes));
  if(SUCCEEDED(hr))hr=factory_->CreateDecoderFromStream(stream.p,NULL,WICDecodeMetadataCacheOnDemand,&decoder.p);
  UINT count=0;if(SUCCEEDED(hr))hr=decoder.p->GetFrameCount(&count);if(SUCCEEDED(hr)&&count!=1)hr=E_INVALIDARG;
  if(SUCCEEDED(hr))hr=decoder.p->GetFrame(0,&frame.p);UINT width=0,height=0;if(SUCCEEDED(hr))hr=frame.p->GetSize(&width,&height);if(SUCCEEDED(hr)&&(width!=side||height!=side))hr=E_INVALIDARG;
  if(SUCCEEDED(hr))hr=factory_->CreateFormatConverter(&converter.p);if(SUCCEEDED(hr))hr=converter.p->Initialize(frame.p,GUID_WICPixelFormat32bppRGBA,WICBitmapDitherTypeNone,NULL,0,WICBitmapPaletteTypeCustom);
  for(unsigned y=0;y<side&&SUCCEEDED(hr);y+=16){if(Cancelled(cancel))return HRESULT_FROM_WIN32(ERROR_CANCELLED);WICRect rect={0,int(y),int(side),16};hr=converter.p->CopyPixels(&rect,side*4,side*16*4,static_cast<BYTE*>(rgba)+size_t(y)*side*4);}
  if(FAILED(hr))return hr;if(Cancelled(cancel))return HRESULT_FROM_WIN32(ERROR_CANCELLED);
  if(!AtlasView::Digest(rgba,DWORD(capacity),actual)||std::memcmp(actual,pin.sha,32))return E_INVALIDARG;
  if(Cancelled(cancel))return HRESULT_FROM_WIN32(ERROR_CANCELLED);
  return S_OK;
 }
};
}
