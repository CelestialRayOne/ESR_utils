#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include "Pattern.h"
#include "D2RFunctions.h"
#include "AutoStocker.h"
#include "BankPanelHook.h"
#include "Config.h"
#include "InventoryPanelHook.h"
#include "MinHook.h"

static AutoStocker g_Stocker;
static HWND g_GameWindow = NULL;
static WNDPROC g_OriginalWndProc = NULL;

static uint32_t g_testCrystalId = 0;
static uint32_t g_testCubeId = 0;
static int g_testStep = 0;
static int g_testClickX = 0;
static int g_testClickY = 0;

static void ClickAtScreenPos(int x, int y)
{
    SetCursorPos(x, y);
    INPUT input[2] = {};
    input[0].type = INPUT_MOUSE;
    input[0].mi.dwFlags = MOUSEEVENTF_LEFTDOWN;
    input[1].type = INPUT_MOUSE;
    input[1].mi.dwFlags = MOUSEEVENTF_LEFTUP;
    SendInput(2, input, sizeof(INPUT));
}

static void InventoryGridToScreen(uint32_t gx, uint32_t gy, int& sx, int& sy)
{
    constexpr double SLOT = 49;
    constexpr double ORIGIN_X = 1422.0;
    constexpr double ORIGIN_Y = 438.0;
    sx = static_cast<int>(ORIGIN_X + gx * SLOT);
    sy = static_cast<int>(ORIGIN_Y + gy * SLOT);
}

static void StashGridToScreen(uint32_t gx, uint32_t gy, int& sx, int& sy)
{
    constexpr double SLOT = 49.0;
    constexpr double ORIGIN_X = 70.0;
    constexpr double ORIGIN_Y = 146.0;
    sx = static_cast<int>(ORIGIN_X + gx * SLOT);
    sy = static_cast<int>(ORIGIN_Y + gy * SLOT);
}

void CleanupBeforeUnload()
{
    if (g_Stocker.IsRunning())
        g_Stocker.Stop();

    if (g_GameWindow && g_OriginalWndProc)
    {
        KillTimer(g_GameWindow, 9999);
        SetWindowLongPtrA(g_GameWindow, GWLP_WNDPROC, (LONG_PTR)g_OriginalWndProc);
        g_OriginalWndProc = NULL;
    }

    BankPanelHook::Shutdown();
    InventoryPanelHook::Shutdown();
    MH_Uninitialize();
}

extern "C" __declspec(dllexport) DWORD WINAPI UnloadDll(LPVOID)
{
    CleanupBeforeUnload();
    FreeLibraryAndExitThread(GetModuleHandleA("ESR-utils.dll"), 0);
    return 0;
}

LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KEYDOWN && !(lParam & 0x40000000))
    {
        auto hk = Config::GetHotkeys();
        auto matches = [&](const Config::Hotkey& h) -> bool
            {
                if (!h.IsSet()) return false;
                if (wParam != h.vk) return false;
                bool ctrl = (GetAsyncKeyState(VK_CONTROL) & 0x8000) != 0;
                bool shift = (GetAsyncKeyState(VK_SHIFT) & 0x8000) != 0;
                bool alt = (GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                return ctrl == h.ctrl && shift == h.shift && alt == h.alt;
            };

        if (matches(hk.storeItems))
        {
            if (!g_Stocker.IsRunning())
            {
                auto flags = Config::GetFlags();
                RunRequest req;
                req.crystals = flags.storeCrystals;
                req.gems = flags.storeGems;
                req.runes = flags.storeRunes;
                req.decals = flags.storeDecals;
                req.multistocker = flags.storeMultistocker;
                req.storeNonBlankCoupons = flags.storeNonBlankCoupons;
                req.rerollMagicRings = flags.storeRerollMagicRings;
                req.rerollMagicAmulets = flags.storeRerollMagicAmulets;
                req.rerollMagicJewels = flags.storeRerollMagicJewels;
                req.rerollMagicCharms = flags.storeRerollMagicCharms;
                req.rerollMagicQuivers = flags.storeRerollMagicQuivers;
                req.rerollRareRings = flags.storeRerollRareRings;
                req.rerollRareAmulets = flags.storeRerollRareAmulets;
                req.rerollRareJewels = flags.storeRerollRareJewels;
                req.rerollRareCharms = flags.storeRerollRareCharms;
                req.rerollRareQuivers = flags.storeRerollRareQuivers;
                req.rerollSetUniqueRings = flags.storeRerollSetUniqueRings;
                req.rerollSetUniqueAmulets = flags.storeRerollSetUniqueAmulets;
                req.rerollSetUniqueQuivers = flags.storeRerollSetUniqueQuivers;
                req.rerollSkipInventory = flags.rerollSkipInventory;
                req.rerollSkipStash = flags.rerollSkipStash;
                req.reroll = req.rerollMagicRings || req.rerollMagicAmulets || req.rerollMagicJewels
                    || req.rerollMagicCharms || req.rerollMagicQuivers
                    || req.rerollRareRings || req.rerollRareAmulets || req.rerollRareJewels
                    || req.rerollRareCharms || req.rerollRareQuivers
                    || req.rerollSetUniqueRings || req.rerollSetUniqueAmulets || req.rerollSetUniqueQuivers;
                g_Stocker.Start(req);
            }
        }
        if (matches(hk.emergencyStop))
        {
            if (g_Stocker.IsRunning())
                g_Stocker.Stop();
        }
    }

    if (msg == WM_TIMER && wParam == 9999)
    {
        if (g_Stocker.IsRunning())
            g_Stocker.Tick();
        return 0;
    }

    return CallWindowProcA(g_OriginalWndProc, hWnd, msg, wParam, lParam);
}

void MainThread(HMODULE hModule)
{
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    Sleep(3000);

    uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL));
    if (!base)
    {
        return;
    }

    if (!D2RFunctions::Init())
    {
        return;
    }

    Config::Init();
    BankPanelHook::Init();
    InventoryPanelHook::Init();

    g_GameWindow = FindWindowA("OsWindow", NULL);
    if (!g_GameWindow)
    {
        return;
    }

    g_OriginalWndProc = (WNDPROC)SetWindowLongPtrA(g_GameWindow, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
    if (!g_OriginalWndProc)
    {
        return;
    }

    SetTimer(g_GameWindow, 9999, 5, NULL);

    while (true)
    {
        Sleep(1000);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        std::thread(MainThread, hModule).detach();
    }
    return TRUE;
}