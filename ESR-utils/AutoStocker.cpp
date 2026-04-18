#include "AutoStocker.h"
#include "Pattern.h"
#include <cstdio>
#include <cstdarg>

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
                if (unit->pItemData->nPage == page)
                {
                    result.push_back({ unit->dwUnitId, unit->dwClassId, page });
                }
            }
            unit = unit->pListNext;
        }
    }
    return result;
}

std::vector<ItemInfo> AutoStocker::FindCrystalsInInventory()
{
    std::vector<ItemInfo> result;
    auto items = FindItemsByPage(ItemPage::Inventory);
    for (const auto& item : items)
    {
        if (ESRClassId::IsCrystal(item.classId))
            result.push_back(item);
    }
    return result;
}

ItemInfo AutoStocker::FindStockerInInventory()
{
    auto items = FindItemsByPage(ItemPage::Inventory);
    for (const auto& item : items)
    {
        if (item.classId == ESRClassId::CrystalCan)
            return item;
    }
    return { 0, 0, 0 };
}

ItemInfo AutoStocker::FindStockerInCube()
{
    auto items = FindItemsByPage(ItemPage::Cube);
    for (const auto& item : items)
    {
        if (item.classId == ESRClassId::CrystalCan)
            return item;
    }
    return { 0, 0, 0 };
}

void AutoStocker::Start()
{
    if (IsRunning())
    {
        LogMessage("Already running");
        return;
    }

    m_Crystals = FindCrystalsInInventory();
    if (m_Crystals.empty())
    {
        LogMessage("No crystals found in inventory");
        m_State = StockerState::Error;
        return;
    }

    auto stocker = FindStockerInInventory();
    if (stocker.unitId == 0)
    {
        LogMessage("No Crystal Can found in inventory");
        m_State = StockerState::Error;
        return;
    }

    m_StockerUnitId = stocker.unitId;
    m_CrystalIndex = 0;
    m_TotalStocked = 0;
    m_TickDelay = 0;

    LogMessage("Starting: %d crystals to stock", (int)m_Crystals.size());
    m_State = StockerState::Starting;
}

void AutoStocker::Stop()
{
    if (IsRunning())
    {
        LogMessage("Stopped. Stocked %d crystals", m_TotalStocked);
        m_State = StockerState::Idle;
    }
}

void AutoStocker::Tick()
{
    if (!IsRunning())
        return;

    if (m_TickDelay > 0)
    {
        m_TickDelay--;
        return;
    }

    switch (m_State)
    {
    case StockerState::Starting:
        LogMessage("Ready to begin. Stocker unitId=%u, %d crystals queued",
            m_StockerUnitId, (int)m_Crystals.size());
        m_State = StockerState::Complete;
        break;

    case StockerState::Complete:
        LogMessage("Complete. Stocked %d crystals", m_TotalStocked);
        m_State = StockerState::Idle;
        break;

    case StockerState::Error:
        LogMessage("Error occurred, stopping");
        m_State = StockerState::Idle;
        break;

    default:
        break;
    }
}