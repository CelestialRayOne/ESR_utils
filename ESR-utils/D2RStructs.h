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
    constexpr uint32_t HoradricCube = 1105;

    constexpr uint32_t CrystalCanFirst = 1352;
    constexpr uint32_t CrystalCanLast = 1363;
    constexpr uint32_t FlawedCrystalFirst = 1472;
    constexpr uint32_t FlawedCrystalLast = 1483;
    constexpr uint32_t NormalCrystalFirst = 1484;
    constexpr uint32_t NormalCrystalLast = 1495;
    constexpr uint32_t ChippedCrystalFirst = 1544;
    constexpr uint32_t ChippedCrystalLast = 1555;

    constexpr uint32_t GemCanFirst = 1344;
    constexpr uint32_t GemCanLast = 1351;
    constexpr uint32_t GemFirst = 1123;
    constexpr uint32_t GemLast = 1170;

    constexpr uint32_t RuneStockerFirst = 1370;
    constexpr uint32_t RuneStockerLast = 1416;
    constexpr uint32_t RuneFirst = 1298;
    constexpr uint32_t RuneLast = 1343;
    constexpr uint32_t RuneR50 = 1498;

    constexpr uint32_t MultistockerFirst = 1433;
    constexpr uint32_t MultistockerLast = 1458;

    constexpr uint32_t RerollOrbFirst = 1417;
    constexpr uint32_t RerollOrbLast = 1432;


    namespace Rarity
    {
        constexpr uint32_t Normal = 2;
        constexpr uint32_t Magic = 4;
        constexpr uint32_t Set = 5;
        constexpr uint32_t Rare = 6;
        constexpr uint32_t Unique = 7;
    }

    inline bool IsMultiItem(uint32_t classId)
    {
        switch (classId)
        {
        case 1087: case 1088: case 1089: case 1090: case 1091: case 1092:
        case 1093: case 1094: case 1095: case 1096: case 1097: case 1098:
        case 1249: case 1250: case 1228: case 1235: case 1236: case 1237:
        case 2185: case 1229: case 1230: case 1231: case 1232: case 1233:
        case 1234: case 2063: case 2064: case 2065: case 2066: case 1072:
        case 1462:
            return true;
        }
        return false;
    }

    inline bool IsCouponClassic(uint32_t classId)
    {
        if (classId >= 1693 && classId <= 1738) return true;
        if (classId >= 1739 && classId <= 1788) return true;
        if (classId >= 1789 && classId <= 1829) return true;
        if (classId >= 1830 && classId <= 1900) return true;
        if (classId >= 1901 && classId <= 1978) return true;
        if (classId >= 1979 && classId <= 2040) return true;
        if (classId >= 2041 && classId <= 2052) return true;
        if (classId >= 2053 && classId <= 2062) return true;
        return false;
    }

    inline bool IsRerollTargetClass(uint32_t c)
    {
        switch (c)
        {
        case 1078: case 1076: case 1262: case 1263: case 1264: case 1265:
        case 1266: case 1267: case 1268: case 1269: case 1270: case 1271:
        case 1272: case 1273: case 1274: case 1275: case 1276: case 1277:
        case 1278: case 1279: case 1280: case 1281: case 1282: case 1283:
        case 1284: case 1285: case 1286: case 1287: case 1288: case 1289:
        case 1290: case 1291: case 1292: case 1293:
        case 1172: case 1173: case 1174:
        case 1459: case 1460: case 1461:
        case 386: case 387: case 388: case 389:
        case 1212:
            return true;
        }
        return false;
    }

    constexpr uint32_t DecalStockerFirst = 2067;
    constexpr uint32_t DecalStockerLast = 2099;
    constexpr uint32_t DecalFirst = 1560;
    constexpr uint32_t DecalLast = 1592;

    inline bool IsCrystalCan(uint32_t c) { return c >= CrystalCanFirst && c <= CrystalCanLast; }
    inline bool IsCrystal(uint32_t c)
    {
        return (c >= FlawedCrystalFirst && c <= FlawedCrystalLast)
            || (c >= NormalCrystalFirst && c <= NormalCrystalLast)
            || (c >= ChippedCrystalFirst && c <= ChippedCrystalLast);
    }
    inline bool IsGemCan(uint32_t c) { return c >= GemCanFirst && c <= GemCanLast; }
    inline bool IsGem(uint32_t c) { return c >= GemFirst && c <= GemLast; }
    inline bool IsRuneStocker(uint32_t c) { return c >= RuneStockerFirst && c <= RuneStockerLast; }
    inline bool IsRune(uint32_t c) { return (c >= RuneFirst && c <= RuneLast) || c == RuneR50; }
    inline bool IsMultistocker(uint32_t c) { return c >= MultistockerFirst && c <= MultistockerLast; }
    inline bool IsDecalStocker(uint32_t c) { return c >= DecalStockerFirst && c <= DecalStockerLast; }
    inline bool IsDecal(uint32_t c) { return c >= DecalFirst && c <= DecalLast; }
    inline bool IsRerollOrb(uint32_t c) { return c >= RerollOrbFirst && c <= RerollOrbLast; }
}

namespace D2ROffsets
{
    constexpr uint32_t UnitHashTable = 0x1D442E0;
    constexpr uint32_t UnitTypeItem = 4;
    constexpr uint32_t HashBuckets = 128;
}