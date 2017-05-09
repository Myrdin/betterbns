#include <Windows.h>
#include <tchar.h>
#include <Shlwapi.h>
#include <TlHelp32.h>
#include "version.h"
#include "core.h"
#include "util.h"


BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);
        HANDLE hThread = CreateThread(NULL, 0, install_hooks_t, NULL, 0, NULL);
        CloseHandle(hThread);
        break;
    }
    case DLL_PROCESS_DETACH:
        if (lpReserved == NULL) {
            Cleanup();
        }
        break;
    default:
        break;
    }
    return TRUE;
}
