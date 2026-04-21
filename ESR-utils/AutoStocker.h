#pragma once
#include "D2RStructs.h"
#include "D2RFunctions.h"
#include <vector>

enum class StockerState
{
    Idle,
    Start,
    PickupStocker,
    ClickCubeForStocker,
    WaitStockerInCube,
    NextCrystal,
    PickupCrystal,
    ClickCubeForCrystal,
    WaitCrystalInCube,
    Transmute,
    WaitTransmute,
    SwitchTab,
    WaitTabSwitched,
    Complete,
    Error
};

struct ItemInfo
{
    uint32_t unitId;
    uint32_t classId;
    uint8_t page;
    uint32_t containerId;
};

class AutoStocker
{
public:
    void Start();
    void Stop();
    void Tick();
    bool IsRunning() const;
    std::vector<ItemInfo> FindItemsByPage(uint8_t page);
    std::vector<ItemInfo> FindCrystalsInCurrentContext(uint8_t page, uint32_t containerId);
    ItemInfo FindStocker();
    ItemInfo FindCubeInInventory();
    bool GetSharedTabBaseId(uint32_t& outBase);

private:
    D2UnitStrc* FindPlayerUnit();
    void LogMessage(const char* fmt, ...);

    StockerState m_State = StockerState::Idle;
    uint32_t m_StockerUnitId = 0;
    uint32_t m_CubeUnitId = 0;
    int m_CubeClickX = 0;
    int m_CubeClickY = 0;

    std::vector<ItemInfo> m_Crystals;
    int m_CrystalIndex = 0;

    int m_TickDelay = 0;
    int m_TotalStocked = 0;

    int m_TabIndex = -1;
    bool m_StashProcessed = false;
    uint32_t m_CurrentContainerId = 0;
    uint8_t m_CurrentPage = 0;
};

void InventoryGridToScreen(uint32_t gx, uint32_t gy, int& sx, int& sy);
void StashGridToScreen(uint32_t gx, uint32_t gy, int& sx, int& sy);
void ClickAtScreenPos(int x, int y);
void ClickStashTabButton(int tabIndex);