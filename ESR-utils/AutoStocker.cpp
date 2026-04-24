#include "AutoStocker.h"
#include "BankPanelHook.h"
#include "InventoryPanelHook.h"
#include "Pattern.h"
#include "Config.h"
#include <Windows.h>
#include <cstdio>
#include <cstdarg>

static double ScaleVirtualToPixel()
{
    auto flags = Config::GetFlags();
    double w = (double)GetSystemMetrics(SM_CXSCREEN);
    double h = (double)GetSystemMetrics(SM_CYSCREEN);
    if (flags.aspectRatio == "21:9")
    {
        // Empirical: D2R scales UI so 98 virtual units = 64 pixels at 3440x1440
        return h / 2205.0;
    }
    // 16:9 default
    double sx = w / 3840.0;
    double sy = h / 2160.0;
    return (sx < sy) ? sx : sy;
}

static double ViewportAspect()
{
    auto flags = Config::GetFlags();
    if (flags.aspectRatio == "21:9") return 2.09;  // empirical D2R cap on 21:9 displays
    return 16.0 / 9.0;
}

static double ViewportWidth()
{
    double h = (double)GetSystemMetrics(SM_CYSCREEN);
    double w = (double)GetSystemMetrics(SM_CXSCREEN);
    double aspect = ViewportAspect();
    double cap_w = h * aspect;
    return (w < cap_w) ? w : cap_w;
}

static double ViewportHeight()
{
    double h = (double)GetSystemMetrics(SM_CYSCREEN);
    double w = (double)GetSystemMetrics(SM_CXSCREEN);
    double aspect = ViewportAspect();
    double cap_h = w / aspect;
    return (h < cap_h) ? h : cap_h;
}

static double LeftBar()
{
    return (GetSystemMetrics(SM_CXSCREEN) - ViewportWidth()) / 2.0;
}

static double TopBar()
{
    return (GetSystemMetrics(SM_CYSCREEN) - ViewportHeight()) / 2.0;
}

void InventoryGridToScreen(uint32_t gx, uint32_t gy, int& sx, int& sy)
{
    double s = ScaleVirtualToPixel();
    double panelX = LeftBar() + ViewportWidth() + (-1140.0 * s);
    double panelY = TopBar() + 0.397 * ViewportHeight() + (-856.0 * s);
    double gridX = panelX + 93.0 * s;
    double gridY = panelY + 819.0 * s;
    sx = static_cast<int>(gridX + gx * 98.0 * s + 49.0 * s);
    sy = static_cast<int>(gridY + gy * 98.0 * s + 49.0 * s);
}

void StashGridToScreen(uint32_t gx, uint32_t gy, int& sx, int& sy)
{
    double s = ScaleVirtualToPixel();
    double panelX = LeftBar() + 0.0 * s;
    double panelY = TopBar() + 0.397 * ViewportHeight() + (-856.0 * s);
    double gridX = panelX + 95.0 * s;
    double gridY = panelY + 235.0 * s;
    sx = static_cast<int>(gridX + gx * 98.0 * s + 49.0 * s);
    sy = static_cast<int>(gridY + gy * 98.0 * s + 49.0 * s);
}

void CubeGridToScreen(uint32_t gx, uint32_t gy, int& sx, int& sy)
{
    double s = ScaleVirtualToPixel();
    double panelX = LeftBar() + 0.0 * s;
    double panelY = TopBar() + 0.397 * ViewportHeight() + (-856.0 * s);
    double gridX = panelX + 80.0 * s;
    double gridY = panelY + 193.0 * s;
    sx = static_cast<int>(gridX + gx * 98.0 * s + 49.0 * s);
    sy = static_cast<int>(gridY + gy * 98.0 * s + 49.0 * s);
}

void ClickStashTabButton(int tabIndex)
{
    double s = ScaleVirtualToPixel();
    int x = static_cast<int>(LeftBar() + 188.0 * s + tabIndex * 196.0 * s);
    int y = static_cast<int>(TopBar() + 196.0 * s);
    ClickAtScreenPos(x, y);
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

bool AutoStocker::IsRunning() const
{
    return m_State != StockerState::Idle
        && m_State != StockerState::Complete
        && m_State != StockerState::Error;
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

    if (BankPanelHook::IsStashOpen())
    {
        auto stash = FindItemsByPage(ItemPage::Stash);
        for (const auto& it : stash)
            if (IsStockerClassOfType(it.classId, t)) return it;
    }

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
                            if (m_Req.rerollSkipInventory && page == ItemPage::Inventory)
                            {
                                accept = false;
                            }
                            else if (m_Req.rerollSkipStash && page == ItemPage::Stash && cid == 1)
                            {
                                accept = false;
                            }
                            else
                            {
                                uint32_t rarity = *reinterpret_cast<uint32_t*>(pd);
                                ESRClassId::RerollType rtype = ESRClassId::GetRerollType(unit->dwClassId);
                                accept = false;

                                if (rarity == ESRClassId::Rarity::Magic)
                                {
                                    switch (rtype)
                                    {
                                    case ESRClassId::RerollType::Ring:   accept = m_Req.rerollMagicRings; break;
                                    case ESRClassId::RerollType::Amulet: accept = m_Req.rerollMagicAmulets; break;
                                    case ESRClassId::RerollType::Jewel:  accept = m_Req.rerollMagicJewels; break;
                                    case ESRClassId::RerollType::Charm:  accept = m_Req.rerollMagicCharms; break;
                                    case ESRClassId::RerollType::Quiver: accept = m_Req.rerollMagicQuivers; break;
                                    default: break;
                                    }
                                }
                                else if (rarity == ESRClassId::Rarity::Rare)
                                {
                                    switch (rtype)
                                    {
                                    case ESRClassId::RerollType::Ring:   accept = m_Req.rerollRareRings; break;
                                    case ESRClassId::RerollType::Amulet: accept = m_Req.rerollRareAmulets; break;
                                    case ESRClassId::RerollType::Jewel:  accept = m_Req.rerollRareJewels; break;
                                    case ESRClassId::RerollType::Charm:  accept = m_Req.rerollRareCharms; break;
                                    case ESRClassId::RerollType::Quiver: accept = m_Req.rerollRareQuivers; break;
                                    default: break;
                                    }
                                }
                                else if (rarity == ESRClassId::Rarity::Set || rarity == ESRClassId::Rarity::Unique)
                                {
                                    switch (rtype)
                                    {
                                    case ESRClassId::RerollType::Ring:   accept = m_Req.rerollSetUniqueRings; break;
                                    case ESRClassId::RerollType::Amulet: accept = m_Req.rerollSetUniqueAmulets; break;
                                    case ESRClassId::RerollType::Quiver: accept = m_Req.rerollSetUniqueQuivers; break;
                                    default: break;
                                    }
                                }
                            }
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
    if (IsRunning())
        return;

    if (!InventoryPanelHook::IsInventoryOpen())
    {
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

    if (m_TypeQueue.empty())
        return;

    auto cube = FindCubeInInventory();
    if (cube.unitId == 0)
        return;
    m_CubeUnitId = cube.unitId;

    uint32_t gx = 0, gy = 0;
    if (!D2RFunctions::GetCubeGridPos(m_CubeUnitId, gx, gy))
        return;

    InventoryGridToScreen(gx, gy, m_CubeClickX, m_CubeClickY);

    m_TypeIndex = 0;
    m_TotalStocked = 0;
    m_TickDelay = 0;
    m_State = StockerState::NextStockerType;
}

void AutoStocker::Stop()
{
    if (IsRunning())
        m_State = StockerState::Idle;
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
        m_State = StockerState::VerifyCubeEmpty;
        break;
    }

    case StockerState::VerifyCubeEmpty:
    {
        auto cubeItems = FindItemsByPage(ItemPage::Cube);
        if (!cubeItems.empty())
        {
            m_State = StockerState::Error;
            break;
        }
        auto st = FindStockerOfType(m_CurrentType);
        if (st.unitId == 0)
        {
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
            ClickStashTabButton(m_TabIndex);
            m_CurrentPage = ItemPage::Stash;
            uint32_t base = 0;
            if (!GetSharedTabBaseId(base))
            {
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
        if (m_Consumables.empty())
            m_State = StockerState::NextTabContext;
        else
            m_State = StockerState::NextConsumable;
        break;
    }

    case StockerState::EjectStocker_OpenCube:
        RightClickAtScreenPos(m_CubeClickX, m_CubeClickY);
        m_TickDelay = 3;
        m_State = StockerState::EjectStocker_CtrlClickStocker;
        break;

    case StockerState::EjectStocker_CtrlClickStocker:
    {
        int sx, sy;
        CubeGridToScreen(15, 12, sx, sy);
        CtrlClickAtScreenPos(sx, sy);
        m_TickDelay = 2;
        m_State = StockerState::EjectStocker_CloseCube;
        break;
    }

    case StockerState::EjectStocker_CloseCube:
        if (BankPanelHook::IsStashOpen())
            PressEscape();
        m_TickDelay = 2;
        m_TypeIndex++;
        m_State = StockerState::WaitBetweenStockerTypes;
        break;

    case StockerState::WaitBetweenStockerTypes:
        m_State = StockerState::NextStockerType;
        break;

    case StockerState::Complete:
        m_State = StockerState::Idle;
        break;

    case StockerState::Error:
        m_State = StockerState::Idle;
        break;

    default: break;
    }
}