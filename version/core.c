#include <Windows.h>
#include <Shlwapi.h>
#include <tchar.h>
#include <TlHelp32.h>
#include "core.h"
#include "util.h"


DWORD WINAPI NewThreadProc(LPVOID lpParam) {
    DWORD dwProcessId = GetCurrentProcessId();
    DWORD dwThreadId = GetCurrentThreadId();
    HANDLE lphThreads[0x1000];
    SIZE_T cb;

    SuspendProcessThreads(dwProcessId, dwThreadId, lphThreads, _countof(lphThreads), &cb);

    HMODULE hm = GetModuleHandle(NULL);
    install_hooks(hm);

    ResumeAndCloseThreads(lphThreads, cb);
    return 0;
}

VOID install_hooks(HMODULE hm) {
    DETOUR_IAT(hm, LoadLibraryW);
    DETOUR_IAT(hm, CreateFileW);
    DETOUR_IAT(hm, CreateProcessW);
    return;
}

HMODULE WINAPI _LoadLibraryW(
    _In_ LPCWSTR lpFileName
) {
    HMODULE hm = LoadLibraryW(lpFileName);
    dwprintf(L"Loaded \"%s\" [%p]", lpFileName, hm);

    install_hooks(hm);
    return hm;
}

HANDLE WINAPI _CreateFileW(
    _In_     LPCWSTR               lpFileName,
    _In_     DWORD                 dwDesiredAccess,
    _In_     DWORD                 dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_     DWORD                 dwCreationDisposition,
    _In_     DWORD                 dwFlagsAndAttributes,
    _In_opt_ HANDLE                hTemplateFile
) {
    WCHAR drive[_MAX_DRIVE + 1], dir[_MAX_DIR + 1], filename[_MAX_FNAME + 1], ext[_MAX_EXT + 1];
    _wsplitpath_s(lpFileName, drive, _countof(drive), dir, _countof(dir), filename, _countof(filename), ext, _countof(ext));

    wcscat_s(dir, _countof(dir), L"_original\\");

    WCHAR buffer[MAX_PATH];
    _wmakepath_s(buffer, _countof(buffer), drive, dir, filename, ext);

    if (PathFileExistsW(buffer) || (!wcscat_s(buffer, _countof(buffer), L".ignore") && PathFileExistsW(buffer))) {
        dwprintf(L"Redirecting \"%s\" => \"%s\"", lpFileName, buffer);
        lpFileName = buffer;
    }

    return CreateFileW(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes,
        dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile
    );
}

BOOL WINAPI _CreateProcessW(
    _In_opt_    LPCWSTR               lpApplicationName,
    _Inout_opt_ LPWSTR                lpCommandLine,
    _In_opt_    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    _In_opt_    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    _In_        BOOL                  bInheritHandles,
    _In_        DWORD                 dwCreationFlags,
    _In_opt_    LPVOID                lpEnvironment,
    _In_opt_    LPCWSTR               lpCurrentDirectory,
    _In_        LPSTARTUPINFO         lpStartupInfo,
    _Out_       LPPROCESS_INFORMATION lpProcessInformation
) {
    WCHAR init_path[MAX_PATH];
    GetCurrentDirectoryW(_countof(init_path), init_path);
    wcscat_s(init_path, _countof(init_path), L"\\betterbns.ini");

    BOOL bExitParentProcess = FALSE;
    LPWSTR buffer;
    WCHAR lpAppName[MAX_PATH];
    if (PathFileExistsW(init_path) && config_find_appname(lpApplicationName, lpCommandLine, init_path, lpAppName, _countof(lpAppName))) {
        bExitParentProcess = (BOOL)GetPrivateProfileIntW(lpAppName, L"ExitParentProcess", 0, init_path);
        SIZE_T size = 32768;
        SIZE_T maxlen = 32768;
        if (lpCommandLine) {
            if (!lpApplicationName) {
                maxlen += MAX_PATH;
            }
            size = maxlen;
            maxlen -= wcsnlen_s(lpCommandLine, size - 1) + 1;
        }

        LPWSTR lpExtraArgs = malloc(maxlen * sizeof(WCHAR));
        GetPrivateProfileStringW(lpAppName, L"ExtraArgs", NULL, lpExtraArgs, (DWORD)(maxlen - 1), init_path);

        if (wcscmp(lpExtraArgs, L"")) {
            buffer = malloc(size * sizeof(WCHAR));
            if (!lpCommandLine) {
                wcscpy_s(buffer, size, lpExtraArgs);
            } else {
                swprintf_s(buffer, size, L"%s %s", lpCommandLine, lpExtraArgs);
            }

            lpCommandLine = buffer;
        }
        free(lpExtraArgs);
    }
    dwprintf(L"Starting \"%s\" %s", lpApplicationName, lpCommandLine);
    BOOL result = CreateProcessW(
        lpApplicationName, lpCommandLine, lpProcessAttributes,
        lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment,
        lpCurrentDirectory, lpStartupInfo, lpProcessInformation
    );
    free(buffer);
    if (result && bExitParentProcess) {
        dwprintf(L"Exiting parent process");
        exit(0);
    }
    return result;
}
