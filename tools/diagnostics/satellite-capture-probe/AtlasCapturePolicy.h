#pragma once
namespace AtlasCapture {
struct Block {unsigned tileX,tileZ,x,z,regionX,regionZ;float centerX,centerZ,span;};
inline bool Select(unsigned index,Block& out){
 if(index>=256)return false;
 const unsigned tile=index/16,local=index%16;
 Block b;b.tileX=tile%4;b.tileZ=tile/4;b.x=(local%4)*2048;b.z=(local/4)*2048;
 b.regionX=b.tileX*2+local%4/2;b.regionZ=b.tileZ*2+local/4/2;
 b.span=18432;b.centerX=-147456.0f+b.tileX*73728.0f+(b.x+1024)*9.0f;
 b.centerZ=-147456.0f+b.tileZ*73728.0f+(b.z+1024)*9.0f;out=b;return true;
}
}
