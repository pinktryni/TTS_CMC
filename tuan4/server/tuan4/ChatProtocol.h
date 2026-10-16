#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#pragma comment(lib, "Ws2_32.lib")

#define WM_CHAT_RECV    (WM_APP + 1)   
#define WM_CHAT_STATUS  (WM_APP + 2)   
#define WM_CHAT_PEER    (WM_APP + 3)   

static const UINT32 MAX_MSG_BYTES = 10 * 1024 * 1024;  

inline bool SendAll(SOCKET s, const char* buf, int len)
{
    int sent = 0;
    while (sent < len)
    {
        int n = send(s, buf + sent, len - sent, 0);
        if (n == SOCKET_ERROR || n == 0) return false;
        sent += n;
    }
    return true;
}

inline bool RecvAll(SOCKET s, char* buf, int len)
{
    int got = 0;
    while (got < len)
    {
        int n = recv(s, buf + got, len - got, 0);
        if (n == SOCKET_ERROR || n == 0) return false;  
        got += n;
    }
    return true;
}

inline bool SendMsg(SOCKET s, const CStringW& text)
{
    int ulen = WideCharToMultiByte(CP_UTF8, 0, text, text.GetLength(), nullptr, 0, nullptr, nullptr);
    std::string buf(ulen, '\0');
    if (ulen > 0)
        WideCharToMultiByte(CP_UTF8, 0, text, text.GetLength(), &buf[0], ulen, nullptr, nullptr);

    UINT32 netLen = htonl((UINT32)ulen);
    return SendAll(s, (const char*)&netLen, 4) && SendAll(s, buf.data(), ulen);
}

inline bool RecvMsg(SOCKET s, CStringW& out)
{
    UINT32 netLen = 0;
    if (!RecvAll(s, (char*)&netLen, 4)) return false;

    UINT32 len = ntohl(netLen);
    if (len > MAX_MSG_BYTES) return false;

    out.Empty();
    if (len == 0) return true;

    std::string buf(len, '\0');
    if (!RecvAll(s, &buf[0], (int)len)) return false;

    int wlen = MultiByteToWideChar(CP_UTF8, 0, buf.data(), (int)len, nullptr, 0);
    if (wlen <= 0) return false;
    wchar_t* p = out.GetBuffer(wlen);
    MultiByteToWideChar(CP_UTF8, 0, buf.data(), (int)len, p, wlen);
    out.ReleaseBuffer(wlen);
    return true;
}

inline void PostText(HWND hWnd, UINT msg, const CStringW& s)
{
    CStringW* p = new CStringW(s);
    if (!::PostMessage(hWnd, msg, 0, (LPARAM)p)) delete p;
}

inline void AppendRich(CRichEditCtrl& rich, const CStringW& who, const CStringW& text, COLORREF clr)
{
    const LONG BUBBLE_INDENT = 1500;   

    bool isStatus = who.IsEmpty();
    bool isMine = (who == L"Me");

    PARAFORMAT2 pf = {};
    pf.cbSize = sizeof(pf);
    pf.dwMask = PFM_ALIGNMENT | PFM_STARTINDENT | PFM_RIGHTINDENT | PFM_SPACEAFTER;
    pf.wAlignment = isStatus ? PFA_CENTER : (isMine ? PFA_RIGHT : PFA_LEFT);
    pf.dxStartIndent = (!isStatus && isMine) ? BUBBLE_INDENT : 0;  
    pf.dxRightIndent = (!isStatus && !isMine) ? BUBBLE_INDENT : 0;   
    pf.dySpaceAfter = 0;

    CHARFORMAT2 cf = {};
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BOLD;

    rich.SetSel(-1, -1);                      

    if (!isStatus)
    {
        rich.SetParaFormat(pf);

        CStringW t = CTime::GetCurrentTime().Format(L"%H:%M:%S");

        if (isMine)
        {

            cf.dwEffects = 0;
            cf.crTextColor = RGB(128, 128, 128);
            rich.SetSelectionCharFormat(cf);
            rich.ReplaceSel(t + L"  ");

            cf.dwEffects = CFE_BOLD;
            cf.crTextColor = clr;
            rich.SetSelectionCharFormat(cf);
            rich.ReplaceSel(who + L"\r\n");
        }
        else
        {
            cf.dwEffects = CFE_BOLD;
            cf.crTextColor = clr;
            rich.SetSelectionCharFormat(cf);
            rich.ReplaceSel(who);

            cf.dwEffects = 0;
            cf.crTextColor = RGB(128, 128, 128);
            rich.SetSelectionCharFormat(cf);
            rich.ReplaceSel(L"  " + t + L"\r\n");
        }

        rich.SetSel(-1, -1);
    }

    pf.dySpaceAfter = 120;            
    rich.SetParaFormat(pf);

    cf.dwEffects = 0;
    cf.crTextColor = isStatus ? RGB(128, 128, 128) : RGB(0, 0, 0);
    rich.SetSelectionCharFormat(cf);
    rich.ReplaceSel(text + L"\r\n");

    rich.SendMessage(WM_VSCROLL, SB_BOTTOM, 0);
}