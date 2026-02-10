#include <Windows.h>
#include <Psapi.h>

#include "CrashShield.h"
#include "Terminal.h"
#include "SDKGenerator.h"
#include "MessageBox.h"
#include "TouchFile.h"

__declspec(dllexport) void
destroySDKGen(const LPVOID handle) {
    printf("Shutting down...\n");

    crash::shield::remove();

    HMODULE hModule = reinterpret_cast<HMODULE>(handle);
    MODULEINFO modInfo{};
    if (GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo))) {
        FlushInstructionCache(GetCurrentProcess(), modInfo.lpBaseOfDll, modInfo.SizeOfImage);
    }

    Sleep(200);
    terminal::tryFreeConsole();

    FreeLibraryAndExitThread(hModule, 0);
}

DWORD WINAPI
Worker(const LPVOID hModule) {
    Sleep(100);
    crash::shield::install();

    terminal::tryHookConsoleIO(false);
    {
        printf("Generator started.\n");
        SDKGenerator generator;
        generator.run();
    }

    destroySDKGen(hModule);
    return 0; // never reached
}

BOOL APIENTRY
DllMain(const HMODULE hModule, const DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        crash::shield::init(hModule);
        DisableThreadLibraryCalls(hModule);

        CreateThread(nullptr, 0, Worker, hModule, 0, nullptr);
    }
    return TRUE;
}