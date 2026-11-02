// ServiceMain.cpp
// Entry point cua chuong trinh.
// - Chay "ChatServer.exe /console"  -> chay nhu console app binh thuong, de F5 debug trong VS2019.
// - Chay "ChatServer.exe"           -> dang ky voi Service Control Manager (SCM),
//                                      dung khi da "sc create" va SCM khoi dong no.

#include <windows.h>
#include <string>
#include "Server.h"

static const wchar_t* kServiceName = L"ChatServerSvc";
static const int kPort = 5555;
static const wchar_t* kDbPath = L"chatserver.db";

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

    if (!gServer.Start(kPort, kDbPath)) {
        gStatus.dwCurrentState = SERVICE_STOPPED;
        gStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
        SetServiceStatus(gStatusHandle, &gStatus);
        return;
    }

    gStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(gStatusHandle, &gStatus);

    // Cho den khi co lenh STOP tu SCM (ServiceCtrlHandler se SetEvent)
    WaitForSingleObject(gStopEvent, INFINITE);

    gStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(gStatusHandle, &gStatus);
}

static void RunAsConsole() {
    printf("Chay server o che do console (debug). Nhan Ctrl+C hoac dong cua so de dung.\n");
    if (!gServer.Start(kPort, kDbPath)) {
        printf("Khong the khoi dong server. Xem ChatServer.log de biet chi tiet.\n");
        return;
    }
    printf("Server dang chay tren port %d. Nhan Enter de dung...\n", kPort);
    getchar();
    gServer.Stop();
}

int wmain(int argc, wchar_t* argv[]) {
    bool consoleMode = (argc > 1 && wcscmp(argv[1], L"/console") == 0);

    if (consoleMode) {
        RunAsConsole();
        return 0;
    }

    // Che do Service: dang ky dispatch table va de SCM goi ServiceMain
    SERVICE_TABLE_ENTRYW serviceTable[] = {
        { const_cast<LPWSTR>(kServiceName), (LPSERVICE_MAIN_FUNCTIONW)ServiceMain },
        { nullptr, nullptr }
    };

    if (!StartServiceCtrlDispatcherW(serviceTable)) {
        // Neu loi (thuong la vi khong duoc chay tu SCM), goi y dung che do console
        DWORD err = GetLastError();
        if (err == ERROR_FAILED_SERVICE_CONTROLLER_CONNECT) {
            printf("Khong ket noi duoc SCM. Hay chay 'ChatServer.exe /console' de debug,\n"
                   "hoac dung 'sc create' de cai dat nhu mot Windows Service.\n");
        }
        return 1;
    }

    return 0;
}
