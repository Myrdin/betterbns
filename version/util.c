#include <Windows.h>
#include <Shlwapi.h>
#include <tchar.h>
#include "util.h"


LPVOID *iat_find(LPSTR func, HMODULE hm) {
    PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)hm;
    PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((LPBYTE)dos + dos->e_lfanew);
    PIMAGE_IMPORT_DESCRIPTOR desc = (PIMAGE_IMPORT_DESCRIPTOR)((LPBYTE)dos + nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress);

    for (PIMAGE_IMPORT_DESCRIPTOR iid = desc; iid->Name != 0; iid++) {
        for (int i = 0; *(i + (LPVOID*)(iid->FirstThunk + (SIZE_T)hm)) != NULL; i++) {
            LPSTR name = (LPSTR)(*(i + (SIZE_T*)(iid->OriginalFirstThunk + (SIZE_T)hm)) + (SIZE_T)hm + 2);
            const uintptr_t n = (uintptr_t)name;
            if (!(n & (sizeof(n) == 4 ? 0x80000000 : 0x8000000000000000)) && !_stricmp(func, name)) {
                return i + (LPVOID*)(iid->FirstThunk + (SIZE_T)hm);
            }
        }
    }
    return NULL;
}

VOID detour_iat_ptr(LPSTR func, LPVOID hook, HMODULE hm) {
    LPVOID *lpAddress = iat_find(func, hm);

    if (lpAddress == NULL || *lpAddress == hook) {
        return;
    }

    DWORD flOldProtect;
    DWORD flNewProtect = PAGE_READWRITE;
    VirtualProtect(lpAddress, sizeof(LPVOID), flNewProtect, &flOldProtect);

    XTRACE(L"\"%S\" hooked [%p => %p]", func, *lpAddress, hook);
    *lpAddress = hook;

    VirtualProtect(lpAddress, sizeof(LPVOID), flOldProtect, &flNewProtect);
}

BOOL init_find_appname(LPCWSTR lpApplicationName, LPCWSTR lpCommandLine, LPCWSTR lpInitFilePath, LPWSTR lpAppName, SIZE_T count) {
    WCHAR lpFullPath[MAX_PATH + 1];
    LPWSTR lpFileName;
    if (lpApplicationName) {
        wcscpy_s(lpFullPath, _countof(lpFullPath), lpApplicationName);
        lpFileName = PathFindFileNameW(lpApplicationName);
    } else {
        int nArgs;
        LPWSTR *args = CommandLineToArgvW(lpCommandLine, &nArgs);
        if (nArgs > 1) {
            wcscpy_s(lpFullPath, _countof(lpFullPath), args[0]);
            lpFileName = PathFindFileNameW(args[0]);
        }
    }
    if (!lpFullPath && !lpFileName) {
        return FALSE;
    }

    WCHAR lpszReturnedBuffer[4096];
    GetPrivateProfileSectionNamesW(lpszReturnedBuffer, _countof(lpszReturnedBuffer), lpInitFilePath);

    for (LPWSTR p = lpszReturnedBuffer; *p; p = wcschr(p, L'\0') + 1) {
        if (!_wcsicmp(p, lpFullPath)) {
            wcscpy_s(lpAppName, count, lpFullPath);
            return TRUE;
        }
    }

    for (LPWSTR p = lpszReturnedBuffer; *p; p = wcschr(p, L'\0') + 1) {
        if (!_wcsicmp(p, lpFileName)) {
            wcscpy_s(lpAppName, count, lpFileName);
            return TRUE;
        }
    }
    return FALSE;
}

#ifdef _DEBUG
VOID __XTRACE(LPCWSTR format, ...) {
    WCHAR buffer[4096];
    va_list argptr;
    va_start(argptr, format);
    vswprintf_s(buffer, _countof(buffer), format, argptr);
    va_end(argptr);

    OutputDebugStringW(buffer);
}
#endif
