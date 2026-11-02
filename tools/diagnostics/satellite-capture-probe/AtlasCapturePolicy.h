#pragma once
namespace AtlasCapture {
struct Block {unsigned tileX,tileZ,x,z,regionX,regionZ;float centerX,centerZ,span;};
// Fixed diagnostic footprints; case0 preserves the original common-frame control.
// It is deliberately NOT an atlas cursor and must never be sent to Gst2::Writer.
inline bool OverlapControl(Block& out,unsigned controlCase=0){
 if(controlCase>1)return false; // Refuse without changing the caller's block.
 Block b;b.tileX=1;b.tileZ=1;b.x=5120;b.z=6144;b.regionX=3;b.regionZ=3;
 b.span=18432;b.centerX=-18432;b.centerZ=-9216;
 if(controlCase==1){b.x=6128;b.z=6128;b.centerX=-9360;b.centerZ=-9360;}
 out=b;return true;
}
inline bool Select(unsigned index,Block& out){
 if(index>=256)return false;
 const unsigned tile=index/16,local=index%16;
 Block b;b.tileX=tile%4;b.tileZ=tile/4;b.x=(local%4)*2048;b.z=(local/4)*2048;
 b.regionX=b.tileX*2+local%4/2;b.regionZ=b.tileZ*2+local/4/2;
 b.span=18432;b.centerX=-147456.0f+b.tileX*73728.0f+(b.x+1024)*9.0f;
 b.centerZ=-147456.0f+b.tileZ*73728.0f+(b.z+1024)*9.0f;out=b;return true;
}
}
