// Integration fragment, not a standalone Kenshi plugin. Call TryConnect from
// your existing GUI/main-thread initialization/update, then stop retrying after
// success. Supply your own read-only instance-grade lookup. No extra hooks needed.
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <ItemMarkersApi.h>
typedef bool (__cdecl *MyGradeLookup)(const void* item,int* rank,uint32_t* rgb);
static const IM_Api* api=0;
static IM_Handle provider=0;
static HMODULE markerModule=0;
static MyGradeLookup lookup=0;
static int __cdecl Query(const IM_ItemView* view,IM_Appearance* out,void*) {
    int rank=0;uint32_t rgb=0;
    // The owning mod knows its own instance quality. Do not cast every Item to Gear.
    if(!lookup||!lookup(view->item,&rank,&rgb))return 0;
    out->size=sizeof(*out);out->rank=rank;out->rgb=rgb;return 1;
}
bool TryConnectItemMarkers(MyGradeLookup readOnlyLookup) {
    if(provider)return true;
    HMODULE found=GetModuleHandleA("ItemMarkers.dll");if(!found)return false;
    ItemMarkers_GetApiFn get=reinterpret_cast<ItemMarkers_GetApiFn>(GetProcAddress(found,"ItemMarkers_GetApi"));
    if(!get)return false;
    HMODULE pinned=0;
    if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,reinterpret_cast<LPCSTR>(get),&pinned))return false;
    const IM_Api* candidate=get(ITEMMARKERS_API_VERSION,sizeof(IM_Api));
    if(!candidate) { FreeLibrary(pinned);return false; }
    lookup=readOnlyLookup;
    IM_Provider definition={sizeof(IM_Provider),"your-mod.backpack-quality",100,&Query,0};
    IM_Handle handle=0;
    if(candidate->register_provider(&definition,&handle)!=IM_OK) { FreeLibrary(pinned);return false; }
    api=candidate;provider=handle;markerModule=pinned;return true;
}
void ItemGradeActuallyChanged() { if(api&&provider)api->notify_changed(provider); }
// Call before freeing callback context or unloading your plugin, OUTSIDE DllMain.
// If BUSY, defer until after callback; do not unload/free anything yet.
bool DisconnectItemMarkers() {
    if(!provider)return true;
    if(api->unregister_provider(provider)!=IM_OK)return false;
    provider=0;api=0;lookup=0;HMODULE release=markerModule;markerModule=0;FreeLibrary(release);return true;
}
