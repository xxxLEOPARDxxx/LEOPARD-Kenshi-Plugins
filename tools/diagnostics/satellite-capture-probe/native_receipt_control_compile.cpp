#include "NativeReceiptControl.h"
// ABI/link-only control; never run by this offline compile.
extern "C" __declspec(dllexport) HRESULT ReceiptControlContract(const SourceReceipts::Scope*s,NativeReceiptControl::Report*out){return NativeReceiptControl::Run(*s,*out);}
