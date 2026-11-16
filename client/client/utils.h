#pragma once
#include <string>
#include <vector>
#include <windows.h>
#include <wincrypt.h>
#include <atlimage.h>
#include <functional>

#pragma comment(lib, "Crypt32.lib")

// --- CÁC HÀM BASE64 ---
inline std::string Base64Encode(const std::vector<BYTE>& data) {
    if (data.empty()) return "";
    DWORD outLen = 0;
    CryptBinaryToStringA(data.data(), (DWORD)data.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, NULL, &outLen);
    std::string out(outLen, 0);
    CryptBinaryToStringA(data.data(), (DWORD)data.size(), CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, &out[0], &outLen);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

inline std::vector<BYTE> Base64Decode(const std::string& b64) {
    if (b64.empty()) return {};
    DWORD outLen = 0;
    CryptStringToBinaryA(b64.c_str(), (DWORD)b64.length(), CRYPT_STRING_BASE64, NULL, &outLen, NULL, NULL);
    std::vector<BYTE> out(outLen);
    CryptStringToBinaryA(b64.c_str(), (DWORD)b64.length(), CRYPT_STRING_BASE64, out.data(), &outLen, NULL, NULL);
    return out;
}

// --- HÀM CHUYỂN BYTE SANG ẢNH ---
inline HBITMAP BytesToHBITMAP(const std::vector<BYTE>& bytes, int targetW, int targetH) {
    if (bytes.empty()) return NULL;

    CImage image;
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, bytes.size());
    if (!hMem) return NULL;

    void* pMem = GlobalLock(hMem);
    memcpy(pMem, bytes.data(), bytes.size());
    GlobalUnlock(hMem);

    IStream* pStream = NULL;
    if (FAILED(CreateStreamOnHGlobal(hMem, TRUE, &pStream))) {
        GlobalFree(hMem);
        return NULL;
    }

    HRESULT hr = image.Load(pStream);
    pStream->Release();
    if (FAILED(hr)) return NULL;

    int imgW = image.GetWidth();
    int imgH = image.GetHeight();

    CImage thumb;
    thumb.Create(targetW, targetH, 32);
    HDC hdc = thumb.GetDC();

    HBRUSH hBrush = CreateSolidBrush(RGB(240, 240, 240));
    RECT rect = { 0, 0, targetW, targetH };
    FillRect(hdc, &rect, hBrush);
    DeleteObject(hBrush);

    // Tính tỷ lệ fit
    double ratioX = (double)targetW / imgW;
    double ratioY = (double)targetH / imgH;
    double ratio = min(ratioX, ratioY);

    int newW = (int)(imgW * ratio);
    int newH = (int)(imgH * ratio);
    int x = (targetW - newW) / 2;
    int y = (targetH - newH) / 2;

    int oldMode = SetStretchBltMode(hdc, HALFTONE);
    SetBrushOrgEx(hdc, 0, 0, NULL);
    image.StretchBlt(hdc, x, y, newW, newH, 0, 0, imgW, imgH, SRCCOPY);
    SetStretchBltMode(hdc, oldMode);
    thumb.ReleaseDC();

    return thumb.Detach();
}

inline void SafeSetBitmap(CStatic& staticCtrl, HBITMAP hBmp) {
    if (staticCtrl.GetSafeHwnd() && ::IsWindow(staticCtrl.GetSafeHwnd())) {
        // Thêm SS_REALSIZECONTROL để tự co giãn
        staticCtrl.ModifyStyle(0, SS_BITMAP | SS_REALSIZECONTROL);
        HBITMAP hOld = staticCtrl.SetBitmap(hBmp);
        if (hOld) DeleteObject(hOld);
    }
    else {
        if (hBmp) DeleteObject(hBmp);
    }
}

inline bool IsStartupEnabled() {
    HKEY hKey;
    bool enabled = false;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_QUERY_VALUE, &hKey) == ERROR_SUCCESS) {
        if (RegQueryValueExW(hKey, L"chatclient", nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
            enabled = true;
        }
        RegCloseKey(hKey);
    }
    return enabled;
}

inline void SetStartup(bool enable) {
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey) == ERROR_SUCCESS) {
        if (enable) {
            wchar_t path[MAX_PATH];
            GetModuleFileNameW(nullptr, path, MAX_PATH);
            // Thêm ngoặc kép để tránh lỗi nếu đường dẫn có dấu cách
            std::wstring quotedPath = L"\"" + std::wstring(path) + L"\"";

            RegSetValueExW(hKey, L"chatclient", 0, REG_SZ,
                (const BYTE*)quotedPath.c_str(), (quotedPath.length() + 1) * sizeof(wchar_t));
        }
        else {
            RegDeleteValueW(hKey, L"chatclient");
        }
        RegCloseKey(hKey);
    }
}

