#pragma once
#include "sqlite3.h"
#include <string>
#include <vector>
#include <mutex>

struct ChatMessage {
    int id;
    std::string fromUser;
    std::string toUser;
    std::string content;
    long long timestamp; 
    bool delivered;
};

struct UserRecord {
    std::string username;
    std::string fullname;
    std::string avatar;
};


class Database {
public:
    Database();
    ~Database();

    bool Open(const std::wstring& dbPath);
    void Close();

    bool RegisterUser(const std::string& username, const std::string& fullname, const std::string& passwordHashHex, const std::string& saltHex, const std::string& avatarBase64);
    bool VerifyLogin(const std::string& username, const std::string& passwordPlain, int& outUserId);

    bool UserExists(const std::string& username);

    bool UpdateAvatar(const std::string& username, const std::string& avatarBase64);

    bool SaveMessage(const std::string& from, const std::string& to, const std::string& content, long long timestamp, bool delivered);
    void MarkDelivered(int messageId);
    std::vector<UserRecord> GetAllUsers();
    std::vector<ChatMessage> GetPendingMessages(const std::string& toUser);
    std::vector<ChatMessage> GetChatHistory(const std::string& user1, const std::string& user2);

private:
    sqlite3* db_ = nullptr;
    std::recursive_mutex mutex_; 

    bool ExecSql(const char* sql);
};
