#include <Windows.h>
#include <tchar.h>
#include "version.h"
#include "util.h"


HMODULE hm = NULL;

WRAPPER_PROC(GetFileVersionInfoA);
WRAPPER_PROC(GetFileVersionInfoByHandle);
WRAPPER_PROC(GetFileVersionInfoExW);
WRAPPER_PROC(GetFileVersionInfoSizeA);
WRAPPER_PROC(GetFileVersionInfoSizeExW);
WRAPPER_PROC(GetFileVersionInfoSizeW);
WRAPPER_PROC(GetFileVersionInfoW);
WRAPPER_PROC(VerFindFileA);
WRAPPER_PROC(VerFindFileW);
WRAPPER_PROC(VerInstallFileA);
WRAPPER_PROC(VerInstallFileW);
WRAPPER_PROC(VerLanguageNameA);
WRAPPER_PROC(VerLanguageNameW);
WRAPPER_PROC(VerQueryValueA);
WRAPPER_PROC(VerQueryValueW);

BOOL LoadLibraryAndGetProcAddress(LPCSTR lpProcName, FARPROC *lpFarProc) {
    if (hm && *lpFarProc) {
        return TRUE;
    }
    if (!hm) {
        TCHAR path[MAX_PATH];
        GetSystemDirectory(path, _countof(path));
        _tcscat_s(path, _countof(path), _T("\\version.dll"));
        hm = LoadLibrary(path);
        dwprintf(L"Loaded original \"version.dll\" [%p]", hm);
    }
    if (hm) {
        *lpFarProc = GetProcAddress(hm, lpProcName);
        dwprintf(L"Resolved function \"%S\" [%p]", lpProcName, *lpFarProc);
        return TRUE;
    }
    return FALSE;
}

BOOL Cleanup(void) {
    BOOL result = FALSE;
    if (!hm) {
        return result;
    }
    result = FreeLibrary(hm);
    if (result) {
        hm = NULL;
    }
    dwprintf(L"Bye bye!");
    log_close();
    return result;
}
