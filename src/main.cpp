#include <Windows.h>
#include <Psapi.h>

#include "Terminal.h"
#include "SDKGenerator.h"

auto WINAPI
Worker(const LPVOID lpParam) -> DWORD {
    Sleep(100);
    const auto handle = static_cast<HMODULE>(lpParam);

    terminal::tryHookConsoleIO();

    {
        SDKGenerator generator;
        generator.run();
        generator.yeet();
    }

    Sleep(100);

    // Check if DLL is still referenced
    HMODULE testHandle = nullptr;
    if (GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCSTR>(handle), &testHandle)) { // NOLINT
        printf("Warning: DLL %p still has active references\n", handle);
    }

    // Get module info for flushing
    MODULEINFO modInfo{};
    if (GetModuleInformation(GetCurrentProcess(), handle, &modInfo, sizeof(modInfo))) {
        FlushInstructionCache(GetCurrentProcess(), modInfo.lpBaseOfDll, modInfo.SizeOfImage);
    }

    terminal::tryFreeConsole();

    FreeLibraryAndExitThread(static_cast<HMODULE>(lpParam), 0);
}

BOOL APIENTRY
DllMain(const HMODULE hModule, const DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(nullptr, 0, Worker, hModule, 0, nullptr);
    }
    return TRUE;
}
