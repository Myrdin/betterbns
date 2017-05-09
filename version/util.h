#pragma once

EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#define HINST_THISCOMPONENT ((HINSTANCE)&__ImageBase)

LPVOID *iat_find(LPSTR func, HMODULE hm);

VOID detour_iat_ptr(LPSTR func, LPVOID hook, HMODULE hm);
#define DETOUR_IAT(x, y) detour_iat_ptr(#x, &_##x, y)

BOOL init_find_appname(LPCWSTR lpApplicationName, LPCWSTR lpCommandLine, LPCWSTR lpInitFilePath, LPWSTR lpAppName, SIZE_T count);

#ifdef _DEBUG
VOID __XTRACE(LPCWSTR format, ...);
#define XTRACE(format, ...) __XTRACE(__FUNCTIONW__ L": " format, ##__VA_ARGS__)
#else
#define XTRACE(format, ...)
#endif
