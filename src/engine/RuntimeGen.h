#pragma once
#include <Windows.h>
#include <Psapi.h>
#include <sstream>

#include "Runtime.h"
#include "RuntimeGen.h"
#include "Schema.h"
#include "WaveWorker.h"

#include <functional>

namespace memory {

inline bool isReadable(uintptr_t addr) {
    if (addr == 0) { return false; }

    MEMORY_BASIC_INFORMATION mbi;
    if (!VirtualQuery(reinterpret_cast<void*>(addr), &mbi, sizeof(mbi)))
        return false;

    if (mbi.State != MEM_COMMIT)
        return false;

    const DWORD protect = mbi.Protect & 0xFF;
    return protect == PAGE_READONLY ||
           protect == PAGE_READWRITE ||
           protect == PAGE_EXECUTE_READ ||
           protect == PAGE_EXECUTE_READWRITE;
}

inline auto getBaseAddress() -> uintptr_t {
    return reinterpret_cast<uintptr_t>(GetModuleHandle(nullptr));
}

inline auto getOffset(void* pointer) -> uintptr_t {
    const auto baseAddress = getBaseAddress();

    if (const auto address = reinterpret_cast<uintptr_t>(pointer); address > baseAddress) {
        return (address - baseAddress);
    }
    return NULL;
}

inline auto parseHexPattern(const std::string& hexStr) -> std::vector<uint8_t> {
    std::vector<uint8_t> bytes;
    std::istringstream stream(hexStr);
    std::string byteStr;

    while (stream >> byteStr) {
        auto byte = static_cast<uint8_t>(std::stoul(byteStr, nullptr, 16));
        bytes.push_back(byte);
    }

    return bytes;
}


inline auto
findPattern(const std::string& patternStr, const std::string& mask) -> uintptr_t {
    const auto pattern = parseHexPattern(patternStr);
    const size_t patternLength = pattern.size();

    if (patternLength > 0 && mask.length() == patternLength) {
        MODULEINFO miInfos;
        ZeroMemory(&miInfos, sizeof(MODULEINFO));

        HMODULE hModule = GetModuleHandle(nullptr);
        K32GetModuleInformation(GetCurrentProcess(), hModule, &miInfos, sizeof(MODULEINFO));

        const auto start = reinterpret_cast<uintptr_t>(hModule);
        const auto end = start + miInfos.SizeOfImage;

        for (uintptr_t retAddress = start; retAddress < end - patternLength; ++retAddress) {
            bool found = true;

            for (size_t i = 0; i < patternLength; ++i) {
                if (mask[i] != '?' && pattern[i] != *reinterpret_cast<uint8_t*>(retAddress + i)) {
                    found = false;
                    break;
                }
            }

            if (found) {
                return retAddress;
            }
        }
    }

    return 0;
}

inline uintptr_t findRipRelativeAddr(uintptr_t startAddr, int offsetToDisplacementInt32) {
    if (!startAddr)
        return 0;
    uintptr_t ripRelativeOffsetAddr = startAddr + offsetToDisplacementInt32;
    int32_t   displacement          = *reinterpret_cast<int32_t *>(ripRelativeOffsetAddr);
    return (ripRelativeOffsetAddr + 4) + displacement;
};

}

struct RuntimeGen {
    static auto populate() -> bool;
    static void dumpUObjects();
    static void dumpFNames();
};