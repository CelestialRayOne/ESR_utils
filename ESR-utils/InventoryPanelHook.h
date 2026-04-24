#pragma once

namespace InventoryPanelHook
{
    bool Init();
    bool IsInventoryOpen();
    void* GetPanel();
    void Shutdown();
}