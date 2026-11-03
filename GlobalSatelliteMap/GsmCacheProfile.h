#pragma once
// Fixed development profile for the verified16GiB GPU test machine.
// Render demand stays bounded independently of the retained neighborhood.
namespace GsmCacheProfile {
enum { MaxVisible=96,FineCacheSlots=256,DiskSlots=512,RetirementSlots=512 };
const unsigned long long GpuBytes=512ULL*1024*1024;
// Overview, lighting, water and both city paths; separate from fine tile cache.
const unsigned long long ViewerBytes=40ULL*1024*1024;
}
