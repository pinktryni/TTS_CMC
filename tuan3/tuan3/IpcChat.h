#pragma once
// IpcChat.h - Giao tiep 2 app chat tren cung 1 may
//  Cach 1: Windows message (WM_COPYDATA)
//  Cach 2: Named pipe
// Project phai build Unicode (mac dinh VS2019 MFC).

#include <afxwin.h>

// Message tuy bien: pipe thread bao tin nhan moi cho dialog
// lParam = CString* (cap phat bang new, nguoi nhan phai delete)
#define WM_PIPE_MSG   (WM_APP + 1)

// Dinh danh de nhan biet WM_COPYDATA cua app minh
#define CHAT_COPYDATA_ID  0x43484154   // 'CHAT'

namespace IpcChat
{
    // Tieu de cua so dung de FindWindow: "AppChat - <username>"
    CString MakeWindowTitle(const CString& user);

    // Gui bang Windows message (WM_COPYDATA). hFrom = HWND dialog cua minh
    BOOL SendByWinMsg(HWND hFrom, const CString& peerUser, const CString& text);

    // Gui bang Named pipe toi pipe cua peer
    BOOL SendByPipe(const CString& peerUser, const CString& text);
}

// Pipe server: chay 1 thread rieng, nhan tin nhan va PostMessage ve dialog
class CPipeServer
{
public:
    CPipeServer() : m_hThread(NULL), m_hNotify(NULL), m_stop(0) {}
    ~CPipeServer() { Stop(); }

    BOOL Start(HWND hNotify, const CString& myUser);
    void Stop();

private:
    static DWORD WINAPI ThreadProc(LPVOID param);
    void Run();

    HANDLE        m_hThread;
    HWND          m_hNotify;
    CString       m_pipeName;
    volatile LONG m_stop;
};
