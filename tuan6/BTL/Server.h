#pragma once
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <thread>
#include <vector>
#include "Database.h"

class ChatServer {
public:
    ChatServer();
    ~ChatServer();

    bool Start(int port, const std::wstring& dbPath);
    void Stop();

    static void Log(const std::string& line);

    static void SetBaseDirectory(const std::wstring& dir);

private:
    void AcceptLoop();
    void HandleClient(SOCKET clientSocket);

    void HandleRegister(SOCKET sock, const std::string& jsonIn);
    void HandleLogin(SOCKET sock, const std::string& jsonIn, std::string& loggedInUser);
    void HandleMessage(SOCKET sock, const std::string& jsonIn, const std::string& fromUser);
    void HandleGetMessage(SOCKET sock, const std::string& username);

    void CloseClientSocket(SOCKET sock);

    void SendOnlinePendingMessages(const std::string& username);
    void RegisterOnline(const std::string& username, SOCKET sock);
    void UnregisterOnline(const std::string& username);

    SOCKET listenSocket_ = INVALID_SOCKET;
    std::atomic<bool> running_{ false };
    Database db_;

    std::unordered_map<std::string, SOCKET> onlineUsers_;
    std::mutex onlineMutex_;

    std::thread acceptThread_;

    std::unordered_set<SOCKET> clientSockets_;
    std::mutex clientSocketsMutex_;
    std::atomic<int> activeClientThreads_{ 0 };
};