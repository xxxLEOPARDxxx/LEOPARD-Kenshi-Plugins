#pragma once
#include "atlas-cache/fine-store/FineStore.h"
#include <sstream>
namespace GsmFinePolicy {
inline std::string NativeRoot(std::string root){while(root.size()>3&&(root[root.size()-1]=='/'||root[root.size()-1]=='\\'))root.erase(root.size()-1);return root;}
// Configure is a pure path copy/validation. The persistent file worker creates
// the one new child and performs all filesystem operations.
inline bool CacheConfig(std::wstring parent,DWORD process,FineStore::U64 epoch,FineStore::Config&out){
 while(parent.size()>3&&(parent[parent.size()-1]==L'\\'||parent[parent.size()-1]==L'/'))parent.erase(parent.size()-1);
 if(!process||!epoch)return false;std::wostringstream child;child<<parent<<L"\\fine-session-"<<process<<L'-'<<epoch;return FineStore::Configure(out,parent.c_str(),child.str().c_str());
}
}
