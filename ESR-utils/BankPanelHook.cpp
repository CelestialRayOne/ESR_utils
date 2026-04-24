#include "BankPanelHook.h"
#include "Pattern.h"
#include <Windows.h>
#include "MinHook.h"

static uint64_t g_LastBankDrawTick = 0;

using BankPanelDraw_t = void(__fastcall*)(void* pBankPanel);
static BankPanelDraw_t oBankPanelDraw = nullptr;

static void __fastcall HookedBankPanelDraw(void* pThis)
{
    g_LastBankDrawTick = GetTickCount64();
    __try
    {
        oBankPanelDraw(pThis);
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
    }
}

void BankPanelHook::Shutdown()
{
    MH_DisableHook(reinterpret_cast<void*>(Pattern::Address(0x18EB90)));
}

bool BankPanelHook::Init()
{
    if (MH_Initialize() != MH_OK && MH_Initialize() != MH_ERROR_ALREADY_INITIALIZED)
        return false;

    void* target = reinterpret_cast<void*>(Pattern::Address(0x18EB90));
    if (MH_CreateHook(target, &HookedBankPanelDraw, reinterpret_cast<void**>(&oBankPanelDraw)) != MH_OK)
        return false;
    if (MH_EnableHook(target) != MH_OK)
        return false;
    return true;
}

bool BankPanelHook::IsStashOpen()
{
    return (GetTickCount64() - g_LastBankDrawTick) < 200;
}