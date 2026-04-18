#pragma once
#include "D2RStructs.h"
#include "Pattern.h"

namespace D2RFunctions
{
    using fnTransmute = void(__fastcall*)();
    using fnGetUnitStatSigned = int32_t(__fastcall*)(D2UnitStrc* pUnit, uint32_t nStatId, uint16_t nLayer);

    inline fnTransmute D2CLIENT_Transmute = nullptr;
    inline fnGetUnitStatSigned STATLIST_GetUnitStatSigned = nullptr;

    inline bool Init()
    {
        uintptr_t base = Pattern::Base();
        size_t size = Pattern::ModuleSize();

        auto resolve_call = [](uintptr_t addr) -> uintptr_t
            {
                int32_t rel = *reinterpret_cast<int32_t*>(addr + 1);
                return addr + 5 + rel;
            };

        uintptr_t transmute_ref = Pattern::FindPattern(base, size,
            "E8 ? ? ? ? EB 41 48 8B 03");
        if (transmute_ref)
            D2CLIENT_Transmute = reinterpret_cast<fnTransmute>(resolve_call(transmute_ref));

        uintptr_t stat_addr = Pattern::FindPattern(base, size,
            "48 83 EC 28 45 0F B7 C8 48 85 C9 74 42 48 8B 89 ? ? ? ? 48 85 C9 74 2F");
        if (stat_addr)
            STATLIST_GetUnitStatSigned = reinterpret_cast<fnGetUnitStatSigned>(stat_addr);

        return D2CLIENT_Transmute && STATLIST_GetUnitStatSigned;
    }
}