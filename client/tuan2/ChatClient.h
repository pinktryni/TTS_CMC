#pragma once
// ChatClient.h
// Lop ket noi toi ChatServer, thay the hoan toan FakeData.h.
// Dung thuan Winsock2 (thu vien Windows, Ws2_32.dll) -- khong dung thu vien socket
// kieu C/POSIX (vd <sys/socket.h>) vi do la cua Linux, khong chay duoc tren Windows.
//
// 2 nhom thao tac:
//  - DONG BO (blocking): Connect(), Register(), Login() -- dung TRUOC khi vao man hinh
//    chat, vi luc nay chi co 1 request/response, chua can nghe lien tuc.
//  - NEN (background thread): StartListening() -- dung SAU khi da login, de nhan
//    MESSAGE_PUSH (tin nhan moi do nguoi khac gui toi) ma khong phai "hoi lien tuc".

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <map>

// Cac custom window message, dung de thread nen gui du lieu AN TOAN ve cho UI thread xu ly.
// LY DO BAT BUOC: MFC/Win32 controls (CEdit, CRichEditCtrl...) CHI duoc phep dung tu dung
// 1 thread da tao ra chung (UI thread). Neu thread nen tu y goi SetWindowText(),... tu thread
// khac se gay loi kho doan (crash ngau nhien, UI ve sai). PostMessage() la cach an toan duy
// nhat de "nho" UI thread tu lam viec do, thay vi thread nen tu lam.
#define WM_CHAT_MESSAGE_PUSH   (WM_APP + 1)   // wParam = 0, lParam = ChatPushData* (nguoi nhan PHAI delete)
#define WM_CHAT_PENDING_LIST   (WM_APP + 2)   // wParam = 0, lParam = std::vector<ChatPushData>* (PHAI delete)
#define WM_CHAT_DISCONNECTED   (WM_APP + 3)   // ket noi toi server bi mat (loi mang, server tat...)
#define WM_CHAT_HISTORY_RESULT (WM_APP + 4)
#define WM_CHAT_STATUS_PUSH    (WM_APP + 5) 

struct ChatPushData {
    std::wstring from;
    std::wstring content;
    long long    time = 0;
};

struct ChatHistoryData {
    std::wstring withUser;
    std::vector<ChatPushData> messages;
};

class ChatClient {
public:
    ChatClient();
    ~ChatClient();

    // Mo ket noi TCP toi server. Goi 1 lan duy nhat luc dau (man hinh Login).
    bool Connect(const std::wstring& serverIp, int port, std::wstring& outError);
    void Disconnect();

    // --- Dong bo: gui 1 request, cho dung 1 response tuong ung, roi tra ve ngay ---
    // Dung o man hinh Login/Register (chua can thread nen, vi chua co ai nhan tin cho minh).
    bool Register(const std::wstring& username, const std::wstring& fullname, const std::wstring& password, std::wstring& outError);
    bool Login(const std::wstring& username, const std::wstring& password, std::wstring& outError);

    // --- Thread nen: bat dau SAU khi Login() thanh cong ---
    // hWndTarget: handle cua dialog se nhan cac WM_CHAT_* o tren (thuong la CMainDlg).
    void StartListening(HWND hWndTarget);
    void StopListening();

    // Gui 1 tin nhan chat. KHONG cho doi MESSAGE_ACK (fire-and-forget tu phia UI) --
    // ket qua gui duoc hay khong se duoc ListenLoop() nhan ngam, khong chan UI thread.
    bool SendChatMessage(const std::wstring& to, const std::wstring& content);

    // Yeu cau server day cac tin nhan dang cho (GETMESSAGE) -- ket qua se tra ve qua
    // WM_CHAT_PENDING_LIST trong ListenLoop(), khong tra ve truc tiep tai day.
    bool RequestPendingMessages();

    bool RequestChatHistory(const std::wstring& withUser);
    std::wstring GetFullName(const std::wstring& username) const;

    bool IsConnected() const { return socket_ != INVALID_SOCKET; }
    const std::vector<std::wstring>& GetCachedUsers() const { return cachedUsers_; }
    const std::wstring& GetCurrentUsername() const { return currentUsername_; }

    bool SetAvatar(const std::wstring& username, const std::string& base64Str);
    std::vector<BYTE> GetAvatar(const std::wstring& username) const;

    bool IsUserOnline(const std::wstring& username) const;
    void SetUserOnline(const std::wstring& username, bool online);

private:
    void ListenLoop();

    static std::string  WideToUtf8(const std::wstring& w);
    static std::wstring Utf8ToWide(const std::string& s);

    SOCKET socket_ = INVALID_SOCKET;
    bool   wsaInitialized_ = false;

    std::thread       listenThread_;
    std::atomic<bool> listening_{ false };
    HWND              hWndTarget_ = nullptr;

    // Bao ve viec nhieu noi cung goi send() tren 1 socket dong thoi (UI thread goi
    // SendChatMessage()/RequestPendingMessages(), trong khi ListenLoop() dang doc o thread khac).
    // send() va recv() tren Winsock an toan de goi dong thoi tu 2 thread khac nhau (1 ben gui,
    // 1 ben nhan), nhung NEU co > 1 thread cung goi send() thi can khoa -- o day chi co UI
    // thread goi send() nen thuc ra khong bat buoc, nhung giu lai cho an toan/mo rong sau nay.
    std::mutex sendMutex_;
    std::vector<std::wstring> cachedUsers_;
    std::wstring              currentUsername_;
    std::map<std::wstring, std::wstring> userFullnames_;
    std::map<std::wstring, std::vector<BYTE>> userAvatars_;
    std::map<std::wstring, bool> userStatus_;
    mutable std::mutex statusMutex_;
};

// Instance dung chung cho toan bo app (giong theApp cua MFC) -- chi co 1 ket noi toi
// server trong suot vong doi app, nen khong can tao nhieu ChatClient rieng cho tung dialog.
// Dinh nghia that su nam trong ChatClient.cpp.
extern ChatClient g_chatClient;

// Dia chi server mac dinh. Doi sang IP that cua VPS khi trien khai thuc te.
extern const wchar_t* kServerIp;
extern const int kServerPort;
