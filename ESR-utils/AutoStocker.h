#pragma once
#include "D2RStructs.h"
#include "D2RFunctions.h"
#include <vector>

enum class StockerType
{
    Crystals,
    Gems,
    Runes,
    Decals,
    Multistocker,
    Reroll
};

enum class StockerState
{
    Idle,
    NextStockerType,
    VerifyCubeEmpty,
    PickupStocker,
    ClickCubeForStocker,
    WaitStockerInCube,
    NextTabContext,
    WaitTabSwitched,
    NextConsumable,
    PickupConsumable,
    ClickCubeForConsumable,
    WaitConsumableInCube,
    Transmute,
    WaitTransmute,
    EjectStocker_OpenCube,
    EjectStocker_CtrlClickStocker,
    EjectStocker_CloseCube,
    WaitBetweenStockerTypes,
    Complete,
    Error
};

struct ItemInfo
{
    uint32_t unitId;
    uint32_t classId;
    uint8_t  page;
    uint32_t containerId;
};

struct RunRequest
{
    bool crystals = false;
    bool gems = false;
    bool runes = false;
    bool decals = false;
    bool multistocker = false;
    bool storeNonBlankCoupons = false;
    bool reroll = false;
    bool rerollSkipInventory = false;
    bool rerollSkipStash = false;

    bool rerollMagicRings = false;
    bool rerollMagicAmulets = false;
    bool rerollMagicJewels = false;
    bool rerollMagicCharms = false;
    bool rerollMagicQuivers = false;

    bool rerollRareRings = false;
    bool rerollRareAmulets = false;
    bool rerollRareJewels = false;
    bool rerollRareCharms = false;
    bool rerollRareQuivers = false;

    bool rerollSetUniqueRings = false;
    bool rerollSetUniqueAmulets = false;
    bool rerollSetUniqueQuivers = false;
};

class AutoStocker
{
public:
    void Start(const RunRequest& req);
    void Stop();
    void Tick();
    bool IsRunning() const;

    std::vector<ItemInfo> FindItemsByPage(uint8_t page);
    ItemInfo FindCubeInInventory();
    bool GetSharedTabBaseId(uint32_t& outBase);

private:
    D2UnitStrc* FindPlayerUnit();

    bool IsStockerClassOfType(uint32_t classId, StockerType t);
    bool IsConsumableOfType(uint32_t classId, StockerType t, bool couponsEnabled);
    const char* TypeName(StockerType t);

    ItemInfo FindStockerOfType(StockerType t);
    std::vector<ItemInfo> FindConsumablesInContext(StockerType t, uint8_t page, uint32_t containerId);

    void BeginStockerType(StockerType t);
    void BeginNextContext();

    RunRequest m_Req;

    std::vector<StockerType> m_TypeQueue;
    int m_TypeIndex = 0;
    StockerType m_CurrentType = StockerType::Crystals;

    StockerState m_State = StockerState::Idle;
    uint32_t m_StockerUnitId = 0;
    uint32_t m_CubeUnitId = 0;
    int m_CubeClickX = 0;
    int m_CubeClickY = 0;

    std::vector<ItemInfo> m_Consumables;
    int m_ConsumableIndex = 0;

    int m_TabIndex = -1;
    uint32_t m_CurrentContainerId = 0;
    uint8_t m_CurrentPage = 0;

    int m_TickDelay = 0;
    int m_TotalStocked = 0;
};

void InventoryGridToScreen(uint32_t gx, uint32_t gy, int& sx, int& sy);
void StashGridToScreen(uint32_t gx, uint32_t gy, int& sx, int& sy);
void CubeGridToScreen(uint32_t gx, uint32_t gy, int& sx, int& sy);
void ClickAtScreenPos(int x, int y);
void CtrlClickAtScreenPos(int x, int y);
void RightClickAtScreenPos(int x, int y);
void PressEscape();
void ClickStashTabButton(int tabIndex);