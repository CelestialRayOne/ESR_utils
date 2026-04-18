#include <Windows.h>
#include <cstdint>
#include <cstdio>
#include <thread>
#include "Pattern.h"
#include "D2RFunctions.h"
#include "AutoStocker.h"

static AutoStocker g_Stocker;

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
    printf("[ESR-utils] F9  = Scan inventory\n");
    printf("[ESR-utils] F10 = Start stocking\n");
    printf("[ESR-utils] F11 = Stop stocking\n");

    while (true)
    {
        if (GetAsyncKeyState(VK_F9) & 1)
        {
            auto crystals = g_Stocker.FindCrystalsInInventory();
            auto stocker = g_Stocker.FindStockerInInventory();
            printf("Crystal Can: %s (unitId=%u)\n",
                stocker.unitId ? "FOUND" : "NOT FOUND", stocker.unitId);
            printf("Crystals in inventory: %d\n", (int)crystals.size());
            for (const auto& c : crystals)
                printf("  classId=%u unitId=%u\n", c.classId, c.unitId);
        }

        if (GetAsyncKeyState(VK_F10) & 1)
        {
            g_Stocker.Start();
        }

        if (GetAsyncKeyState(VK_F11) & 1)
        {
            g_Stocker.Stop();
        }

        if (g_Stocker.IsRunning())
        {
            g_Stocker.Tick();
        }

        Sleep(50);
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