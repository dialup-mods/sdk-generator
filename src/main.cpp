#include <Windows.h>
#include <Psapi.h>

#include "CrashShield.h"
#include "Terminal.h"
#include "SDKGenerator.h"
#include "MessageBox.h"
#include "ShutdownMonitor.h"

std::atomic<bool> g_shouldExit{false};
HANDLE g_shutdownThread{nullptr};
std::string g_shutdownFilename = "shutdown_sdkgen";

__declspec(dllexport) void
destroySDKGen(const LPVOID handle) {
    printf("Shutting down...\n");
    shutdown::touchfile::remove();
    terminal::tryFreeConsole();

    crash::shield::remove();

    HMODULE hModule = reinterpret_cast<HMODULE>(handle);
    MODULEINFO modInfo{};
    if (GetModuleInformation(GetCurrentProcess(), hModule, &modInfo, sizeof(modInfo))) {
        FlushInstructionCache(GetCurrentProcess(), modInfo.lpBaseOfDll, modInfo.SizeOfImage);
    }

    Sleep(200);

    FreeLibraryAndExitThread(hModule, 0);
}

DWORD WINAPI
Worker(const LPVOID hModule) {
    Sleep(100);
    crash::shield::install();

    g_shutdownThread = CreateThread(nullptr, 0, shutdown::watcher, hModule, 0, nullptr);

    terminal::tryHookConsoleIO(false);
    {
        printf("Generator started.\n");
        SDKGenerator generator;
        generator.run();
    }

    // block forever - let ShutdownWatcher handle shutdown
    while (!g_shouldExit.load()) {
        Sleep(100);
    }

    WaitForSingleObject(g_shutdownThread, 200);  // Block until Worker actually exits
    CloseHandle(g_shutdownThread);
    destroySDKGen(hModule);
    return 0; // never reached
}

BOOL APIENTRY
DllMain(const HMODULE hModule, const DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        crash::shield::init(hModule);
        DisableThreadLibraryCalls(hModule);

        shutdown::touchfile::remove();

        CreateThread(nullptr, 0, Worker, hModule, 0, nullptr);
    }
    return TRUE;
}