#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include "Server.h"

#pragma comment(lib, "Advapi32.lib") 

static const wchar_t* kServiceName = L"BTL";
static const int kPort = 5555;

static std::wstring GetExeDirectory() {
    wchar_t path[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, path, MAX_PATH);
    if (len == 0 || len == MAX_PATH) return L".";
    std::wstring full(path, len);
    size_t pos = full.find_last_of(L'\\');
    if (pos == std::wstring::npos) return L".";
    return full.substr(0, pos);
}

static std::string WideToUtf8(const std::wstring& w) {
    if (w.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &out[0], size, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

static SERVICE_STATUS gStatus = {};
static SERVICE_STATUS_HANDLE gStatusHandle = nullptr;
static ChatServer gServer;
static HANDLE gStopEvent = nullptr;

void WINAPI ServiceCtrlHandler(DWORD ctrlCode) {
    switch (ctrlCode) {
    case SERVICE_CONTROL_STOP:
    case SERVICE_CONTROL_SHUTDOWN:
        gStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(gStatusHandle, &gStatus);
        gServer.Stop();
        SetEvent(gStopEvent);
        break;
    default:
        break;
    }
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv) {
    gStatusHandle = RegisterServiceCtrlHandlerW(kServiceName, ServiceCtrlHandler);
    if (!gStatusHandle) return;

    ZeroMemory(&gStatus, sizeof(gStatus));
    gStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    gStatus.dwCurrentState = SERVICE_START_PENDING;
    gStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN;
    SetServiceStatus(gStatusHandle, &gStatus);

    gStopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);

    std::wstring exeDir = GetExeDirectory();
    ChatServer::SetBaseDirectory(exeDir);
    std::wstring dbPath = exeDir + L"\\chatserver.db";

    if (!gServer.Start(kPort, dbPath)) {
        gStatus.dwCurrentState = SERVICE_STOPPED;
        gStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        SetServiceStatus(gStatusHandle, &gStatus);
        return;
    }

    gStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(gStatusHandle, &gStatus);

    WaitForSingleObject(gStopEvent, INFINITE);

    gStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(gStatusHandle, &gStatus);
}

static bool InstallService() {
    wchar_t exePath[MAX_PATH];
    DWORD len = GetModuleFileNameW(nullptr, exePath, MAX_PATH);
    if (len == 0 || len == MAX_PATH) {
        ChatServer::Log("InstallService: khong lay duoc duong dan .exe");
        return false;
    }

    std::wstring quotedPath = L"\"" + std::wstring(exePath) + L"\"";

    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!scm) {
        ChatServer::Log("InstallService: khong mo duoc SCM (can quyen Administrator). Loi: " +
            std::to_string(GetLastError()));
        return false;
    }

    SC_HANDLE svc = CreateServiceW(
        scm,
        kServiceName,
        L"Chat Server (BTL)",
        SERVICE_ALL_ACCESS,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        quotedPath.c_str(),
        nullptr, nullptr, nullptr, nullptr, nullptr
    );

    if (!svc) {
        DWORD err = GetLastError();
        if (err == ERROR_SERVICE_EXISTS)
            ChatServer::Log("InstallService: service da ton tai (dung /uninstall truoc neu muon cai lai)");
        else
            ChatServer::Log("InstallService: CreateServiceW that bai. Loi: " + std::to_string(err));
        CloseServiceHandle(scm);
        return false;
    }

    ChatServer::Log("InstallService: da cai dat service '" + WideToUtf8(kServiceName) + "' thanh cong");

    if (StartServiceW(svc, 0, nullptr))
        ChatServer::Log("InstallService: da khoi dong service");
    else
        ChatServer::Log("InstallService: cai dat xong nhung chua khoi dong duoc ngay (hay thu 'sc start " +
            WideToUtf8(kServiceName) + "')");

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return true;
}

static bool UninstallService() {
    SC_HANDLE scm = OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scm) {
        ChatServer::Log("UninstallService: khong mo duoc SCM (can quyen Administrator). Loi: " +
            std::to_string(GetLastError()));
        return false;
    }

    SC_HANDLE svc = OpenServiceW(scm, kServiceName, DELETE | SERVICE_STOP | SERVICE_QUERY_STATUS);
    if (!svc) {
        ChatServer::Log("UninstallService: khong tim thay service (co the chua cai hoac da go roi)");
        CloseServiceHandle(scm);
        return false;
    }

    SERVICE_STATUS status{};
    ControlService(svc, SERVICE_CONTROL_STOP, &status);

    bool ok = DeleteService(svc) != 0;
    if (ok)
        ChatServer::Log("UninstallService: da go cai dat service");
    else
        ChatServer::Log("UninstallService: DeleteService that bai. Loi: " + std::to_string(GetLastError()));

    CloseServiceHandle(svc);
    CloseServiceHandle(scm);
    return ok;
}

static void RunAsConsole() {
    std::wstring exeDir = GetExeDirectory();
    ChatServer::SetBaseDirectory(exeDir);
    std::wstring dbPath = exeDir + L"\\chatserver.db";

    if (!gServer.Start(kPort, dbPath)) {
        return;
    }
    getchar();
    gServer.Stop();
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc > 1) {
        std::wstring arg = argv[1];
        if (arg == L"/console") {
            RunAsConsole();
            return 0;
        }
        if (arg == L"/install") {
            InstallService();
            return 0;
        }
        if (arg == L"/uninstall") {
            UninstallService();
            return 0;
        }
    }

    SERVICE_TABLE_ENTRYW serviceTable[] = {
        { const_cast<LPWSTR>(kServiceName), (LPSERVICE_MAIN_FUNCTIONW)ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcherW(serviceTable)) {
        DWORD err = GetLastError();
        if (err == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            ChatServer::Log("wmain: khong ket noi duoc SCM. Chay 'BTL.exe /console' de debug, "
                "hoac 'BTL.exe /install' de cai dat nhu Windows Service.");
        }
        return 1;
    }

    return 0;
}