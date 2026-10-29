#pragma once
#include <Windows.h>
#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Kenshi.h>
#include <kenshi/util/UtilityT.h>

// Same version table used by the working RecruitMarkers/Vital-Sense code.
namespace WorldMarkerProjection
{
    inline UtilityT* Get()
    {
        static UtilityT* p = NULL;
        static bool tried = false;
        if (tried) return p;
        tried = true;
        KenshiLib::BinaryVersion v = KenshiLib::GetKenshiVersion();
        uintptr_t offset = 0;
        const std::string version = v.GetVersion();
        if (v.GetPlatform() == KenshiLib::BinaryVersion::STEAM)
            offset = version == "1.0.65" ? 0x02134b10 : version == "1.0.68" ? 0x02135b70 : 0;
        else if (v.GetPlatform() == KenshiLib::BinaryVersion::GOG)
            offset = version == "1.0.65" ? 0x02132a80 : version == "1.0.68" ? 0x02134aa0 : 0;
        if (offset) p = (UtilityT*)((uintptr_t)GetModuleHandleA(NULL) + offset);
        else ErrorLog("World markers: unsupported game version " + v.ToString());
        return p;
    }
    inline bool Project(const Ogre::Vector3& pos, float& x, float& y)
    {
        UtilityT* const utility = Get();
        if (utility == NULL) return false;
        __try { return utility->worldToScreenPX(pos, x, y); }
        __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
    }
}
