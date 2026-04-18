#include "Pattern.h"
#include <cstdint>
#include <Windows.h>

namespace Pattern
{
    static uintptr_t g_Base = 0;
    static size_t g_Size = 0;

    uintptr_t Base()
    {
        if (!g_Base)
            g_Base = reinterpret_cast<uintptr_t>(GetModuleHandleA(NULL));
        return g_Base;
    }

    size_t ModuleSize()
    {
        if (!g_Size)
        {
            auto* dos = reinterpret_cast<IMAGE_DOS_HEADER*>(Base());
            auto* nt = reinterpret_cast<IMAGE_NT_HEADERS*>(Base() + dos->e_lfanew);
            g_Size = nt->OptionalHeader.SizeOfImage;
        }
        return g_Size;
    }

    uintptr_t Address(uint32_t rva)
    {
        return Base() + rva;
    }

    static int HexCharToVal(char c)
    {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    }

    uintptr_t FindPattern(uintptr_t start, size_t size, const char* pattern)
    {
        const char* p = pattern;
        struct PatternByte { uint8_t value; bool wildcard; };
        PatternByte bytes[256];
        int count = 0;

        while (*p && count < 256)
        {
            while (*p == ' ') p++;
            if (!*p) break;

            if (*p == '?')
            {
                bytes[count++] = { 0, true };
                p++;
                if (*p == '?') p++;
            }
            else
            {
                int hi = HexCharToVal(*p++);
                int lo = HexCharToVal(*p++);
                if (hi < 0 || lo < 0) break;
                bytes[count++] = { static_cast<uint8_t>((hi << 4) | lo), false };
            }
        }

        const uint8_t* data = reinterpret_cast<const uint8_t*>(start);
        for (size_t i = 0; i <= size - count; i++)
        {
            bool found = true;
            for (int j = 0; j < count; j++)
            {
                if (!bytes[j].wildcard && data[i + j] != bytes[j].value)
                {
                    found = false;
                    break;
                }
            }
            if (found)
                return start + i;
        }

        return 0;
    }
}