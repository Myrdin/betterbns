#include <Windows.h>
#include <Shlwapi.h>
#include <tchar.h>
#include <TlHelp32.h>
#include "core.h"
#include "util.h"


DWORD WINAPI install_hooks_t(LPVOID lpParam) {
    DWORD dwProcessId = GetCurrentProcessId();
    DWORD dwThreadId = GetCurrentThreadId();

    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

    THREADENTRY32 te;
    te.dwSize = sizeof(te);

    HANDLE hThreads[2048];
    int count = 0;

    Thread32First(hSnap, &te);
    do {
        if (te.th32OwnerProcessID != dwProcessId || te.th32ThreadID == dwThreadId) {
            continue;
        }
        hThreads[count] = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
        SuspendThread(hThreads[count]);
        XTRACE(L"Suspended thread %d [%p]", te.th32ThreadID, hThreads[count]);
        count++;
    } while (Thread32Next(hSnap, &te));
    CloseHandle(hSnap);

    install_hooks(NULL);

    for (int i = 0; i < count; i++) {
        ResumeThread(hThreads[i]);
        XTRACE(L"Resumed thread [%p]", hThreads[i]);
        CloseHandle(hThreads[i]);
    }
    return 0;
}

VOID install_hooks(HMODULE hm) {
    if (hm == NULL) {
        hm = GetModuleHandle(NULL);
    }
    DETOUR_IAT(LoadLibraryW, hm);
    DETOUR_IAT(CreateFileW, hm);
    DETOUR_IAT(CreateProcessW, hm);
};

HMODULE WINAPI _LoadLibraryW(
    _In_ LPCWSTR lpFileName
) {
    HMODULE hm = LoadLibraryW(lpFileName);
    XTRACE(L"Loaded \"%s\" [%p]", lpFileName, hm);

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

    WCHAR buffer[MAX_PATH + 1];
    _wmakepath_s(buffer, _countof(buffer), drive, dir, filename, ext);

    if (PathFileExistsW(buffer) || (!wcscat_s(buffer, _countof(buffer), L".ignore") && PathFileExistsW(buffer))) {
        XTRACE(L"Redirecting \"%s\" => \"%s\"", lpFileName, buffer);
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
    WCHAR init_path[MAX_PATH + 1];
    GetCurrentDirectoryW(_countof(init_path), init_path);
    wcscat_s(init_path, _countof(init_path), L"\\init.ini");

    BOOL bExitParentProcess = FALSE;
    LPWSTR buffer;
    WCHAR lpAppName[MAX_PATH + 1];
    if (PathFileExistsW(init_path) && init_find_appname(lpApplicationName, lpCommandLine, init_path, lpAppName, _countof(lpAppName))) {
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
    XTRACE(L"Starting \"%s\" %s", lpApplicationName, lpCommandLine);
    BOOL result = CreateProcessW(
        lpApplicationName, lpCommandLine, lpProcessAttributes,
        lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment,
        lpCurrentDirectory, lpStartupInfo, lpProcessInformation
    );
    free(buffer);
    if (result && bExitParentProcess) {
        XTRACE(L"Exiting parent process");
        exit(0);
    }
    return result;
}
