#include "Server.h"
#include "Protocol.h"
#include "PasswordHash.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <ctime>

using json = nlohmann::json;

ChatServer::ChatServer() {}

ChatServer::~ChatServer() {
    Stop();
}

static std::wstring g_baseDir;

void ChatServer::SetBaseDirectory(const std::wstring& dir) {
    g_baseDir = dir;
    if (!g_baseDir.empty() && g_baseDir.back() != L'\\')
        g_baseDir += L'\\';
}

static std::string WideToUtf8Log(const std::wstring& w) {
    if (w.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &out[0], size, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

void ChatServer::Log(const std::string& line) {
    std::string logPath = WideToUtf8Log(g_baseDir) + "ChatServer.log";
    std::ofstream f(logPath, std::ios::app);
    time_t now = time(nullptr);
    char buf[32];
    ctime_s(buf, sizeof(buf), &now);
    std::string ts(buf);
    if (!ts.empty() && ts.back() == '\n') ts.pop_back();
    f << "[" << ts << "] " << line << "\n";
}

bool ChatServer::Start(int port, const std::wstring& dbPath) {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        Log("WSAStartup failed");
        return false;
    }

    if (!db_.Open(dbPath)) {
        Log("Database.Open failed");
        WSACleanup();
        return false;
    }

    listenSocket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket_ == INVALID_SOCKET) {
        Log("socket() failed: " + std::to_string(WSAGetLastError()));
        WSACleanup();
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<u_short>(port));

    if (bind(listenSocket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        Log("bind() failed: " + std::to_string(WSAGetLastError()));
        closesocket(listenSocket_);
        WSACleanup();
        return false;
    }

    if (listen(listenSocket_, SOMAXCONN) == SOCKET_ERROR) {
        Log("listen() failed: " + std::to_string(WSAGetLastError()));
        closesocket(listenSocket_);
        WSACleanup();
        return false;
    }

    running_ = true;
    Log("Server started on port " + std::to_string(port));
    acceptThread_ = std::thread(&ChatServer::AcceptLoop, this);
    return true;
}

void ChatServer::CloseClientSocket(SOCKET sock) {
    bool shouldClose = false;
    {
        std::lock_guard<std::mutex> lock(clientSocketsMutex_);
        shouldClose = clientSockets_.erase(sock) > 0;
    }
    if (shouldClose)
        closesocket(sock);
}

void ChatServer::Stop() {
    if (!running_) return;
    running_ = false;

    if (listenSocket_ != INVALID_SOCKET) {
        closesocket(listenSocket_); 
        listenSocket_ = INVALID_SOCKET;
    }
    if (acceptThread_.joinable())
        acceptThread_.join();

    std::vector<SOCKET> toClose;
    {
        std::lock_guard<std::mutex> lock(clientSocketsMutex_);
        toClose.assign(clientSockets_.begin(), clientSockets_.end());
    }
    for (SOCKET s : toClose) {
        shutdown(s, SD_BOTH);
        CloseClientSocket(s);
    }

    const int maxWaitMs = 5000;
    int waited = 0;
    while (activeClientThreads_.load() > 0 && waited < maxWaitMs) {
        Sleep(50);
        waited += 50;
    }
    if (activeClientThreads_.load() > 0) {
        Log("CANH BAO: con " + std::to_string(activeClientThreads_.load()) +
            " client thread chua thoat sau timeout " + std::to_string(maxWaitMs) + "ms");
    }

    db_.Close();
    WSACleanup();
    Log("Server stopped");
}

void ChatServer::AcceptLoop() {
    while (running_) {
        sockaddr_in clientAddr{};
        int addrLen = sizeof(clientAddr);
        SOCKET clientSocket = accept(listenSocket_, (sockaddr*)&clientAddr, &addrLen);
        if (clientSocket == INVALID_SOCKET) {
            if (!running_) break; 
            continue;
        }
        std::thread(&ChatServer::HandleClient, this, clientSocket).detach();
    }
}

void ChatServer::RegisterOnline(const std::string& username, SOCKET sock) {
    std::lock_guard<std::mutex> lock(onlineMutex_);
    onlineUsers_[username] = sock;
}

void ChatServer::UnregisterOnline(const std::string& username) {
    std::lock_guard<std::mutex> lock(onlineMutex_);
    onlineUsers_.erase(username);
}

void ChatServer::SendOnlinePendingMessages(const std::string& username) {
    auto pending = db_.GetPendingMessages(username);
    SOCKET sock = INVALID_SOCKET;
    {
        std::lock_guard<std::mutex> lock(onlineMutex_);
        auto it = onlineUsers_.find(username);
        if (it == onlineUsers_.end()) return;
        sock = it->second;
    }

    for (auto& m : pending) {
        json push;
        push["type"] = "MESSAGE_PUSH";
        push["from"] = m.fromUser;
        push["content"] = m.content;
        push["time"] = m.timestamp;
        if (proto::SendMessage(sock, push.dump()))
            db_.MarkDelivered(m.id);
    }
}

void ChatServer::HandleRegister(SOCKET sock, const std::string& jsonIn) {
    json resp;
    try {
        json req = json::parse(jsonIn);
        std::string username = req.at("username").get<std::string>();
        std::string password = req.at("password").get<std::string>();

        if (username.empty() || password.empty()) {
            resp["type"] = "REGISTER_FAIL";
            resp["reason"] = "username/password rong";
        }
        else if (db_.UserExists(username)) {
            resp["type"] = "REGISTER_FAIL";
            resp["reason"] = "username da ton tai";
        }
        else {
            std::string hashHex, saltHex;
            pwdhash::HashNewPassword(password, hashHex, saltHex);
            if (db_.RegisterUser(username, hashHex, saltHex)) {
                resp["type"] = "REGISTER_OK";
            }
            else {
                resp["type"] = "REGISTER_FAIL";
                resp["reason"] = "loi DB";
            }
        }
    }
    catch (const std::exception& e) {
        resp["type"] = "ERROR";
        resp["reason"] = std::string("JSON khong hop le: ") + e.what();
    }
    proto::SendMessage(sock, resp.dump());
}

void ChatServer::HandleLogin(SOCKET sock, const std::string& jsonIn, std::string& loggedInUser) {
    json resp;
    try {
        json req = json::parse(jsonIn);
        std::string username = req.at("username").get<std::string>();
        std::string password = req.at("password").get<std::string>();

        int userId = -1;
        if (db_.VerifyLogin(username, password, userId)) {
            resp["type"] = "LOGIN_OK";
            resp["user_id"] = userId;
            loggedInUser = username;
            RegisterOnline(username, sock);
        }
        else {
            resp["type"] = "LOGIN_FAIL";
            resp["reason"] = "sai username hoac password";
        }
    }
    catch (const std::exception& e) {
        resp["type"] = "ERROR";
        resp["reason"] = std::string("JSON khong hop le: ") + e.what();
    }
    proto::SendMessage(sock, resp.dump());

    if (!loggedInUser.empty())
        SendOnlinePendingMessages(loggedInUser);
}

void ChatServer::HandleMessage(SOCKET sock, const std::string& jsonIn, const std::string& fromUser) {
    json resp;
    try {
        json req = json::parse(jsonIn);
        std::string to = req.at("to").get<std::string>();
        std::string content = req.at("content").get<std::string>();
        long long ts = (long long)time(nullptr);

        SOCKET targetSock = INVALID_SOCKET;
        {
            std::lock_guard<std::mutex> lock(onlineMutex_);
            auto it = onlineUsers_.find(to);
            if (it != onlineUsers_.end()) targetSock = it->second;
        }

        bool deliveredNow = false;
        if (targetSock != INVALID_SOCKET) {
            json push;
            push["type"] = "MESSAGE_PUSH";
            push["from"] = fromUser;
            push["content"] = content;
            push["time"] = ts;
            deliveredNow = proto::SendMessage(targetSock, push.dump());
        }

        db_.SaveMessage(fromUser, to, content, ts, deliveredNow);

        resp["type"] = "MESSAGE_ACK";
        resp["status"] = deliveredNow ? "delivered" : "offline";
    }
    catch (const std::exception& e) {
        resp["type"] = "ERROR";
        resp["reason"] = std::string("JSON khong hop le: ") + e.what();
    }
    proto::SendMessage(sock, resp.dump());
}

void ChatServer::HandleGetMessage(SOCKET sock, const std::string& username) {
    auto pending = db_.GetPendingMessages(username);
    json resp;
    resp["type"] = "GETMESSAGE_RESULT";
    json arr = json::array();
    for (auto& m : pending) {
        json item;
        item["from"] = m.fromUser;
        item["content"] = m.content;
        item["time"] = m.timestamp;
        arr.push_back(item);
        db_.MarkDelivered(m.id);
    }
    resp["messages"] = arr;
    proto::SendMessage(sock, resp.dump());
}

void ChatServer::HandleClient(SOCKET clientSocket) {
    activeClientThreads_++;
    {
        std::lock_guard<std::mutex> lock(clientSocketsMutex_);
        clientSockets_.insert(clientSocket);
    }

    std::string loggedInUser;

    while (running_) {
        std::string jsonIn;
        if (!proto::RecvMessage(clientSocket, jsonIn)) break; 

        std::string type;
        try {
            json req = json::parse(jsonIn);
            type = req.value("type", "");
        }
        catch (...) {
            json err; err["type"] = "ERROR"; err["reason"] = "JSON khong hop le";
            proto::SendMessage(clientSocket, err.dump());
            continue;
        }

        if (type == "REGISTER") {
            HandleRegister(clientSocket, jsonIn);
        }
        else if (type == "LOGIN") {
            HandleLogin(clientSocket, jsonIn, loggedInUser);
        }
        else if (type == "MESSAGE") {
            if (loggedInUser.empty()) {
                json err; err["type"] = "ERROR"; err["reason"] = "chua dang nhap";
                proto::SendMessage(clientSocket, err.dump());
            }
            else {
                HandleMessage(clientSocket, jsonIn, loggedInUser);
            }
        }
        else if (type == "GETMESSAGE") {
            if (loggedInUser.empty()) {
                json err; err["type"] = "ERROR"; err["reason"] = "chua dang nhap";
                proto::SendMessage(clientSocket, err.dump());
            }
            else {
                HandleGetMessage(clientSocket, loggedInUser);
            }
        }
        else {
            json err; err["type"] = "ERROR"; err["reason"] = "type khong duoc ho tro: " + type;
            proto::SendMessage(clientSocket, err.dump());
        }
    }

    if (!loggedInUser.empty())
        UnregisterOnline(loggedInUser);

    CloseClientSocket(clientSocket);
    activeClientThreads_--;
}