#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h>

#include "FiniteOreVeins.h"

#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/Building/ProductionBuilding.h>
#include <kenshi/Inventory.h>

namespace finite_ore
{
namespace
{
    typedef void (*UpdateOutput)(ProductionBuilding*, float);
    UpdateOutput originalUpdateOutput = NULL;

    // The SDK thunk is public in the binary and protected in the C++ header.
    struct OutputAddress : ProductionBuilding
    {
        using ProductionBuilding::_NV_updateOutput;
    };

    struct Profile
    {
        DWORD timestamp;
        DWORD imageSize;
        unsigned output;
        unsigned getProductionItem;
        unsigned getNumItems;
        unsigned threshold;
        unsigned __int64 outputHash;
    };

    // Offline checks use the RE_Kenshi-compatible 1.0.65 images. The SDK
    // slots are 5783, 6452 and 4393 in the installed 0.5.0 runtime.
    const Profile profiles[] = {
        {0x65d604d7, 0x232c000, 0x298470, 0x595a90, 0x745d00, 0x16af574, 0x89d0b4c776e1a37dULL},
        {0x65d60519, 0x232a000, 0x298000, 0x595da0, 0x745630, 0x16ae284, 0xe8ecda35220d9042ULL}
    };
    const float completionProgress = 0.999f;

    unsigned __int64 HashBytes(const unsigned char* bytes, unsigned size)
    {
        unsigned __int64 hash = 14695981039346656037ULL;
        for (unsigned i = 0; i != size; ++i)
            hash = (hash ^ bytes[i]) * 1099511628211ULL;
        return hash;
    }

    intptr_t AdmitOutputAddress()
    {
        const unsigned char* image =
            reinterpret_cast<const unsigned char*>(GetModuleHandleW(NULL));
        if (!image)
            return 0;
        const IMAGE_DOS_HEADER* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(image);
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew <= 0)
            return 0;
        const IMAGE_NT_HEADERS64* pe =
            reinterpret_cast<const IMAGE_NT_HEADERS64*>(image + dos->e_lfanew);
        if (pe->Signature != IMAGE_NT_SIGNATURE ||
            pe->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
            pe->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            return 0;

        for (unsigned i = 0; i != sizeof(profiles) / sizeof(profiles[0]); ++i)
        {
            const Profile& profile = profiles[i];
            if (pe->FileHeader.TimeDateStamp != profile.timestamp ||
                pe->OptionalHeader.SizeOfImage != profile.imageSize)
                continue;
            const intptr_t address = KenshiLib::GetRealAddress(&OutputAddress::_NV_updateOutput);
            if (address != reinterpret_cast<intptr_t>(image + profile.output) ||
                KenshiLib::GetRealAddress(&StorageBuilding::getProductionItem) !=
                    reinterpret_cast<intptr_t>(image + profile.getProductionItem) ||
                KenshiLib::GetRealAddress(&InventorySection::getNumItems) !=
                    reinterpret_cast<intptr_t>(image + profile.getNumItems))
                return 0;
            // updateOutput inserts one item and resets the progress after the
            // attempt. The two read helpers return the native output record
            // and sum every stack quantity in its physical section.
            if (HashBytes(image + profile.output, 0x244) != profile.outputHash ||
                HashBytes(image + profile.getProductionItem, 8) != 0xf9d8482f0e1c0b18ULL ||
                HashBytes(image + profile.getNumItems, 0x34) != 0x97208d7e51c34253ULL ||
                *reinterpret_cast<const float*>(image + profile.threshold) != completionProgress)
                return 0;
            return address;
        }
        return 0;
    }

    struct OutputReservation
    {
        ProductionBuilding* building;
        bool produced;

        explicit OutputReservation(ProductionBuilding* building)
            : building(building), produced(false) {}

        ~OutputReservation()
        {
            EndOreOutput(building, produced);
        }

    private:
        OutputReservation(const OutputReservation&);
        OutputReservation& operator=(const OutputReservation&);
    };

    void UpdateOutputHook(ProductionBuilding* building, float rate)
    {
        if (!IsManagedOre(building))
        {
            originalUpdateOutput(building, rate);
            return;
        }
        if (!BeginOreOutput(building))
            return;
        OutputReservation reservation(building);
        StorageBuilding::ConsumptionItem* const output = building->getProductionItem();
        // Match the native float addition and threshold. An ordinary tick
        // that only advances progress does not scan inventory.
        const bool canComplete = output && output->item && output->inventorySection &&
                                 output->amount + rate >= completionProgress;
        if (!canComplete)
        {
            originalUpdateOutput(building, rate);
            return;
        }
        InventorySection* const section = output->inventorySection;
        GameData* const product = output->item;
        const unsigned before = section->getNumItems();
        originalUpdateOutput(building, rate);
        StorageBuilding::ConsumptionItem* const current = building->getProductionItem();
        if (current != output || current->inventorySection != section || current->item != product)
            return;
        const unsigned after = section->getNumItems();
        reservation.produced = after > before && after - before == 1;
    }
}

bool InstallOutputHook()
{
    if (originalUpdateOutput)
        return true;

    intptr_t address = 0;
    __try
    {
        address = AdmitOutputAddress();
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        address = 0;
    }
    if (!address)
    {
        ErrorLog("FiniteOreVeins: unsupported native ore output contract");
        return false;
    }
    if (KenshiLib::AddHook(address, UpdateOutputHook, &originalUpdateOutput) != KenshiLib::SUCCESS)
    {
        ErrorLog("FiniteOreVeins: failed to install the native ore output hook");
        return false;
    }
    return true;
}
}
