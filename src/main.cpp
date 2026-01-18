#include <Windows.h>
#include <Psapi.h>

#include "Terminal.h"
#include "SDKGenerator.h"

auto WINAPI
Worker(void*) -> DWORD {
    terminal::tryHookConsoleIO();
    {
        SDKGenerator generator;
        generator.run();
    }

    auto handle = GetModuleHandle("DialUp-SDKGen.dll");

    MODULEINFO modInfo{};
    if (GetModuleInformation(GetCurrentProcess(), handle, &modInfo, sizeof(modInfo))) {
        FlushInstructionCache(GetCurrentProcess(), modInfo.lpBaseOfDll, modInfo.SizeOfImage);
    }

    Sleep(200);
    FreeLibraryAndExitThread(handle, 0);
}

BOOL APIENTRY
DllMain(const HMODULE hModule, const DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, Worker, hModule, 0, nullptr);
    }
    return TRUE;
}