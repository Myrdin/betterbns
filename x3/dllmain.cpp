#include <Windows.h>
#include <stdio.h>
#include <tchar.h>

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);
        /*TCHAR Buffer[MAX_PATH], FileName[MAX_PATH];
        GetCurrentDirectory(_countof(Buffer), Buffer);
        GetCurrentDirectory(_countof(FileName), FileName);
        _tcscat_s(Buffer, _countof(Buffer), _T("\\aegisty.bin"));
        _tcscat_s(FileName, _countof(FileName), _T("\\aegisty1.bin"));
        DeleteFile(FileName);
        MoveFile(Buffer, FileName);*/
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        /*TCHAR Buffer[MAX_PATH], FileName[MAX_PATH];
        GetCurrentDirectory(_countof(Buffer), Buffer);
        GetCurrentDirectory(_countof(FileName), FileName);
        _tcscat_s(Buffer, _countof(Buffer), _T("\\aegisty.bin"));
        _tcscat_s(FileName, _countof(FileName), _T("\\aegisty1.bin"));
        MoveFile(FileName, Buffer);*/
        break;
    }
    default:
        break;
    }
    return TRUE;
}
