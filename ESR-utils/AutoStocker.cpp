#include "AutoStocker.h"
#include "BankPanelHook.h"
#include "InventoryPanelHook.h"
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

void CubeGridToScreen(uint32_t gx, uint32_t gy, int& sx, int& sy)
{
    constexpr double SLOT = 49.0;
    constexpr double ORIGIN_X = 67.0;
    constexpr double ORIGIN_Y = 119.0;
    sx = static_cast<int>(ORIGIN_X + gx * SLOT);
    sy = static_cast<int>(ORIGIN_Y + gy * SLOT);
}

void ClickAtScreenPos(int x, int y)
{
    SetCursorPos(x, y);
    INPUT i[2] = {};
    i[0].type = INPUT_MOUSE; i[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    i[1].type = INPUT_MOUSE; i[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, i, sizeof(INPUT));
}

void CtrlClickAtScreenPos(int x, int y)
{
    SetCursorPos(x, y);
    INPUT i[4] = {};
    i[0].type = INPUT_KEYBOARD; i[0].ki.wVk = VK_CONTROL;
    i[1].type = INPUT_MOUSE;    i[1].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    i[2].type = INPUT_MOUSE;    i[2].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    i[3].type = INPUT_KEYBOARD; i[3].ki.wVk = VK_CONTROL; i[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, i, sizeof(INPUT));
}

void RightClickAtScreenPos(int x, int y)
{
    SetCursorPos(x, y);
    INPUT i[2] = {};
    i[0].type = INPUT_MOUSE; i[0].mi.dwFlags = MOUSEEVENTF_RIGHTDOWN;
    i[1].type = INPUT_MOUSE; i[1].mi.dwFlags = MOUSEEVENTF_RIGHTUP;
    SendInput(2, i, sizeof(INPUT));
}

void PressEscape()
{
    INPUT i[2] = {};
    i[0].type = INPUT_KEYBOARD; i[0].ki.wVk = VK_ESCAPE;
    i[1].type = INPUT_KEYBOARD; i[1].ki.wVk = VK_ESCAPE; i[1].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(2, i, sizeof(INPUT));
}

void ClickStashTabButton(int tabIndex)
{
    int x = 94 + tabIndex * 98;
    int y = 98;
    ClickAtScreenPos(x, y);
}

bool AutoStocker::IsRunning() const
{
    return m_State != StockerState::Idle
        && m_State != StockerState::Complete
        && m_State != StockerState::Error;
}

void AutoStocker::LogMessage(const char* fmt, ...)
{
    char buf[512];
    va_list args; va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args); va_end(args);
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
            if (unit->dwUnitType == 0) return unit;
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
                uint8_t ipage = *reinterpret_cast<uint8_t*>(reinterpret_cast<uint8_t*>(unit->pItemData) + 0xB8);
                if (ipage == page)
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

ItemInfo AutoStocker::FindCubeInInventory()
{
    auto items = FindItemsByPage(ItemPage::Inventory);
    for (const auto& it : items)
        if (it.classId == ESRClassId::HoradricCube) return it;
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

bool AutoStocker::IsStockerClassOfType(uint32_t c, StockerType t)
{
    switch (t)
    {
        case StockerType::Crystals:     return ESRClassId::IsCrystalCan(c);
        case StockerType::Gems:         return ESRClassId::IsGemCan(c);
        case StockerType::Runes:        return ESRClassId::IsRuneStocker(c);
        case StockerType::Decals:       return ESRClassId::IsDecalStocker(c);
        case StockerType::Multistocker: return ESRClassId::IsMultistocker(c);
        case StockerType::Reroll:       return ESRClassId::IsRerollOrb(c);
    }
    return false;
}

bool AutoStocker::IsConsumableOfType(uint32_t c, StockerType t, bool coupons)
{
    switch (t)
    {
    case StockerType::Crystals: return ESRClassId::IsCrystal(c);
    case StockerType::Gems:     return ESRClassId::IsGem(c);
    case StockerType::Runes:    return ESRClassId::IsRune(c);
    case StockerType::Decals:   return ESRClassId::IsDecal(c);
    case StockerType::Multistocker:
        if (ESRClassId::IsMultiItem(c)) return true;
        if (coupons && ESRClassId::IsCouponClassic(c)) return true;
        return false;
    case StockerType::Reroll:
        return ESRClassId::IsRerollTargetClass(c);
    }
    return false;
}

const char* AutoStocker::TypeName(StockerType t)
{
    switch (t)
    {
    case StockerType::Crystals: return "Crystals";
    case StockerType::Gems: return "Gems";
    case StockerType::Runes: return "Runes";
    case StockerType::Decals: return "Decals";
    case StockerType::Multistocker: return "Multistocker";
    case StockerType::Reroll: return "Reroll";
    }
    return "?";
}

ItemInfo AutoStocker::FindStockerOfType(StockerType t)
{
    auto inv = FindItemsByPage(ItemPage::Inventory);
    for (const auto& it : inv)
        if (IsStockerClassOfType(it.classId, t)) return it;
    auto stash = FindItemsByPage(ItemPage::Stash);
    for (const auto& it : stash)
        if (IsStockerClassOfType(it.classId, t)) return it;
    return { 0, 0, 0, 0 };
}

std::vector<ItemInfo> AutoStocker::FindConsumablesInContext(StockerType t, uint8_t page, uint32_t containerId)
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
                uint8_t* pd = reinterpret_cast<uint8_t*>(unit->pItemData);
                uint8_t itemPage = *(pd + 0xB8);
                if (itemPage == page)
                {
                    uint32_t cid = *reinterpret_cast<uint32_t*>(pd + 0x0C);
                    if (cid == containerId && IsConsumableOfType(unit->dwClassId, t, m_Req.storeNonBlankCoupons))
                    {
                        bool accept = true;
                        if (t == StockerType::Reroll)
                        {
                            uint32_t rarity = *reinterpret_cast<uint32_t*>(pd);
                            bool wantMagic = m_Req.rerollMagic && rarity == ESRClassId::Rarity::Magic;
                            bool wantRare = m_Req.rerollRare && rarity == ESRClassId::Rarity::Rare;
                            accept = wantMagic || wantRare;
                        }
                        if (accept)
                            result.push_back({ unit->dwUnitId, unit->dwClassId, page, cid });
                    }
                }
            }
            unit = unit->pListNext;
        }
    }
    return result;
}

void AutoStocker::Start(const RunRequest& req)
{
    if (IsRunning()) { LogMessage("Already running"); return; }

    if (!InventoryPanelHook::IsInventoryOpen())
    {
        LogMessage("Inventory is not open, abort");
        return;
    }

    m_Req = req;

    m_TypeQueue.clear();
    if (req.crystals)     m_TypeQueue.push_back(StockerType::Crystals);
    if (req.gems)         m_TypeQueue.push_back(StockerType::Gems);
    if (req.runes)        m_TypeQueue.push_back(StockerType::Runes);
    if (req.decals)       m_TypeQueue.push_back(StockerType::Decals);
    if (req.multistocker) m_TypeQueue.push_back(StockerType::Multistocker);
    if (req.reroll)       m_TypeQueue.push_back(StockerType::Reroll);

    if (m_TypeQueue.empty()) { LogMessage("No stockers checked"); return; }

    auto cube = FindCubeInInventory();
    if (cube.unitId == 0) { LogMessage("No cube in inventory, abort"); return; }
    m_CubeUnitId = cube.unitId;

    uint32_t gx = 0, gy = 0;
    if (!D2RFunctions::GetCubeGridPos(m_CubeUnitId, gx, gy))
    {
        LogMessage("Cannot read cube grid pos"); return;
    }
    InventoryGridToScreen(gx, gy, m_CubeClickX, m_CubeClickY);

    m_TypeIndex = 0;
    m_TotalStocked = 0;
    m_TickDelay = 0;

    LogMessage("Starting. %d stocker type(s) queued. Cube unitId=%u at (%u,%u)",
        (int)m_TypeQueue.size(), m_CubeUnitId, gx, gy);
    m_State = StockerState::NextStockerType;
}

void AutoStocker::Stop()
{
    if (IsRunning())
    {
        LogMessage("Stopped. Total stocked=%d", m_TotalStocked);
        m_State = StockerState::Idle;
    }
}

void AutoStocker::BeginStockerType(StockerType t)
{
    m_CurrentType = t;
    m_StockerUnitId = 0;
    m_Consumables.clear();
    m_ConsumableIndex = 0;
    m_TabIndex = -1;
    m_CurrentPage = ItemPage::Inventory;
    m_CurrentContainerId = 1;
}

void AutoStocker::Tick()
{
    if (!IsRunning()) return;
    if (m_TickDelay > 0) { m_TickDelay--; return; }

    switch (m_State)
    {
    case StockerState::NextStockerType:
    {
        if (m_TypeIndex >= (int)m_TypeQueue.size())
        {
            m_State = StockerState::Complete;
            break;
        }
        BeginStockerType(m_TypeQueue[m_TypeIndex]);
        LogMessage("=== Starting %s ===", TypeName(m_CurrentType));
        m_State = StockerState::VerifyCubeEmpty;
        break;
    }

    case StockerState::VerifyCubeEmpty:
    {
        auto cubeItems = FindItemsByPage(ItemPage::Cube);
        if (!cubeItems.empty())
        {
            LogMessage("Cube is not empty (%d items). Abort.", (int)cubeItems.size());
            m_State = StockerState::Error;
            break;
        }
        auto st = FindStockerOfType(m_CurrentType);
        if (st.unitId == 0)
        {
            LogMessage("No %s stocker found, skip", TypeName(m_CurrentType));
            m_TypeIndex++;
            m_State = StockerState::NextStockerType;
            break;
        }
        m_StockerUnitId = st.unitId;
        m_State = StockerState::PickupStocker;
        break;
    }

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
        m_TickDelay = 2;
        m_State = StockerState::WaitStockerInCube;
        break;

    case StockerState::WaitStockerInCube:
        m_CurrentPage = ItemPage::Inventory;
        m_CurrentContainerId = 1;
        m_Consumables = FindConsumablesInContext(m_CurrentType, m_CurrentPage, m_CurrentContainerId);
        m_ConsumableIndex = 0;
        LogMessage("Inventory consumables: %d", (int)m_Consumables.size());
        m_State = StockerState::NextConsumable;
        break;

    case StockerState::NextConsumable:
    {
        if (m_ConsumableIndex >= (int)m_Consumables.size())
        {
            m_State = StockerState::NextTabContext;
            break;
        }
        m_State = StockerState::PickupConsumable;
        break;
    }

    case StockerState::PickupConsumable:
    {
        auto& c = m_Consumables[m_ConsumableIndex];
        LogMessage("Pickup consumable %u (%d/%d) cid=%u", c.unitId,
            m_ConsumableIndex + 1, (int)m_Consumables.size(), c.containerId);
        if (c.page == ItemPage::Stash && c.containerId != 1)
            D2RFunctions::PickItemFromSharedStash(c.unitId);
        else
            D2RFunctions::PickItemFromBuffer(c.unitId);
        m_TickDelay = 1;
        m_State = StockerState::ClickCubeForConsumable;
        break;
    }

    case StockerState::ClickCubeForConsumable:
        ClickAtScreenPos(m_CubeClickX, m_CubeClickY);
        m_TickDelay = 1;
        m_State = StockerState::WaitConsumableInCube;
        break;

    case StockerState::WaitConsumableInCube:
        m_State = StockerState::Transmute;
        break;

    case StockerState::Transmute:
        D2RFunctions::D2CLIENT_Transmute();
        m_TotalStocked++;
        m_ConsumableIndex++;
        m_TickDelay = 1;
        m_State = StockerState::WaitTransmute;
        break;

    case StockerState::WaitTransmute:
        m_State = StockerState::NextConsumable;
        break;

    case StockerState::NextTabContext:
    {
        bool stashOpen = BankPanelHook::IsStashOpen();
        if (!stashOpen)
        {
            m_State = StockerState::EjectStocker_OpenCube;
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
                m_State = StockerState::EjectStocker_OpenCube;
                break;
            }
            LogMessage("Switching to Shared %d", m_TabIndex);
            ClickStashTabButton(m_TabIndex);
            m_CurrentPage = ItemPage::Stash;
            uint32_t base = 0;
            if (!GetSharedTabBaseId(base))
            {
                LogMessage("Failed to read shared base, skip");
                m_TickDelay = 5;
                m_State = StockerState::NextTabContext;
                break;
            }
            m_CurrentContainerId = base + (m_TabIndex - 1);
        }
        m_TickDelay = 2;
        m_State = StockerState::WaitTabSwitched;
        break;
    }

    case StockerState::WaitTabSwitched:
    {
        m_Consumables = FindConsumablesInContext(m_CurrentType, m_CurrentPage, m_CurrentContainerId);
        m_ConsumableIndex = 0;
        LogMessage("Tab %d: %d consumables (cid=%u)",
            m_TabIndex, (int)m_Consumables.size(), m_CurrentContainerId);
        if (m_Consumables.empty())
            m_State = StockerState::NextTabContext;
        else
            m_State = StockerState::NextConsumable;
        break;
    }

    case StockerState::EjectStocker_OpenCube:
        LogMessage("Eject: right-click cube");
        RightClickAtScreenPos(m_CubeClickX, m_CubeClickY);
        m_TickDelay = 3;
        m_State = StockerState::EjectStocker_CtrlClickStocker;
        break;

    case StockerState::EjectStocker_CtrlClickStocker:
    {
        int sx, sy;
        CubeGridToScreen(15, 12, sx, sy);
        LogMessage("Eject: ctrl-click cube slot (15,12) at (%d,%d)", sx, sy);
        CtrlClickAtScreenPos(sx, sy);
        m_TickDelay = 2;
        m_State = StockerState::EjectStocker_CloseCube;
        break;
    }

    case StockerState::EjectStocker_CloseCube:
        if (BankPanelHook::IsStashOpen())
        {
            LogMessage("Eject: close cube (ESC)");
            PressEscape();
        }
        else
        {
            LogMessage("Eject: stash closed, leaving cube open");
        }
        m_TickDelay = 2;
        m_TypeIndex++;
        m_State = StockerState::WaitBetweenStockerTypes;
        break;

    case StockerState::WaitBetweenStockerTypes:
        m_State = StockerState::NextStockerType;
        break;

    case StockerState::Complete:
        LogMessage("All done. Total stocked=%d", m_TotalStocked);
        m_State = StockerState::Idle;
        break;

    case StockerState::Error:
        LogMessage("Error, stopping");
        m_State = StockerState::Idle;
        break;

    default: break;
    }
}