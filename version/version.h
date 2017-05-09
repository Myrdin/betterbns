#pragma once

BOOL LoadLibraryAndGetProcAddress(LPCSTR lpProcName, FARPROC *lpFarProc);

#define WRAPPER_PROC(x) \
    FARPROC lp##x = NULL; \
    __declspec(naked) VOID WINAPI _##x() { \
        LoadLibraryAndGetProcAddress(#x, &lp##x); \
        __asm jmp lp##x \
    }

BOOL Cleanup(void);
