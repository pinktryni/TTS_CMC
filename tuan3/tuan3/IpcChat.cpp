#include "pch.h"          // doi thanh "stdafx.h" neu project cua ban dung stdafx
#include "IpcChat.h"

#define PIPE_PREFIX   L"\\\\.\\pipe\\AppChatPipe_"
#define MAX_MSG_CHARS 1000

static CString MakePipeName(const CString& user)
{
    return CString(PIPE_PREFIX) + user;
}

// ---------------------------------------------------------------
// Windows message (WM_COPYDATA)
// ---------------------------------------------------------------
CString IpcChat::MakeWindowTitle(const CString& user)
{
    return L"AppChat - " + user;
}

BOOL IpcChat::SendByWinMsg(HWND hFrom, const CString& peerUser, const CString& text)
{
    CString title = MakeWindowTitle(peerUser);
    HWND hPeer = ::FindWindow(NULL, title);     // tim cua so app kia theo title
    if (!hPeer)
        return FALSE;

    COPYDATASTRUCT cds = { 0 };
    cds.dwData = CHAT_COPYDATA_ID;
    cds.cbData = (text.GetLength() + 1) * sizeof(WCHAR);   // gom ca ky tu NULL
    cds.lpData = (PVOID)(LPCWSTR)text;

    // WM_COPYDATA bat buoc dung SendMessage (khong dung PostMessage)
    ::SendMessage(hPeer, WM_COPYDATA, (WPARAM)hFrom, (LPARAM)&cds);
    return TRUE;
}

// ---------------------------------------------------------------
// Named pipe - phia gui (client)
// ---------------------------------------------------------------
BOOL IpcChat::SendByPipe(const CString& peerUser, const CString& text)
{
    CString name = MakePipeName(peerUser);

    if (!::WaitNamedPipe(name, 1000))           // doi pipe san sang toi da 1s
        return FALSE;

    HANDLE h = ::CreateFile(name, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h == INVALID_HANDLE_VALUE)
        return FALSE;

    DWORD written = 0;
    DWORD bytes = (min(text.GetLength(), MAX_MSG_CHARS)) * sizeof(WCHAR);
    BOOL ok = ::WriteFile(h, (LPCWSTR)text, bytes, &written, NULL);

    ::CloseHandle(h);
    return ok;
}

// ---------------------------------------------------------------
// Named pipe - phia nhan (server, chay thread rieng)
// ---------------------------------------------------------------
BOOL CPipeServer::Start(HWND hNotify, const CString& myUser)
{
    if (m_hThread) return TRUE;

    m_hNotify  = hNotify;
    m_pipeName = MakePipeName(myUser);
    m_stop     = 0;

    m_hThread = ::CreateThread(NULL, 0, ThreadProc, this, 0, NULL);
    return m_hThread != NULL;
}

void CPipeServer::Stop()
{
    if (!m_hThread) return;

    ::InterlockedExchange(&m_stop, 1);

    // ConnectNamedPipe dang block -> tu ket noi vao pipe cua minh de "danh thuc" thread
    HANDLE h = ::CreateFile(m_pipeName, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    if (h != INVALID_HANDLE_VALUE)
        ::CloseHandle(h);

    ::WaitForSingleObject(m_hThread, 3000);
    ::CloseHandle(m_hThread);
    m_hThread = NULL;
}

DWORD WINAPI CPipeServer::ThreadProc(LPVOID param)
{
    ((CPipeServer*)param)->Run();
    return 0;
}

void CPipeServer::Run()
{
    while (!m_stop)
    {
        HANDLE hPipe = ::CreateNamedPipe(
            m_pipeName,
            PIPE_ACCESS_INBOUND,                                   // server chi doc
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            0, (MAX_MSG_CHARS + 1) * sizeof(WCHAR), 0, NULL);

        if (hPipe == INVALID_HANDLE_VALUE)
            break;

        BOOL connected = ::ConnectNamedPipe(hPipe, NULL)
                         ? TRUE : (::GetLastError() == ERROR_PIPE_CONNECTED);

        if (connected && !m_stop)
        {
            WCHAR buf[MAX_MSG_CHARS + 1] = { 0 };
            DWORD n = 0;
            if (::ReadFile(hPipe, buf, MAX_MSG_CHARS * sizeof(WCHAR), &n, NULL) && n > 0)
            {
                buf[n / sizeof(WCHAR)] = 0;
                // Khong cham vao UI tu thread phu -> PostMessage ve UI thread
                CString* pMsg = new CString(buf);
                if (!::PostMessage(m_hNotify, WM_PIPE_MSG, 0, (LPARAM)pMsg))
                    delete pMsg;
            }
        }

        ::DisconnectNamedPipe(hPipe);
        ::CloseHandle(hPipe);
    }
}
