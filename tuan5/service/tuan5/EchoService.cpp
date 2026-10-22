#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winsvc.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <thread>
#include <mutex>

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Advapi32.lib")

static const wchar_t* SERVICE_NAME = L"Tuan5";
static const wchar_t* SERVICE_DISPLAY_NAME = L"Tuần 5";
static const int LISTEN_PORT = 9999;

static SERVICE_STATUS        g_ServiceStatus = { 0 };
static SERVICE_STATUS_HANDLE g_StatusHandle = nullptr;
static HANDLE                g_hStopEvent = nullptr;

static SOCKET                g_listenSocket = INVALID_SOCKET;
static std::vector<std::thread> g_clientThreads;
static std::mutex            g_threadsMutex;

static void HandleClient(SOCKET clientSocket, std::string clientAddr)
{
    char buffer[4096];

    while (WaitForSingleObject(g_hStopEvent, 0) != WAIT_OBJECT_0)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);

        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int sel = select(0, &readfds, nullptr, nullptr, &tv);
        if (sel == SOCKET_ERROR) {
            break;
        }
        if (sel == 0) {
            continue;
        }

        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);
        if (bytesReceived > 0) {
            int sent = 0;
            while (sent < bytesReceived) {
                int n = send(clientSocket, buffer + sent, bytesReceived - sent, 0);
                if (n == SOCKET_ERROR) { sent = -1; break; }
                sent += n;
            }
            if (sent < 0) {
                break;
            }
        }
        else if (bytesReceived == 0) {
            break;
        }
        else {
            break;
        }
    }

    closesocket(clientSocket);
}

static void ServiceWorker()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return;
    }

    g_listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_listenSocket == INVALID_SOCKET) {
        WSACleanup();
        return;
    }

    BOOL reuse = TRUE;
    setsockopt(g_listenSocket, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.s_addr = INADDR_ANY;
    service.sin_port = htons(LISTEN_PORT);

    if (bind(g_listenSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR) {
        closesocket(g_listenSocket);
        WSACleanup();
        return;
    }

    if (listen(g_listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(g_listenSocket);
        WSACleanup();
        return;
    }

    while (WaitForSingleObject(g_hStopEvent, 0) != WAIT_OBJECT_0)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(g_listenSocket, &readfds);

        timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int sel = select(0, &readfds, nullptr, nullptr, &tv);
        if (sel == SOCKET_ERROR) {
            if (WaitForSingleObject(g_hStopEvent, 0) == WAIT_OBJECT_0) break;
            break;
        }
        if (sel == 0) continue;

        sockaddr_in clientInfo;
        int clientInfoLen = sizeof(clientInfo);
        SOCKET clientSocket = accept(g_listenSocket, (SOCKADDR*)&clientInfo, &clientInfoLen);
        if (clientSocket == INVALID_SOCKET) {
            if (WaitForSingleObject(g_hStopEvent, 0) == WAIT_OBJECT_0) break;
            continue;
        }

        char ipStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientInfo.sin_addr, ipStr, sizeof(ipStr));
        std::string clientAddr = std::string(ipStr) + ":" + std::to_string(ntohs(clientInfo.sin_port));

        std::lock_guard<std::mutex> lock(g_threadsMutex);
        g_clientThreads.emplace_back(HandleClient, clientSocket, clientAddr);
    }

    if (g_listenSocket != INVALID_SOCKET) {
        closesocket(g_listenSocket);
        g_listenSocket = INVALID_SOCKET;
    }

    {
        std::lock_guard<std::mutex> lock(g_threadsMutex);
        for (auto& t : g_clientThreads) {
            if (t.joinable()) t.join();
        }
        g_clientThreads.clear();
    }

    WSACleanup();
}

static void ReportStatus(DWORD state, DWORD exitCode, DWORD waitHint)
{
    static DWORD checkpoint = 1;
    g_ServiceStatus.dwCurrentState = state;
    g_ServiceStatus.dwWin32ExitCode = exitCode;
    g_ServiceStatus.dwWaitHint = waitHint;

    g_ServiceStatus.dwControlsAccepted =
        (state == SERVICE_START_PENDING) ? 0 : SERVICE_ACCEPT_STOP;

    g_ServiceStatus.dwCheckPoint =
        (state == SERVICE_RUNNING || state == SERVICE_STOPPED) ? 0 : checkpoint++;

    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

static void WINAPI ServiceCtrlHandler(DWORD ctrlCode)
{
    switch (ctrlCode)
    {
    case SERVICE_CONTROL_STOP:
        if (g_ServiceStatus.dwCurrentState != SERVICE_RUNNING) break;
        ReportStatus(SERVICE_STOP_PENDING, NO_ERROR, 4000);
        SetEvent(g_hStopEvent);
        break;
    default:
        break;
    }
}

static void WINAPI SvcMain(DWORD argc, LPTSTR* argv)
{
    g_StatusHandle = RegisterServiceCtrlHandler(SERVICE_NAME, ServiceCtrlHandler);
    if (!g_StatusHandle) return;

    ZeroMemory(&g_ServiceStatus, sizeof(g_ServiceStatus));
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwServiceSpecificExitCode = 0;

    ReportStatus(SERVICE_START_PENDING, NO_ERROR, 3000);

    g_hStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (!g_hStopEvent) {
        ReportStatus(SERVICE_STOPPED, GetLastError(), 0);
        return;
    }

    ReportStatus(SERVICE_RUNNING, NO_ERROR, 0);

    ServiceWorker();

    CloseHandle(g_hStopEvent);
    g_hStopEvent = nullptr;

    ReportStatus(SERVICE_STOPPED, NO_ERROR, 0);
}

static void InstallService()
{
    wchar_t path[MAX_PATH];
    if (!GetModuleFileName(nullptr, path, MAX_PATH)) {
        return;
    }

    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!scm) {
        return;
    }

    SC_HANDLE svc = CreateService(
        scm, SERVICE_NAME, SERVICE_DISPLAY_NAME,
        SERVICE_ALL_ACCESS, SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        path, nullptr, nullptr, nullptr, nullptr, nullptr);

    if (svc) {
        CloseServiceHandle(svc);
    }
    CloseServiceHandle(scm);
}

static void UninstallService()
{
    SC_HANDLE scm = OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS);
    if (!scm) {
        return;
    }

    SC_HANDLE svc = OpenService(scm, SERVICE_NAME, SERVICE_ALL_ACCESS);
    if (!svc) {
        CloseServiceHandle(scm);
        return;
    }

    SERVICE_STATUS status;
    ControlService(svc, SERVICE_CONTROL_STOP, &status);
    DeleteService(svc);

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
}

int wmain(int argc, wchar_t* argv[])
{
    if (argc > 1 && _wcsicmp(argv[1], L"/install") == 0) {
        InstallService();
        return 0;
    }
    if (argc > 1 && _wcsicmp(argv[1], L"/uninstall") == 0) {
        UninstallService();
        return 0;
    }

    SERVICE_TABLE_ENTRY serviceTable[] = {
        { (LPWSTR)SERVICE_NAME, (LPSERVICE_MAIN_FUNCTION)SvcMain },
        { nullptr, nullptr }
    };

    StartServiceCtrlDispatcher(serviceTable);
    return 0;
}