#include <Windows.h>
#include <Psapi.h>

#include "CrashShield.h"
#include "Terminal.h"
#include "SDKGenerator.h"
#include "MessageBox.h"
#include "TouchFile.h"

std::atomic g_shouldExit{false};
HANDLE g_shutdownThread{nullptr};
auto touchfile = TouchFile("shutdown");

auto deleteShutdownTouchFile() -> bool {
    if (touchfile.removeFile()) {
        return true;
    }
    return false;
}

auto WINAPI ShutdownWatcher(void*) -> DWORD {
    while (!g_shouldExit.load() && touchfile.path()) {
        Sleep(1000);
        if (deleteShutdownTouchFile()) {
            g_shouldExit.store(true);
            break;
        }
    }
    return 0;
}


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

    g_shutdownThread = CreateThread(nullptr, 0, ShutdownWatcher, hModule, 0, nullptr);

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

        deleteShutdownTouchFile();

        CreateThread(nullptr, 0, Worker, hModule, 0, nullptr);
    }
    return TRUE;
}