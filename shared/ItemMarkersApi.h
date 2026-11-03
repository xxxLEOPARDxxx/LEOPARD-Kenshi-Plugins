#ifndef LEOPARD_ITEM_MARKERS_API_H
#define LEOPARD_ITEM_MARKERS_API_H
/* ItemMarkers SDK v1, x64. This header is C-compatible and needs no game headers.
   All text is UTF-8. No Item ownership is transferred. See sdk/README.txt. */
#include <stdint.h>
#ifndef _MSC_VER
#ifndef __cdecl
#define __cdecl
#endif
#endif
#define ITEMMARKERS_API_VERSION 1u
#pragma pack(push, 8)
typedef uint64_t IM_Handle;
enum IM_Status { IM_OK=0, IM_INVALID=1, IM_FULL=2, IM_NOT_FOUND=3, IM_BUSY=4, IM_NOT_READY=5 };
typedef struct IM_Appearance {
    uint32_t size;
    int32_t rank;                 /* display threshold 0..5, NOT a grade ID */
    uint32_t rgb;                 /* 0xRRGGBB, opaque; style/opacity belong to player */
} IM_Appearance;
typedef struct IM_ItemView {
    uint32_t size;
    int32_t type;                 /* GameData::type / itemType from matching Kenshi SDK */
    const void* item;            /* borrowed current Item*, GUI thread, callback only */
    const char* item_id;
    const char* material_id;
    int32_t native_level;         /* -1 unless genuinely supported native Gear */
    uint32_t reserved;
} IM_ItemView;
/* 0: no opinion, 1: handled, any other value: faulty provider is disabled.
   Initialize result.size and fill rank/rgb on success. No retained view pointers. */
typedef int (__cdecl *IM_Query)(const IM_ItemView*, IM_Appearance*, void* context);
typedef struct IM_Provider {
    uint32_t size;
    const char* name;             /* unique stable ASCII identifier, at most 63 bytes */
    int32_t priority;             /* -10000..10000; higher first, then registration order */
    IM_Query query;
    void* context;                /* caller-owned until unregister returns IM_OK */
} IM_Provider;
typedef struct IM_Api {
    uint32_t size, version;
    int (__cdecl *register_provider)(const IM_Provider*, IM_Handle*);
    int (__cdecl *unregister_provider)(IM_Handle);
    int (__cdecl *notify_changed)(IM_Handle); /* coalesced visible-icon refresh */
    int (__cdecl *reload_rules)(void);        /* deferred filesystem work on GUI tick */
} IM_Api;
/* Discover with GetProcAddress(module, "ItemMarkers_GetApi"); no import library.
   Module lookup must be repeated until available: either plugin load order works. */
typedef const IM_Api* (__cdecl *ItemMarkers_GetApiFn)(uint32_t version, uint32_t api_size);
#pragma pack(pop)
#endif
