#pragma once
#include <cstdint>

namespace Pattern
{
    uintptr_t FindPattern(uintptr_t start, size_t size, const char* pattern);
    uintptr_t Address(uint32_t rva);
    uintptr_t Base();
    size_t ModuleSize();
}