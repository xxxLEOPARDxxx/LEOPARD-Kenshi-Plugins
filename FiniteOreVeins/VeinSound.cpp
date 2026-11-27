#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>
#include <cstring>

#include <Debug.h>
#include <kenshi/Building/Building.h>
#include <kenshi/Character.h>
#include "VeinSound.h"

namespace finite_ore {
namespace {

typedef unsigned __int64 AudioObject;

typedef int (*RegisterObject)(AudioObject, const char*, unsigned);
typedef int (*UnregisterObject)(AudioObject);
typedef int (*SetPosition)(AudioObject, const AkSoundPosition&);
typedef int (*SetSwitch)(const char*, const char*, AudioObject);
typedef unsigned (*PostEvent)(const char*, AudioObject, unsigned, void*, void*,
                              unsigned, void*, unsigned);

RegisterObject registerObject = NULL;
UnregisterObject unregisterObject = NULL;
SetPosition setPosition = NULL;
SetSwitch setSwitch = NULL;
PostEvent postEvent = NULL;
const void* volatile const* audioQueue = NULL;
unsigned soundSerial = 0;
bool reportedPlaybackError = false;

enum { Register, Unregister, Position, Switch, Post, FunctionCount };
struct Profile {
    DWORD timestamp;
    DWORD imageSize;
    unsigned queue;
    unsigned function[FunctionCount];
};

// Wwise entry points used by the native building and character sound paths.
const Profile profiles[] = {
    {0x65d604d7, 0x232c000, 0x2135b60, {0xa7fa50, 0xa7fb00, 0xa81d80, 0xa82010, 0xa860c0}},
    {0x65d60519, 0x232a000, 0x2133ad0, {0xa7f170, 0xa7f220, 0xa814a0, 0xa81730, 0xa857e0}}
};
const unsigned char signatures[][24] = {
    {0x48, 0x89, 0x6c, 0x24, 0x10, 0x48, 0x89, 0x74, 0x24, 0x18, 0x57, 0x48,
     0x83, 0xec, 0x20, 0x41, 0x8b, 0xf0, 0x48, 0x8b, 0xea, 0x48, 0x8b, 0xf9},
    {0x40, 0x53, 0x48, 0x83, 0xec, 0x20, 0x48, 0x8b, 0xd9, 0x48, 0x85, 0xc9,
     0x75, 0x09, 0x8d, 0x41, 0x02, 0x48, 0x83, 0xc4, 0x20, 0x5b, 0xc3, 0xe8},
    {0x48, 0x85, 0xc9, 0x75, 0x04, 0x8d, 0x41, 0x02, 0xc3, 0xe9, 0x82, 0xcb,
     0xff, 0xff, 0xcc, 0xcc, 0x48, 0x89, 0x5c, 0x24, 0x08, 0x48, 0x89, 0x74},
    {0x48, 0x89, 0x5c, 0x24, 0x08, 0x48, 0x89, 0x74, 0x24, 0x10, 0x57, 0x48,
     0x83, 0xec, 0x20, 0x49, 0x8b, 0xf0, 0x48, 0x8b, 0xda, 0xe8, 0x66, 0xfc},
    {0x48, 0x89, 0x5c, 0x24, 0x08, 0x48, 0x89, 0x6c, 0x24, 0x10, 0x48, 0x89,
     0x7c, 0x24, 0x18, 0x41, 0x54, 0x41, 0x55, 0x41, 0x56, 0x48, 0x83, 0xec}
};

bool BindAudio()
{
    const unsigned char* image = reinterpret_cast<const unsigned char*>(GetModuleHandleW(NULL));
    if (!image)
        return false;
    const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
        return false;
    const IMAGE_NT_HEADERS64* pe = reinterpret_cast<const IMAGE_NT_HEADERS64*>(image + dos->e_lfanew);
    if (pe->Signature != IMAGE_NT_SIGNATURE || pe->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        pe->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        return false;

    for (unsigned i = 0; i != sizeof(profiles) / sizeof(profiles[0]); ++i) {
        const Profile& profile = profiles[i];
        if (pe->FileHeader.TimeDateStamp != profile.timestamp ||
            pe->OptionalHeader.SizeOfImage != profile.imageSize)
            continue;
        intptr_t entry[FunctionCount];
        for (unsigned j = 0; j != FunctionCount; ++j) {
            const unsigned char* address = image + profile.function[j];
            if (std::memcmp(address, signatures[j], sizeof(signatures[j])))
                return false;
            entry[j] = reinterpret_cast<intptr_t>(address);
        }
        audioQueue = reinterpret_cast<const void* volatile const*>(image + profile.queue);
        registerObject = reinterpret_cast<RegisterObject>(entry[Register]);
        unregisterObject = reinterpret_cast<UnregisterObject>(entry[Unregister]);
        setPosition = reinterpret_cast<SetPosition>(entry[Position]);
        setSwitch = reinterpret_cast<SetSwitch>(entry[Switch]);
        postEvent = reinterpret_cast<PostEvent>(entry[Post]);
        return true;
    }
    return false;
}
}

void InitializeVeinSound()
{
    if (!BindAudio())
        ErrorLog("FiniteOreVeins: unsupported native audio, vein depletion sound unavailable");
}

void PlayVeinBreak(Building& vein)
{
    if (!audioQueue || !*audioQueue)
        return;
    // Building destruction calls StopAll on the building emitter. Use a
    // separate, fixed-position emitter whose ID is outside Kenshi's range.
    const AudioObject emitter = 0x464f560000000000ULL | ++soundSerial;
    const Ogre::Vector3& at = vein.getPosition();
    const AkSoundPosition position = {{at.x, at.y, at.z}, {0.0f, 0.0f, 1.0f}};
    const int akSuccess = 1;
    if (registerObject(emitter, "FiniteOreVeins", 1) == akSuccess) {
        const bool started = setPosition(emitter, position) == akSuccess &&
                             setSwitch("Impacts", "Stone", emitter) == akSuccess &&
                             postEvent("Impact", emitter, 0, NULL, NULL, 0, NULL, 0) != 0;
        // An unregistered Wwise emitter retains its last position for voices
        // already playing. This releases it without stopping the one-shot.
        const bool released = unregisterObject(emitter) == akSuccess;
        if (started && released)
            return;
    }
    if (!reportedPlaybackError) {
        reportedPlaybackError = true;
        ErrorLog("FiniteOreVeins: could not play or release the vein depletion sound");
    }
}

}
