#pragma pack(push,1)
#include <ItemMarkersApi.h>
struct outer_pack_restored { char c;uint64_t n; };
#pragma pack(pop)
typedef char appearance_size[(sizeof(IM_Appearance)==12)?1:-1];
typedef char view_size[(sizeof(IM_ItemView)==40)?1:-1];
typedef char provider_size[(sizeof(IM_Provider)==40)?1:-1];
typedef char api_size[(sizeof(IM_Api)==40)?1:-1];
typedef char restored_pack[(sizeof(struct outer_pack_restored)==9)?1:-1];
int main(void) { return 0; }
