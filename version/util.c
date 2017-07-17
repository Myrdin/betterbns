#include <Windows.h>
#include <Shlwapi.h>
#include <TlHelp32.h>
#include <tchar.h>
#include <stdio.h>
#include <Psapi.h>
#include "util.h"

static FILE *log_fp = NULL;

LPVOID *FindIAT(HMODULE hModule, LPSTR lpFunctionName) {
    uintptr_t hm = (uintptr_t)hModule;

    for (PIMAGE_IMPORT_DESCRIPTOR iid = (PIMAGE_IMPORT_DESCRIPTOR)(hm + ((PIMAGE_NT_HEADERS)(hm + ((PIMAGE_DOS_HEADER)hm)->e_lfanew))
        ->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress); iid->Name; iid++) {

        LPVOID *p;
        for (SIZE_T i = 0; *(p = i + (LPVOID *)(hm + iid->FirstThunk)); i++) {
            LPSTR fn = (LPSTR)(hm + *(i + (SIZE_T *)(hm + iid->OriginalFirstThunk)) + 2);
            if (!((uintptr_t)fn & IMAGE_ORDINAL_FLAG) && !_stricmp(lpFunctionName, fn)) {
                return p;
            }
        }
    }
    return NULL;
}

VOID DetourIAT(HMODULE hModule, LPSTR lpFuncName, LPVOID *lpOldAddress, LPVOID lpNewAddress) {
    LPVOID *lpAddress = FindIAT(hModule, lpFuncName);
    if (!lpAddress || *lpAddress == lpNewAddress) {
        return;
    }

    DWORD flOldProtect;
    DWORD flNewProtect = PAGE_READWRITE;
    VirtualProtect(lpAddress, sizeof(LPVOID), flNewProtect, &flOldProtect);
    if (lpOldAddress) {
        *lpOldAddress = *lpAddress;
    }
    dwprintf(L"Modified %S import address: %p => %p", lpFuncName, *lpAddress, lpNewAddress);
    *lpAddress = lpNewAddress;
    VirtualProtect(lpAddress, sizeof(LPVOID), flOldProtect, &flNewProtect);
}

VOID SuspendProcessThreads(DWORD dwProcessId, DWORD dwThreadId, HANDLE *lphThreads, SIZE_T dwSize, SIZE_T *lpcb) {
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    THREADENTRY32 te;
    ZeroMemory(&te, sizeof(THREADENTRY32));
    te.dwSize = sizeof(te);
    Thread32First(hSnap, &te);

    SIZE_T count = 0;

    do {
        if (te.th32OwnerProcessID != dwProcessId || te.th32ThreadID == dwThreadId) {
            continue;
        }
        lphThreads[count] = OpenThread(THREAD_SUSPEND_RESUME, FALSE, te.th32ThreadID);
        SuspendThread(lphThreads[count]);
        count++;
    } while (count < dwSize && Thread32Next(hSnap, &te));
    CloseHandle(hSnap);

    *lpcb = count;
    dwprintf(L"Suspended %d other threads", count);
}

VOID ResumeAndCloseThreads(HANDLE *lphThreads, SIZE_T cb) {
    for (SIZE_T i = 0; i < cb; i++) {
        ResumeThread(lphThreads[i]);
        CloseHandle(lphThreads[i]);
    }
    dwprintf(L"Resumed %d other threads", cb);
}

BOOL config_find_appname(LPCWSTR lpApplicationName, LPCWSTR lpCommandLine, LPCWSTR lpInitFilePath, LPWSTR lpAppName, SIZE_T count) {
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

BOOL log_open(void) {
    if (log_fp) {
        return TRUE;
    }
    WCHAR filename[MAX_PATH];
    GetModuleFileNameW(HINST_THISCOMPONENT, filename, _countof(filename));
    WCHAR drive[_MAX_DRIVE], dir[_MAX_DIR], fname[_MAX_FNAME];
    _wsplitpath_s(filename, drive, _countof(drive), dir, _countof(dir), fname, _countof(fname), NULL, 0);

    WCHAR basename[MAX_PATH];
    GetModuleBaseNameW(GetCurrentProcess(), NULL, basename, _countof(basename));
    wcscat_s(fname, _countof(fname), L".");
    wcscat_s(fname, _countof(fname), basename);
    _wmakepath_s(filename, _countof(filename), drive, dir, fname, L".log");

    HANDLE hFile = CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    LARGE_INTEGER size;
    GetFileSizeEx(hFile, &size);
    CloseHandle(hFile);
    log_fp = _wfsopen(filename, size.QuadPart < (1 << 20) ? L"at" : L"wt", _SH_DENYWR);
    if (!log_fp) {
        return FALSE;
    }
    return TRUE;
}

VOID log_close(void) {
    if (log_fp) {
        fclose(log_fp);
    }
}

VOID dwprintf_(LPCWSTR format, ...) {
    if (log_open()) {
        WCHAR datebuf[9], timebuf[9];
        _wstrdate_s(datebuf, _countof(datebuf));
        _wstrtime_s(timebuf, _countof(timebuf));
        fwprintf_s(log_fp, L"%s %s [%d] ", datebuf, timebuf, GetCurrentProcessId());

        va_list argptr;
        va_start(argptr, format);
        vfwprintf_s(log_fp, format, argptr);
        va_end(argptr);
        fflush(log_fp);
    }
}
