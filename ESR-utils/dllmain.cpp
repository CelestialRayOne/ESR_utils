#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include "Pattern.h"
#include "D2RFunctions.h"
#include "AutoStocker.h"
#include "BankPanelHook.h"

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

LRESULT CALLBACK HookedWndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_KEYDOWN && wParam == 'K')
    {
        if ((GetAsyncKeyState(VK_CONTROL) & 0x8000) && !(lParam & 0x40000000))
        {
            if (!g_Stocker.IsRunning())
                g_Stocker.Start();
            else
                g_Stocker.Stop();
            return 0;
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
    AllocConsole();
    FILE* f;
    freopen_s(&f, "CONOUT$", "w", stdout);

    printf("[ESR-utils] Waiting for D2R...\n");
    Sleep(3000);

    uintptr_t base = reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL));
    if (!base)
    {
        printf("[ESR-utils] Failed to find D2R base\n");
        return;
    }
    printf("[ESR-utils] D2R base: %llX\n", base);

    if (!D2RFunctions::Init())
    {
        printf("[ESR-utils] Failed to init D2R functions\n");
        return;
    }
    printf("[ESR-utils] Functions resolved\n");

    g_GameWindow = FindWindowA("OsWindow", NULL);
    if (!g_GameWindow)
    {
        printf("[ESR-utils] Game window not found\n");
        return;
    }

    g_OriginalWndProc = (WNDPROC)SetWindowLongPtrA(g_GameWindow, GWLP_WNDPROC, (LONG_PTR)HookedWndProc);
    if (!g_OriginalWndProc)
    {
        printf("[ESR-utils] Failed to hook WndProc\n");
        return;
    }

    SetTimer(g_GameWindow, 9999, 10, NULL);

    printf("[ESR-utils] Ready. Press Ctrl+K to test.\n");

    while (true)
    {
        Sleep(1000);
    }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    if (!BankPanelHook::Init())
        printf("[ESR-utils] Warning: bank panel hook failed, stash detection disabled\n");
    else
        printf("[ESR-utils] Bank panel hook installed\n");

    if (ul_reason_for_call == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);
        std::thread(MainThread, hModule).detach();
    }
    return TRUE;
}