#include "AutoStocker.h"
#include "BankPanelHook.h"
#include "Pattern.h"
#include <Windows.h>
#include <cstdio>
#include <cstdarg>

void InventoryGridToScreen(uint32_t gx, uint32_t gy, int& sx, int& sy)
{
    constexpr double SLOT = 48.8;
    constexpr double ORIGIN_X = 1422.0;
    constexpr double ORIGIN_Y = 438.0;
    sx = static_cast<int>(ORIGIN_X + gx * SLOT);
    sy = static_cast<int>(ORIGIN_Y + gy * SLOT);
}

void StashGridToScreen(uint32_t gx, uint32_t gy, int& sx, int& sy)
{
    constexpr double SLOT = 49.0;
    constexpr double ORIGIN_X = 70.0;
    constexpr double ORIGIN_Y = 146.0;
    sx = static_cast<int>(ORIGIN_X + gx * SLOT);
    sy = static_cast<int>(ORIGIN_Y + gy * SLOT);
}

void ClickAtScreenPos(int x, int y)
{
    SetCursorPos(x, y);
    INPUT input[2] = {};
    input[0].type = INPUT_MOUSE;
    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    input[1].type = INPUT_MOUSE;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, input, sizeof(INPUT));
}

void ClickStashTabButton(int tabIndex)
{
    int x = 94 + tabIndex * 98;
    int y = 98;
    ClickAtScreenPos(x, y);
}

bool AutoStocker::IsRunning() const
{
    return m_State != StockerState::Idle &&
        m_State != StockerState::Complete &&
        m_State != StockerState::Error;
}

void AutoStocker::LogMessage(const char* fmt, ...)
{
    char buf[512];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    printf("[AutoStocker] %s\n", buf);
}

D2UnitStrc* AutoStocker::FindPlayerUnit()
{
    uintptr_t base = Pattern::Base();
    uintptr_t table = base + D2ROffsets::UnitHashTable;
    for (uint32_t i = 0; i < D2ROffsets::HashBuckets; i++)
    {
        D2UnitStrc* unit = *reinterpret_cast<D2UnitStrc**>(table + i * 8);
        while (unit)
        {
            if (unit->dwUnitType == 0)
                return unit;
            unit = unit->pListNext;
        }
    }
    return nullptr;
}

std::vector<ItemInfo> AutoStocker::FindItemsByPage(uint8_t page)
{
    std::vector<ItemInfo> result;
    uintptr_t base = Pattern::Base();
    uintptr_t table = base + D2ROffsets::UnitHashTable + (D2ROffsets::UnitTypeItem * 0x400);

    for (uint32_t i = 0; i < D2ROffsets::HashBuckets; i++)
    {
        D2UnitStrc* unit = *reinterpret_cast<D2UnitStrc**>(table + i * 8);
        while (unit)
        {
            if (unit->dwUnitType == D2ROffsets::UnitTypeItem && unit->pItemData)
            {
                uint8_t itemPage = *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(unit->pItemData) + 0xB8);
                if (itemPage == page)
                {
                    uint32_t cid = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(unit->pItemData) + 0x0C);
                    result.push_back({ unit->dwUnitId, unit->dwClassId, page, cid });
                }
            }
            unit = unit->pListNext;
        }
    }
    return result;
}

std::vector<ItemInfo> AutoStocker::FindCrystalsInCurrentContext(uint8_t page, uint32_t containerId)
{
    std::vector<ItemInfo> result;
    auto items = FindItemsByPage(page);
    for (const auto& item : items)
    {
        if (item.containerId == containerId && ESRClassId::IsCrystal(item.classId))
            result.push_back(item);
    }
    return result;
}

ItemInfo AutoStocker::FindStocker()
{
    auto inv = FindItemsByPage(ItemPage::Inventory);
    for (const auto& item : inv)
        if (ESRClassId::IsCrystalCan(item.classId))
            return item;
    auto stash = FindItemsByPage(ItemPage::Stash);
    for (const auto& item : stash)
        if (ESRClassId::IsCrystalCan(item.classId))
            return item;
    return { 0, 0, 0, 0 };
}

ItemInfo AutoStocker::FindCubeInInventory()
{
    auto items = FindItemsByPage(ItemPage::Inventory);
    for (const auto& item : items)
        if (item.classId == ESRClassId::HoradricCube)
            return item;
    return { 0, 0, 0, 0 };
}

bool AutoStocker::GetSharedTabBaseId(uint32_t& outBase)
{
    D2UnitStrc* player = FindPlayerUnit();
    if (!player) return false;
    uint8_t* pInv = reinterpret_cast<uint8_t*>(player->pInventory);
    if (!pInv) return false;
    uint8_t* tab1 = *reinterpret_cast<uint8_t**>(pInv + 0x68);
    if (!tab1) return false;
    outBase = *reinterpret_cast<uint32_t*>(tab1);
    return true;
}

void AutoStocker::Start()
{
    if (IsRunning())
    {
        LogMessage("Already running");
        return;
    }

    auto cube = FindCubeInInventory();
    if (cube.unitId == 0)
    {
        LogMessage("No cube in inventory, aborting");
        return;
    }
    m_CubeUnitId = cube.unitId;

    uint32_t gx = 0, gy = 0;
    if (!D2RFunctions::GetCubeGridPos(m_CubeUnitId, gx, gy))
    {
        LogMessage("Cannot read cube grid position");
        return;
    }
    InventoryGridToScreen(gx, gy, m_CubeClickX, m_CubeClickY);

    auto cubeItems = FindItemsByPage(ItemPage::Cube);
    bool stockerAlreadyInCube = false;
    for (const auto& it : cubeItems)
    {
        if (ESRClassId::IsCrystalCan(it.classId))
        {
            stockerAlreadyInCube = true;
            m_StockerUnitId = it.unitId;
        }
        else
        {
            LogMessage("Cube is not empty (unitId=%u classId=%u). Abort.",
                it.unitId, it.classId);
            return;
        }
    }

    if (!stockerAlreadyInCube)
    {
        auto stocker = FindStocker();
        if (stocker.unitId == 0)
        {
            LogMessage("No stocker found");
            return;
        }
        m_StockerUnitId = stocker.unitId;
    }

    m_TotalStocked = 0;
    m_CrystalIndex = 0;
    m_Crystals.clear();
    m_TabIndex = -1;
    m_StashProcessed = false;
    m_TickDelay = 0;
    m_CurrentPage = ItemPage::Inventory;
    m_CurrentContainerId = 1;

    LogMessage("Starting. Cube=%u Stocker=%u AlreadyInCube=%d",
        m_CubeUnitId, m_StockerUnitId, stockerAlreadyInCube ? 1 : 0);
    m_State = stockerAlreadyInCube ? StockerState::WaitStockerInCube : StockerState::Start;
}

void AutoStocker::Stop()
{
    if (IsRunning())
    {
        LogMessage("Stopped. Total stocked=%d", m_TotalStocked);
        m_State = StockerState::Idle;
    }
}

void AutoStocker::Tick()
{
    if (!IsRunning()) return;
    if (m_TickDelay > 0) { m_TickDelay--; return; }

    switch (m_State)
    {
    case StockerState::Start:
        m_State = StockerState::PickupStocker;
        break;

    case StockerState::PickupStocker:
    {
        D2UnitStrc* pStocker = D2RFunctions::FindItemById(m_StockerUnitId);
        if (!pStocker || !pStocker->pItemData) { m_State = StockerState::Error; break; }
        uint8_t page = *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(pStocker->pItemData) + 0xB8);
        uint32_t cid = *reinterpret_cast<uint32_t*>(reinterpret_cast<uint8_t*>(pStocker->pItemData) + 0x0C);
        LogMessage("Pickup stocker %u (page=%u cid=%u)", m_StockerUnitId, page, cid);
        if (page == ItemPage::Stash && cid != 1)
            D2RFunctions::PickItemFromSharedStash(m_StockerUnitId);
        else
            D2RFunctions::PickItemFromBuffer(m_StockerUnitId);
        m_TickDelay = 1;
        m_State = StockerState::ClickCubeForStocker;
        break;
    }

    case StockerState::ClickCubeForStocker:
        ClickAtScreenPos(m_CubeClickX, m_CubeClickY);
        m_TickDelay = 1;
        m_State = StockerState::WaitStockerInCube;
        break;

    case StockerState::WaitStockerInCube:
        m_CurrentPage = ItemPage::Inventory;
        m_CurrentContainerId = 1;
        m_Crystals = FindCrystalsInCurrentContext(m_CurrentPage, m_CurrentContainerId);
        m_CrystalIndex = 0;
        LogMessage("Inventory crystals: %d", (int)m_Crystals.size());
        m_State = StockerState::NextCrystal;
        break;

    case StockerState::NextCrystal:
    {
        if (m_CrystalIndex >= (int)m_Crystals.size())
        {
            m_State = StockerState::SwitchTab;
            break;
        }
        m_State = StockerState::PickupCrystal;
        break;
    }

    case StockerState::PickupCrystal:
    {
        auto& c = m_Crystals[m_CrystalIndex];
        LogMessage("Pickup crystal %u (%d/%d) containerId=%u", c.unitId,
            m_CrystalIndex + 1, (int)m_Crystals.size(), c.containerId);
        if (c.page == ItemPage::Stash && c.containerId != 1)
            D2RFunctions::PickItemFromSharedStash(c.unitId);
        else
            D2RFunctions::PickItemFromBuffer(c.unitId);
        m_TickDelay = 1;
        m_State = StockerState::ClickCubeForCrystal;
        break;
    }

    case StockerState::ClickCubeForCrystal:
        ClickAtScreenPos(m_CubeClickX, m_CubeClickY);
        m_TickDelay = 3;
        m_State = StockerState::WaitCrystalInCube;
        break;

    case StockerState::WaitCrystalInCube:
        m_State = StockerState::Transmute;
        break;

    case StockerState::Transmute:
        D2RFunctions::D2CLIENT_Transmute();
        m_TotalStocked++;
        m_CrystalIndex++;
        m_TickDelay = 1;
        m_State = StockerState::WaitTransmute;
        break;

    case StockerState::WaitTransmute:
        m_State = StockerState::NextCrystal;
        break;

    case StockerState::SwitchTab:
    {
        bool stashOpen = BankPanelHook::IsStashOpen();
        if (!stashOpen)
        {
            LogMessage("Stash not open, done. Total=%d", m_TotalStocked);
            m_State = StockerState::Complete;
            break;
        }

        if (m_TabIndex == -1)
        {
            m_TabIndex = 0;
            LogMessage("Switching to Personal tab");
            ClickStashTabButton(m_TabIndex);
            m_CurrentPage = ItemPage::Stash;
            m_CurrentContainerId = 1;
        }
        else
        {
            m_TabIndex++;
            if (m_TabIndex > 7)
            {
                LogMessage("All tabs processed. Total=%d", m_TotalStocked);
                m_State = StockerState::Complete;
                break;
            }
            LogMessage("Switching to Shared %d", m_TabIndex);
            ClickStashTabButton(m_TabIndex);
            m_CurrentPage = ItemPage::Stash;
            uint32_t base = 0;
            if (!GetSharedTabBaseId(base))
            {
                LogMessage("Failed to read shared tab base id, skipping");
                m_TickDelay = 3;
                m_State = StockerState::SwitchTab;
                break;
            }
            m_CurrentContainerId = base + (m_TabIndex - 1);
        }
        m_TickDelay = 3;
        m_State = StockerState::WaitTabSwitched;
        break;
    }

    case StockerState::WaitTabSwitched:
    {
        m_Crystals = FindCrystalsInCurrentContext(m_CurrentPage, m_CurrentContainerId);
        m_CrystalIndex = 0;
        LogMessage("Tab %d: %d crystals found (containerId=%u)",
            m_TabIndex, (int)m_Crystals.size(), m_CurrentContainerId);
        if (m_Crystals.empty())
            m_State = StockerState::SwitchTab;
        else
            m_State = StockerState::NextCrystal;
        break;
    }

    case StockerState::Complete:
        LogMessage("Complete. Total stocked=%d", m_TotalStocked);
        m_State = StockerState::Idle;
        break;

    case StockerState::Error:
        LogMessage("Error, stopping");
        m_State = StockerState::Idle;
        break;

    default:
        break;
    }
}