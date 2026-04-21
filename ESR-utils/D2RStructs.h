#pragma once
#include <cstdint>
#include <cstddef>

#pragma pack(push, 1)

class D2ItemDataStrc
{
public:
    char pad_0000[0x18];            // 0x0000
    uint32_t dwItemFlags;           // 0x0018
    char pad_001C[0x9C];            // 0x001C
    uint8_t nPage;                  // 0x00B8
    char pad_00B9[0x5F];            // 0x00B9
};
static_assert(offsetof(D2ItemDataStrc, dwItemFlags) == 0x18);
static_assert(offsetof(D2ItemDataStrc, nPage) == 0xB8);

class D2InventoryStrc
{
public:
    char pad_0000[0x10];            // 0x0000
    class D2UnitStrc* pFirstItem;   // 0x0010
    char pad_0018[0x70];            // 0x0018
};

class D2UnitStrc
{
public:
    uint32_t dwUnitType;            // 0x0000
    uint32_t dwClassId;             // 0x0004
    uint32_t dwUnitId;              // 0x0008
    uint32_t dwAnimMode;            // 0x000C
    D2ItemDataStrc* pItemData;      // 0x0010
    char pad_0018[0x78];            // 0x0018
    D2InventoryStrc* pInventory;    // 0x0090
    char pad_0098[0xB8];            // 0x0098
    D2UnitStrc* pListNext;          // 0x0150
    char pad_0158[0x60];            // 0x0158
};
static_assert(offsetof(D2UnitStrc, pItemData) == 0x10);
static_assert(offsetof(D2UnitStrc, pInventory) == 0x90);
static_assert(offsetof(D2UnitStrc, pListNext) == 0x150);

#pragma pack(pop)

namespace ItemPage
{
    constexpr uint8_t Inventory = 3;
    constexpr uint8_t Equip = 1;
    constexpr uint8_t Belt = 2;
    constexpr uint8_t Cube = 6;
    constexpr uint8_t Stash = 7;
}

namespace ItemFlag
{
    constexpr uint32_t Identified = 0x10;
}

namespace ESRClassId
{
    constexpr uint32_t CrystalCanFirst = 1352;
    constexpr uint32_t CrystalCanLast = 1363;
    constexpr uint32_t HoradricCube = 1105;

    constexpr uint32_t FlawedCrystalFirst = 1472;
    constexpr uint32_t FlawedCrystalLast = 1483;

    constexpr uint32_t NormalCrystalFirst = 1484;
    constexpr uint32_t NormalCrystalLast = 1495;

    constexpr uint32_t ChippedCrystalFirst = 1544;
    constexpr uint32_t ChippedCrystalLast = 1555;

    inline bool IsCrystal(uint32_t classId)
    {
        return (classId >= FlawedCrystalFirst && classId <= FlawedCrystalLast) ||
            (classId >= NormalCrystalFirst && classId <= NormalCrystalLast) ||
            (classId >= ChippedCrystalFirst && classId <= ChippedCrystalLast);
    }

    inline bool IsCrystalCan(uint32_t classId)
    {
        return classId >= CrystalCanFirst && classId <= CrystalCanLast;
    }
}

namespace D2ROffsets
{
    constexpr uint32_t UnitHashTable = 0x1D442E0;
    constexpr uint32_t UnitTypeItem = 4;
    constexpr uint32_t HashBuckets = 128;
}