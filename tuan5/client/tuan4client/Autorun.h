#pragma once
#include <windows.h>

static const wchar_t* AUTORUN_VALUE_NAME = L"ChatClientApp";

inline bool Autorun_Enable()
{
    wchar_t exePath[MAX_PATH];
    if (GetModuleFileNameW(NULL, exePath, MAX_PATH) == 0)
        return false;

    HKEY hKey;
    LONG r = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE, &hKey);
    if (r != ERROR_SUCCESS) return false;

    CStringW quoted = L"\"" + CStringW(exePath) + L"\"";

    r = RegSetValueExW(hKey, AUTORUN_VALUE_NAME, 0, REG_SZ,
        (const BYTE*)(LPCWSTR)quoted,
        (quoted.GetLength() + 1) * sizeof(wchar_t));

    RegCloseKey(hKey);
    return r == ERROR_SUCCESS;
}

inline bool Autorun_Disable()
{
    HKEY hKey;
    LONG r = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_SET_VALUE, &hKey);
    if (r != ERROR_SUCCESS) return false;

    r = RegDeleteValueW(hKey, AUTORUN_VALUE_NAME);
    RegCloseKey(hKey);
    return r == ERROR_SUCCESS;
}

inline bool Autorun_IsEnabled()
{
    HKEY hKey;
    LONG r = RegOpenKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, KEY_QUERY_VALUE, &hKey);
    if (r != ERROR_SUCCESS) return false;

    wchar_t buf[MAX_PATH * 2] = {};
    DWORD size = sizeof(buf);
    DWORD type = 0;
    r = RegQueryValueExW(hKey, AUTORUN_VALUE_NAME, nullptr, &type, (BYTE*)buf, &size);
    RegCloseKey(hKey);
    return (r == ERROR_SUCCESS && type == REG_SZ);
}
