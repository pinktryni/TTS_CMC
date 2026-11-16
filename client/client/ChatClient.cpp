#include "pch.h"
#include "ChatClient.h"
#include "Protocol.h"
#include <nlohmann/json.hpp>
#include <wincrypt.h>
#include "Utils.h"

#pragma comment(lib, "Crypt32.lib")

using json = nlohmann::json;

ChatClient g_chatClient;
const wchar_t* kServerIp = L"127.0.0.1"; 
const int kServerPort = 5555;

ChatClient::ChatClient() {}

ChatClient::~ChatClient() {
    Disconnect();
}

std::string ChatClient::WideToUtf8(const std::wstring& w) {
    if (w.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string out(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, w.c_str(), -1, &out[0], size, nullptr, nullptr);
    if (!out.empty() && out.back() == '\0') out.pop_back();
    return out;
}

std::wstring ChatClient::Utf8ToWide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, nullptr, 0);
    std::wstring out(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, s.c_str(), -1, &out[0], size);
    if (!out.empty() && out.back() == L'\0') out.pop_back();
    return out;
}

bool ChatClient::Connect(const std::wstring& serverIp, int port, std::wstring& outError) {
    if (socket_ != INVALID_SOCKET) {
        outError = L"Da ket noi roi, goi Disconnect() truoc neu muon ket noi lai.";
        return false;
    }

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        outError = L"WSAStartup that bai.";
        return false;
    }
    wsaInitialized_ = true;

    socket_ = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_ == INVALID_SOCKET) {
        outError = L"Khong tao duoc socket. Loi: " + std::to_wstring(WSAGetLastError());
        WSACleanup();
        wsaInitialized_ = false;
        return false;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<u_short>(port));

    if (InetPtonW(AF_INET, serverIp.c_str(), &addr.sin_addr) != 1) {
        outError = L"Dia chi IP khong hop le: " + serverIp;
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
        WSACleanup();
        wsaInitialized_ = false;
        return false;
    }

    if (connect(socket_, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        outError = L"Khong ket noi duoc toi server. Loi: " + std::to_wstring(WSAGetLastError());
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
        WSACleanup();
        wsaInitialized_ = false;
        return false;
    }

    return true;
}

void ChatClient::Disconnect() {
    StopListening();
    if (wsaInitialized_) {
        WSACleanup();
        wsaInitialized_ = false;
    }
}

bool ChatClient::Register(const std::wstring& username, const std::wstring& fullname, const std::wstring& password, std::wstring& outError) {
    if (socket_ == INVALID_SOCKET) {
        outError = L"Chua ket noi toi server.";
        return false;
    }

    json req;
    req["type"] = "REGISTER";
    req["username"] = WideToUtf8(username);
    req["fullname"] = WideToUtf8(fullname);
    req["password"] = WideToUtf8(password);

    std::lock_guard<std::mutex> lock(sendMutex_);
    if (!proto::SendMessage(socket_, req.dump())) {
        outError = L"Gui du lieu that bai (mat ket noi?).";
        return false;
    }

    std::string respText;
    if (!proto::RecvMessage(socket_, respText)) {
        outError = L"Khong nhan duoc phan hoi tu server (mat ket noi?).";
        return false;
    }

    try {
        json resp = json::parse(respText);
        std::string type = resp.value("type", "");
        if (type == "REGISTER_OK") {
            return true;
        } else {
            std::string reason = resp.value("reason", "Dang ky that bai");
            outError = Utf8ToWide(reason);
            return false;
        }
    } catch (const std::exception&) {
        outError = L"Phan hoi tu server khong hop le.";
        return false;
    }
}

bool ChatClient::Login(const std::wstring& username, const std::wstring& password, std::wstring& outError) {
    if (socket_ == INVALID_SOCKET) {
        outError = L"Chua ket noi toi server.";
        return false;
    }

    json req;
    req["type"] = "LOGIN";
    req["username"] = WideToUtf8(username);
    req["password"] = WideToUtf8(password);

    std::lock_guard<std::mutex> lock(sendMutex_);
    if (!proto::SendMessage(socket_, req.dump())) {
        outError = L"Gui du lieu that bai.";
        return false;
    }

    // SỬ DỤNG VÒNG LẶP ĐỂ CHỜ ĐÚNG GÓI TIN LOGIN
    while (true) {
        std::string respText;
        if (!proto::RecvMessage(socket_, respText)) {
            outError = L"Khong nhan duoc phan hoi tu server.";
            return false;
        }

        try {
            json resp = json::parse(respText);
            std::string type = resp.value("type", "");

            if (type == "LOGIN_OK") {
                currentUsername_ = username;
                cachedUsers_.clear();
                userFullnames_.clear();
                {
                    std::lock_guard<std::mutex> lockStatus(statusMutex_);
                    userStatus_.clear();
                }

                if (resp.contains("users") && resp["users"].is_array()) {
                    for (auto& u : resp["users"]) {
                        std::wstring uname = Utf8ToWide(u.value("u", ""));
                        std::wstring fname = Utf8ToWide(u.value("n", ""));
                        std::string avatarB64 = u.value("a", "");

                        cachedUsers_.push_back(uname);
                        userFullnames_[uname] = fname;
                        bool isOnline = u.value("o", false);
                        SetUserOnline(uname, isOnline);

                        if (!avatarB64.empty()) {
                            userAvatars_[uname] = Base64Decode(avatarB64);
                        }
                    }
                }
                return true; // Đăng nhập thành công
            }
            else if (type == "LOGIN_FAIL") {
                std::string reason = resp.value("reason", "Dang nhap that bai");
                outError = Utf8ToWide(reason);
                return false; // Đăng nhập thất bại thực sự
            }
            // Nếu type là "STATUS_PUSH" hay bất kỳ gói tin nào khác, vòng lặp sẽ tự động chạy tiếp (continue)
        }
        catch (const std::exception&) {
            outError = L"Phan hoi tu server khong hop le.";
            return false;
        }
    }
}

std::wstring ChatClient::GetFullName(const std::wstring& username) const {
    auto it = userFullnames_.find(username);
    if (it != userFullnames_.end()) return it->second;
    return username;
}
void ChatClient::StartListening(HWND hWndTarget) {
    if (listening_) return;
    hWndTarget_ = hWndTarget;
    listening_ = true;
    listenThread_ = std::thread(&ChatClient::ListenLoop, this);
}

void ChatClient::StopListening() {
    listening_ = false;
    if (socket_ != INVALID_SOCKET) {
        shutdown(socket_, SD_BOTH);
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
    if (listenThread_.joinable()) {
        listenThread_.detach();
    }
}

void ChatClient::ListenLoop() {
    while (listening_) {
        std::string jsonText;
        if (!proto::RecvMessage(socket_, jsonText)) {
            if (listening_ && hWndTarget_) {
                ::PostMessage(hWndTarget_, WM_CHAT_DISCONNECTED, 0, 0);
            }
            break;
        }

        try {
            json msg = json::parse(jsonText);
            std::string type = msg.value("type", "");

            if (type == "MESSAGE_PUSH") {
                std::wstring fromUser = Utf8ToWide(msg.value("from", ""));
                if (msg.contains("n")) {
                    userFullnames_[fromUser] = Utf8ToWide(msg.value("n", ""));
                }
                if (msg.contains("a")) {
                    std::string avtB64 = msg.value("a", "");
                    if (!avtB64.empty()) {
                        userAvatars_[fromUser] = Base64Decode(avtB64);
                    }
                }
                ChatPushData* data = new ChatPushData();
                data->from = Utf8ToWide(msg.value("from", ""));
                data->content = Utf8ToWide(msg.value("content", ""));
                data->time = msg.value("time", (long long)0);
                if (!::PostMessage(hWndTarget_, WM_CHAT_MESSAGE_PUSH, 0,
                    reinterpret_cast<LPARAM>(data))) {
                    delete data;
                }
            }
            else if (type == "GETMESSAGE_RESULT") {
                auto* list = new std::vector<ChatPushData>();
                if (msg.contains("messages") && msg["messages"].is_array()) {
                    for (auto& item : msg["messages"]) {
                        ChatPushData d;
                        d.from = Utf8ToWide(item.value("from", ""));
                        d.content = Utf8ToWide(item.value("content", ""));
                        d.time = item.value("time", (long long)0);
                        list->push_back(d);
                    }
                }
                if (!::PostMessage(hWndTarget_, WM_CHAT_PENDING_LIST, 0,
                    reinterpret_cast<LPARAM>(list))) {
                    delete list;
                }
            }
            else if (type == "HISTORY_RESULT") {
                auto* hist = new ChatHistoryData();
                hist->withUser = Utf8ToWide(msg.value("with_user", ""));
                if (msg.contains("messages") && msg["messages"].is_array()) {
                    for (auto& item : msg["messages"]) {
                        ChatPushData d;
                        d.from = Utf8ToWide(item.value("from", ""));
                        d.content = Utf8ToWide(item.value("content", ""));
                        d.time = item.value("time", (long long)0);
                        hist->messages.push_back(d);
                    }
                }
                if (!::PostMessage(hWndTarget_, WM_CHAT_HISTORY_RESULT, 0,
                    reinterpret_cast<LPARAM>(hist))) {
                    delete hist;
                }
            }
            else if (type == "STATUS_PUSH") {
                std::wstring uname = Utf8ToWide(msg.value("user", ""));
                bool isOnline = msg.value("online", false);
                userStatus_[uname] = isOnline;

                // Gửi message về UI để cập nhật nếu đang chat với người này
                if (hWndTarget_) {
                    ::PostMessage(hWndTarget_, WM_CHAT_STATUS_PUSH, 0, 0);
                }
            }
        }
        catch (const std::exception&) {
            continue;
        }
    }
}

bool ChatClient::SendChatMessage(const std::wstring& to, const std::wstring& content) {
    if (socket_ == INVALID_SOCKET) return false;

    json req;
    req["type"] = "MESSAGE";
    req["to"] = WideToUtf8(to);
    req["content"] = WideToUtf8(content);

    std::lock_guard<std::mutex> lock(sendMutex_);
    return proto::SendMessage(socket_, req.dump());
}

bool ChatClient::RequestPendingMessages() {
    if (socket_ == INVALID_SOCKET) return false;

    json req;
    req["type"] = "GETMESSAGE";

    std::lock_guard<std::mutex> lock(sendMutex_);
    return proto::SendMessage(socket_, req.dump());
}

bool ChatClient::RequestChatHistory(const std::wstring& withUser) {
    if (socket_ == INVALID_SOCKET) return false;

    json req;
    req["type"] = "GET_HISTORY";
    req["with_user"] = WideToUtf8(withUser);

    std::lock_guard<std::mutex> lock(sendMutex_);
    return proto::SendMessage(socket_, req.dump());
}

bool ChatClient::SetAvatar(const std::wstring& username, const std::string& base64Str) {
    if (socket_ == INVALID_SOCKET) return false;
    json req;
    req["type"] = "SET_AVATAR";
    req["avatar"] = base64Str;

    std::lock_guard<std::mutex> lock(sendMutex_);
    bool ok = proto::SendMessage(socket_, req.dump());
    if (ok) userAvatars_[username] = Base64Decode(base64Str); 
    return ok;
}

std::vector<BYTE> ChatClient::GetAvatar(const std::wstring& username) const {
    auto it = userAvatars_.find(username);
    if (it != userAvatars_.end()) return it->second;
    return {};
}

void ChatClient::SetUserOnline(const std::wstring& username, bool online) {
    std::lock_guard<std::mutex> lock(statusMutex_);
    userStatus_[username] = online;
}

bool ChatClient::IsUserOnline(const std::wstring& username) const {
    std::lock_guard<std::mutex> lock(statusMutex_);
    auto it = userStatus_.find(username);
    if (it != userStatus_.end()) return it->second;
    return false;
}
