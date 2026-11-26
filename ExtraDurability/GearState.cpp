#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include "GearState.h"
#include "ExtraDurability.h"

#include <Debug.h>
#include <core/Functions.h>
#include <kenshi/GameData.h>
#include <kenshi/Gear.h>
#include <kenshi/RootObjectFactory.h>

#include <climits>
#include <map>
#include <string>

namespace durability
{
    namespace
    {
        // The key is additional data on the saved instance. Native stats and
        // the shared item definition are left to the original serializer.
        const std::string g_saveKey("edhard_cond_v1");
        const std::string g_playerDestroyKey("edhard_destroy_player_v1");
        typedef std::map<const Item*, unsigned> Conditions;
        Conditions g_conditions;
        SRWLOCK g_lock = SRWLOCK_INIT;
        volatile LONG g_ready = 0;
        bool g_installAttempted = false;
        volatile LONG g_writeError = 0;
        volatile LONG g_storeError = 0;
        volatile LONG g_invalidValue = 0;

        struct ConditionWriteLock
        {
            ConditionWriteLock() { AcquireSRWLockExclusive(&g_lock); }
            ~ConditionWriteLock() { ReleaseSRWLockExclusive(&g_lock); }

        private:
            ConditionWriteLock(const ConditionWriteLock&);
            ConditionWriteLock& operator=(const ConditionWriteLock&);
        };

        struct ConditionReadLock
        {
            ConditionReadLock() { AcquireSRWLockShared(&g_lock); }
            ~ConditionReadLock() { ReleaseSRWLockShared(&g_lock); }

        private:
            ConditionReadLock(const ConditionReadLock&);
            ConditionReadLock& operator=(const ConditionReadLock&);
        };

        void ReportOnce(volatile LONG& reported, const char* message)
        {
            if (!InterlockedExchange(&reported, 1))
                ErrorLog(message);
        }

        unsigned EffectiveCondition(const Conditions::const_iterator& found, unsigned full)
        {
            return found != g_conditions.end() && found->second < full ? found->second : full;
        }

        struct ItemAccess : Item
        {
            using Item::_DESTRUCTOR;
        };

        void (*g_destroy)(Item*) = 0;
        GameData* (*g_save)(Gear*, GameDataContainer*, itemType) = 0;
        void (*g_load)(Gear*, GameDataContainer*, GameData*) = 0;
        Item* (*g_copy)(RootObjectFactory*, Item*) = 0;

        void DestroyItem(Item* item)
        {
            // Derived gear destructors inline Gear cleanup and call Item's
            // destructor directly in the supported optimized builds.
            if (GearStateReady())
            {
                ConditionWriteLock lock;
                g_conditions.erase(item);
            }
            g_destroy(item);
        }

        GameData* SaveGear(Gear* gear, GameDataContainer* container, itemType type)
        {
            GameData* record = g_save(gear, container, type);
            if (!record || !GearStateReady())
                return record;

            unsigned condition;
            {
                ConditionReadLock lock;
                const Conditions::const_iterator found = g_conditions.find(gear);
                // The native serializer constructs a fresh item record.
                if (found == g_conditions.end())
                    return record;
                condition = found->second;
            }

            try
            {
                record->add(g_saveKey, static_cast<int>(condition), std::string(), false);
                // A dropped item can outlive the pending removal across a save.
                if (!condition && HasPendingPlayerDestruction(gear))
                    record->add(g_playerDestroyKey, 1, std::string(), false);
            }
            catch (...)
            {
                ReportOnce(g_writeError, "[Extra Durability (HARD)] Cannot write item condition to the save");
            }
            return record;
        }

        void LoadGear(Gear* gear, GameDataContainer* container, GameData* record)
        {
            g_load(gear, container, record);
            if (!record || !GearStateReady())
                return;

            const auto value = record->idata.find(g_saveKey);
            if (value == record->idata.end() || value->second < 0)
            {
                {
                    ConditionWriteLock lock;
                    g_conditions.erase(gear);
                }
                if (value != record->idata.end())
                    ReportOnce(g_invalidValue, "[Extra Durability (HARD)] Invalid saved item condition");
                return;
            }

            try
            {
                ConditionWriteLock lock;
                g_conditions[gear] = static_cast<unsigned>(value->second);
            }
            catch (...)
            {
                ReportOnce(g_storeError, "[Extra Durability (HARD)] Cannot store item condition in memory");
                return;
            }
            if (value->second == 0) {
                const auto playerPending = record->idata.find(g_playerDestroyKey);
                QueueRestoredBrokenEquipment(gear, playerPending != record->idata.end() && playerPending->second == 1);
            }
        }

        Item* CopyItem(RootObjectFactory* factory, Item* source)
        {
            Item* copy = g_copy(factory, source);
            if (!source || !copy || copy == source || !GearStateReady())
                return copy;

            try
            {
                ConditionWriteLock lock;
                const Conditions::const_iterator found = g_conditions.find(source);
                if (found != g_conditions.end())
                    g_conditions[copy] = found->second;
            }
            catch (...)
            {
                ReportOnce(g_storeError, "[Extra Durability (HARD)] Cannot store item condition in memory");
            }
            return copy;
        }
    }

    bool GearStateReady()
    {
        return g_ready != 0;
    }

    unsigned ReadCondition(Gear* gear, unsigned full)
    {
        if (!gear || !GearStateReady())
            return full;

        ConditionReadLock lock;
        return EffectiveCondition(g_conditions.find(gear), full);
    }

    unsigned WearCondition(Gear* gear, unsigned full, unsigned amount, unsigned* before)
    {
        if (!gear || !full || full > INT_MAX || !GearStateReady())
        {
            if (before)
                *before = full;
            return full;
        }

        unsigned current = full;
        try
        {
            ConditionWriteLock lock;
            const Conditions::iterator found = g_conditions.find(gear);
            current = EffectiveCondition(found, full);
            if (before)
                *before = current;
            if (!current || !amount)
                return current;

            const unsigned remaining = amount < current ? current - amount : 0;
            if (found == g_conditions.end())
                g_conditions.insert(Conditions::value_type(gear, remaining));
            else
                found->second = remaining;
            return remaining;
        }
        catch (...)
        {
            ReportOnce(g_storeError, "[Extra Durability (HARD)] Cannot store item condition in memory");
            return current;
        }
    }

    bool RestoreCondition(Gear* gear, unsigned full, unsigned amount)
    {
        if (!gear || !full || full > INT_MAX || !amount || !GearStateReady())
            return false;

        ConditionWriteLock lock;
        const Conditions::iterator found = g_conditions.find(gear);
        const unsigned current = EffectiveCondition(found, full);
        if (current == full)
            return false;

        if (amount >= full - current)
            g_conditions.erase(found);
        else
            found->second = current + amount;
        return true;
    }

    void InstallGearState()
    {
        if (g_installAttempted)
            return;
        g_installAttempted = true;

        if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&ItemAccess::_DESTRUCTOR), DestroyItem, &g_destroy) != KenshiLib::SUCCESS)
        {
            ErrorLog("[Extra Durability (HARD)] Item condition unavailable: item destructor hook failed");
            return;
        }
        if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&Gear::_NV__serialise), SaveGear, &g_save) != KenshiLib::SUCCESS)
        {
            ErrorLog("[Extra Durability (HARD)] Item condition unavailable: Gear save hook failed");
            return;
        }
        if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&Gear::_NV__loadFromSerialise), LoadGear, &g_load) != KenshiLib::SUCCESS)
        {
            ErrorLog("[Extra Durability (HARD)] Item condition unavailable: Gear load hook failed");
            return;
        }
        if (KenshiLib::AddHook(KenshiLib::GetRealAddress(&RootObjectFactory::copyItem), CopyItem, &g_copy) != KenshiLib::SUCCESS)
        {
            ErrorLog("[Extra Durability (HARD)] Item condition unavailable: item copy hook failed");
            return;
        }

        InterlockedExchange(&g_ready, 1);
    }
}
