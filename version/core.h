#pragma once

DWORD WINAPI NewThreadProc(LPVOID lpParam);

VOID install_hooks(HMODULE hm);

HMODULE WINAPI _LoadLibraryW(
    _In_ LPCWSTR lpFileName
);

HANDLE WINAPI _CreateFileW(
    _In_     LPCWSTR               lpFileName,
    _In_     DWORD                 dwDesiredAccess,
    _In_     DWORD                 dwShareMode,
    _In_opt_ LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    _In_     DWORD                 dwCreationDisposition,
    _In_     DWORD                 dwFlagsAndAttributes,
    _In_opt_ HANDLE                hTemplateFile
);

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
);
