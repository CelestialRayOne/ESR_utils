#pragma once
#include <cstdint>
#include <mutex>
#include <string>

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

        bool storeRerollMagicRings = false;
        bool storeRerollMagicAmulets = false;
        bool storeRerollMagicJewels = false;
        bool storeRerollMagicCharms = false;
        bool storeRerollMagicQuivers = false;

        bool storeRerollRareRings = false;
        bool storeRerollRareAmulets = false;
        bool storeRerollRareJewels = false;
        bool storeRerollRareCharms = false;
        bool storeRerollRareQuivers = false;

        bool storeRerollSetUniqueRings = false;
        bool storeRerollSetUniqueAmulets = false;
        bool storeRerollSetUniqueQuivers = false;

        bool rerollSkipInventory = true;
        bool rerollSkipStash = false;
        std::string aspectRatio = "16:9";
    };

    bool Init();
    Hotkeys GetHotkeys();
    Flags GetFlags();
}