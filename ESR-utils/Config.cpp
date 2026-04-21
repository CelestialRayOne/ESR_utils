#include "Config.h"
#include <Windows.h>
#include <string>
#include <fstream>
#include <sstream>
#include <thread>
#include <chrono>

namespace Config
{
    static Hotkeys g_Hotkeys;
    static Flags g_Flags;
    static std::mutex g_Mutex;
    static std::string g_ConfigPath;
    static std::string g_WatchDir;
    static FILETIME g_LastWriteTime = {};

    static std::string GetExeDir()
    {
        char path[MAX_PATH] = {};
        HMODULE hModule = NULL;
        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&GetExeDir, &hModule);
        GetModuleFileNameA(hModule, path, MAX_PATH);
        std::string p(path);
        size_t slash = p.find_last_of("\\/");
        return (slash != std::string::npos) ? p.substr(0, slash) : p;
    }

    static uint32_t ParseVK(const std::string& name)
    {
        if (name.length() == 1)
        {
            char c = name[0];
            if (c >= 'a' && c <= 'z') c -= 32;
            if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z'))
                return (uint32_t)c;
        }
        if (name == "Space") return VK_SPACE;
        if (name == "Enter" || name == "Return") return VK_RETURN;
        if (name == "Tab") return VK_TAB;
        if (name == "Escape") return VK_ESCAPE;
        if (name == "Backspace") return VK_BACK;
        if (name == "Delete") return VK_DELETE;
        if (name == "Insert") return VK_INSERT;
        if (name == "Home") return VK_HOME;
        if (name == "End") return VK_END;
        if (name == "PageUp") return VK_PRIOR;
        if (name == "PageDown") return VK_NEXT;
        if (name == "Left") return VK_LEFT;
        if (name == "Right") return VK_RIGHT;
        if (name == "Up") return VK_UP;
        if (name == "Down") return VK_DOWN;
        for (int i = 1; i <= 24; i++)
        {
            char buf[8];
            sprintf_s(buf, "F%d", i);
            if (name == buf) return VK_F1 + (i - 1);
        }
        if (name.length() == 3 && name[0] == 'D' && name[1] >= '0' && name[1] <= '9')
            return (uint32_t)name[1];
        return 0;
    }

    static Hotkey ParseHotkey(const std::string& s)
    {
        Hotkey hk;
        if (s.empty()) return hk;
        std::string rest = s;
        while (true)
        {
            size_t plus = rest.find('+');
            if (plus == std::string::npos) break;
            std::string part = rest.substr(0, plus);
            rest = rest.substr(plus + 1);
            if (part == "Ctrl") hk.ctrl = true;
            else if (part == "Alt") hk.alt = true;
            else if (part == "Shift") hk.shift = true;
        }
        hk.vk = ParseVK(rest);
        if (hk.vk == 0) { Hotkey empty; return empty; }
        return hk;
    }

    static std::string ExtractField(const std::string& json, const std::string& field)
    {
        std::string key = "\"" + field + "\"";
        size_t kp = json.find(key);
        if (kp == std::string::npos) return "";
        size_t colon = json.find(':', kp);
        if (colon == std::string::npos) return "";
        size_t q1 = json.find('"', colon);
        if (q1 == std::string::npos) return "";
        size_t q2 = json.find('"', q1 + 1);
        if (q2 == std::string::npos) return "";
        return json.substr(q1 + 1, q2 - q1 - 1);
    }

    static bool ExtractBool(const std::string& json, const std::string& field, bool defaultVal)
    {
        std::string key = "\"" + field + "\"";
        size_t kp = json.find(key);
        if (kp == std::string::npos) return defaultVal;
        size_t colon = json.find(':', kp);
        if (colon == std::string::npos) return defaultVal;
        size_t pos = colon + 1;
        while (pos < json.size() && (json[pos] == ' ' || json[pos] == '\t' || json[pos] == '\n' || json[pos] == '\r'))
            pos++;
        if (pos + 4 <= json.size() && json.compare(pos, 4, "true") == 0) return true;
        if (pos + 5 <= json.size() && json.compare(pos, 5, "false") == 0) return false;
        return defaultVal;
    }

    static void LoadFile()
    {
        std::ifstream f(g_ConfigPath);
        if (!f.is_open()) return;
        std::stringstream ss;
        ss << f.rdbuf();
        std::string content = ss.str();

        Hotkeys fresh;
        fresh.storeItems = ParseHotkey(ExtractField(content, "Hotkey"));
        fresh.emergencyStop = ParseHotkey(ExtractField(content, "EmergencyStop"));

        std::lock_guard<std::mutex> lock(g_Mutex);
        g_Hotkeys = fresh;

        Flags freshFlags;
        freshFlags.storeCrystals = ExtractBool(content, "StoreCrystals", true);
        freshFlags.storeGems = ExtractBool(content, "StoreGems", true);
        freshFlags.storeRunes = ExtractBool(content, "StoreRunes", true);
        freshFlags.storeDecals = ExtractBool(content, "StoreDecals", true);
        freshFlags.storeMultistocker = ExtractBool(content, "StoreMultistocker", true);
        freshFlags.storeNonBlankCoupons = ExtractBool(content, "StoreNonBlankCoupons", false);
        freshFlags.storeRerollMagic = ExtractBool(content, "StoreRerollMagic", false);
        freshFlags.storeRerollRare = ExtractBool(content, "StoreRerollRare", false);

        g_Flags = freshFlags;
    }

    static void WatcherThread()
    {
        HANDLE hNotify = FindFirstChangeNotificationA(g_WatchDir.c_str(), FALSE,
            FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME);
        if (hNotify == INVALID_HANDLE_VALUE) return;

        while (true)
        {
            DWORD r = WaitForSingleObject(hNotify, INFINITE);
            if (r != WAIT_OBJECT_0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            LoadFile();
            if (!FindNextChangeNotification(hNotify)) break;
        }
        FindCloseChangeNotification(hNotify);
    }

    bool Init()
    {
        g_WatchDir = GetExeDir();
        g_ConfigPath = g_WatchDir + "\\ESR-utils.json";
        LoadFile();
        std::thread(WatcherThread).detach();
        return true;
    }

    Hotkeys GetHotkeys()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return g_Hotkeys;
    }

    Flags GetFlags()
    {
        std::lock_guard<std::mutex> lock(g_Mutex);
        return g_Flags;
    }
}