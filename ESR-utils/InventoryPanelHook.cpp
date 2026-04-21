#include "InventoryPanelHook.h"
#include "Pattern.h"
#include <Windows.h>
#include "MinHook.h"

static uint64_t g_LastInvDrawTick = 0;

using InvPanelDraw_t = void(__fastcall*)(void* pThis);
static InvPanelDraw_t oInvPanelDraw = nullptr;

static void __fastcall HookedInvPanelDraw(void* pThis)
{
    g_LastInvDrawTick = GetTickCount64();
    oInvPanelDraw(pThis);
}

bool InventoryPanelHook::Init()
{
    void* target = reinterpret_cast<void*>(Pattern::Address(0x19ABB0));
    if (MH_CreateHook(target, &HookedInvPanelDraw, reinterpret_cast<void**>(&oInvPanelDraw)) != MH_OK)
        return false;
    if (MH_EnableHook(target) != MH_OK)
        return false;
    return true;
}

bool InventoryPanelHook::IsInventoryOpen()
{
    return (GetTickCount64() - g_LastInvDrawTick) < 200;
}