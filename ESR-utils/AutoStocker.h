#pragma once
#include "D2RStructs.h"
#include "D2RFunctions.h"
#include <vector>

enum class StockerState
{
    Idle,
    Starting,
    PickupStocker,
    WaitStockerOnCursor,
    DropStockerToCube,
    WaitStockerInCube,
    PickupCrystal,
    WaitCrystalOnCursor,
    DropCrystalToCube,
    WaitCrystalInCube,
    Transmute,
    WaitTransmute,
    PickupStockerFromCube,
    WaitStockerFromCube,
    DropStockerToInventory,
    WaitStockerInInventory,
    Complete,
    Error
};

struct ItemInfo
{
    uint32_t unitId;
    uint32_t classId;
    uint8_t page;
};

class AutoStocker
{
    public:
        void Start();
        void Stop();
        void Tick();
        bool IsRunning() const;
        std::vector<ItemInfo> FindItemsByPage(uint8_t page);
        std::vector<ItemInfo> FindCrystalsInInventory();
        ItemInfo FindStockerInInventory();
    private:
        D2UnitStrc* FindPlayerUnit();
        ItemInfo FindStockerInCube();
        void LogMessage(const char* fmt, ...);
        StockerState m_State = StockerState::Idle;
        uint32_t m_StockerUnitId = 0;
        uint32_t m_CurrentCrystalUnitId = 0;
        std::vector<ItemInfo> m_Crystals;
        int m_CrystalIndex = 0;
        int m_TickDelay = 0;
        int m_TotalStocked = 0;
};