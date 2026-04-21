#pragma once
#include "D2RStructs.h"
#include "Pattern.h"

namespace D2RFunctions
{
    using fnTransmute = void(__fastcall*)();
    using fnGetUnitStatSigned = int32_t(__fastcall*)(D2UnitStrc* pUnit, uint32_t nStatId, uint16_t nLayer);
    using fnSendPacket = void(__fastcall*)(void* pPacket, size_t nSize);

    inline fnTransmute D2CLIENT_Transmute = nullptr;
    inline fnGetUnitStatSigned STATLIST_GetUnitStatSigned = nullptr;
    inline fnSendPacket SendPacketToServer = nullptr;

    inline D2UnitStrc* FindItemById(uint32_t unitId)
    {
        uintptr_t base = Pattern::Base();
        uintptr_t table = base + 0x1D442E0 + (4 * 0x400);

        for (uint32_t i = 0; i < 128; i++)
        {
            D2UnitStrc* unit = *reinterpret_cast<D2UnitStrc**>(table + i * 8);
            while (unit)
            {
                if (unit->dwUnitType == 4 && unit->dwUnitId == unitId)
                    return unit;
                unit = unit->pListNext;
            }
        }
        return nullptr;
    }

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

        SendPacketToServer = reinterpret_cast<fnSendPacket>(Pattern::Address(0x10E710));

        return D2CLIENT_Transmute && STATLIST_GetUnitStatSigned && SendPacketToServer;
    }

    inline void PickItemFromBuffer(uint32_t unitId)
    {
        D2UnitStrc* pItem = FindItemById(unitId);
        if (!pItem) return;

        uint32_t x = 0, y = 0, page = 0xFF;
        uintptr_t pStaticPath = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uint8_t*>(pItem) + 0x38);
        if (pStaticPath)
        {
            x = *reinterpret_cast<uint32_t*>(pStaticPath + 0x10);
            y = *reinterpret_cast<uint32_t*>(pStaticPath + 0x14);
        }
        if (pItem->pItemData)
        {
            page = *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(pItem->pItemData) + 0x55);
        }

        uint8_t packet[17] = {};
        packet[0] = 0x19;
        *reinterpret_cast<uint32_t*>(&packet[1]) = unitId;
        *reinterpret_cast<uint32_t*>(&packet[5]) = x;
        *reinterpret_cast<uint32_t*>(&packet[9]) = y;
        *reinterpret_cast<uint32_t*>(&packet[13]) = page;
        SendPacketToServer(packet, 17);
    }

    inline void PickItemFromSharedStash(uint32_t unitId)
    {
        D2UnitStrc* pItem = FindItemById(unitId);
        if (!pItem || !pItem->pItemData) return;

        uint32_t containerId = *reinterpret_cast<uint32_t*>(
            reinterpret_cast<uint8_t*>(pItem->pItemData) + 0x0C);

        uint16_t x = 0, y = 0;
        uintptr_t pStaticPath = *reinterpret_cast<uintptr_t*>(
            reinterpret_cast<uint8_t*>(pItem) + 0x38);
        if (pStaticPath)
        {
            x = *reinterpret_cast<uint16_t*>(pStaticPath + 0x10);
            y = *reinterpret_cast<uint16_t*>(pStaticPath + 0x14);
        }

        uint8_t packet[13] = {};
        packet[0] = 0x46;
        *reinterpret_cast<uint32_t*>(&packet[1]) = unitId;
        *reinterpret_cast<uint32_t*>(&packet[5]) = containerId;
        *reinterpret_cast<uint16_t*>(&packet[9]) = x;
        *reinterpret_cast<uint16_t*>(&packet[11]) = y;
        SendPacketToServer(packet, 13);
    }

    inline void DropItemToBuffer(uint32_t unitId, uint32_t x, uint32_t y, uint32_t page)
    {
        uint8_t packet[17] = {};
        packet[0] = 0x18;
        *reinterpret_cast<uint32_t*>(&packet[1]) = unitId;
        *reinterpret_cast<uint32_t*>(&packet[5]) = x;
        *reinterpret_cast<uint32_t*>(&packet[9]) = y;
        *reinterpret_cast<uint32_t*>(&packet[13]) = page;
        SendPacketToServer(packet, 17);
    }

    inline void DropItemOnItem(uint32_t pickedItemId, uint32_t targetItemId, uint16_t slotX = 0, uint16_t slotY = 0)
    {
        uint8_t packet[21] = {};
        packet[0] = 0x2A;
        *reinterpret_cast<uint32_t*>(&packet[1]) = pickedItemId;
        *reinterpret_cast<uint32_t*>(&packet[5]) = targetItemId;
        *reinterpret_cast<uint16_t*>(&packet[17]) = slotX;
        *reinterpret_cast<uint16_t*>(&packet[19]) = slotY;
        SendPacketToServer(packet, 21);
    }

    inline void DropItemToGridAtCube(uint32_t pickedItemId, uint32_t cubeItemId)
    {
        D2UnitStrc* pCube = FindItemById(cubeItemId);
        if (!pCube) return;

        uint32_t x = 0, y = 0;
        uintptr_t pStaticPath = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uint8_t*>(pCube) + 0x38);
        if (pStaticPath)
        {
            x = *reinterpret_cast<uint32_t*>(pStaticPath + 0x10);
            y = *reinterpret_cast<uint32_t*>(pStaticPath + 0x14);
        }

        uint8_t packet[17] = {};
        packet[0] = 0x18;
        *reinterpret_cast<uint32_t*>(&packet[1]) = pickedItemId;
        *reinterpret_cast<uint32_t*>(&packet[5]) = x;
        *reinterpret_cast<uint32_t*>(&packet[9]) = y;
        *reinterpret_cast<uint32_t*>(&packet[13]) = 0;
        SendPacketToServer(packet, 17);
    }

    inline bool GetCubeGridPos(uint32_t cubeId, uint32_t& outX, uint32_t& outY)
    {
        D2UnitStrc* pCube = FindItemById(cubeId);
        if (!pCube) return false;
        uintptr_t pStaticPath = *reinterpret_cast<uintptr_t*>(reinterpret_cast<uint8_t*>(pCube) + 0x38);
        if (!pStaticPath) return false;
        outX = *reinterpret_cast<uint32_t*>(pStaticPath + 0x10);
        outY = *reinterpret_cast<uint32_t*>(pStaticPath + 0x14);
        return true;
    }
}