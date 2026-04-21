#pragma once
#include <cstdint>
#include <mutex>

namespace Config
{
    struct Hotkey
    {
        uint32_t vk = 0;
        bool ctrl = false;
        bool alt = false;
        bool shift = false;
        bool IsSet() const { return vk != 0; }
    };

    struct Hotkeys
    {
        Hotkey storeItems;
        Hotkey emergencyStop;
    };

    struct Flags
    {
        bool storeCrystals = true;
        bool storeGems = true;
        bool storeRunes = true;
        bool storeDecals = true;
        bool storeMultistocker = true;
        bool storeNonBlankCoupons = false;
        bool storeRerollMagic = false;
        bool storeRerollRare = false;
    };

    bool Init();
    Hotkeys GetHotkeys();
    Flags GetFlags();
}