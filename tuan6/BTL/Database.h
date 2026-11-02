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

class Database {
public:
    Database();
    ~Database();

    bool Open(const std::wstring& dbPath);
    void Close();

    bool RegisterUser(const std::string& username, const std::string& passwordHashHex, const std::string& saltHex);

    bool VerifyLogin(const std::string& username, const std::string& passwordPlain, int& outUserId);

    bool UserExists(const std::string& username);

    bool SaveMessage(const std::string& from, const std::string& to, const std::string& content, long long timestamp, bool delivered);
    void MarkDelivered(int messageId);

    std::vector<ChatMessage> GetPendingMessages(const std::string& toUser);

private:
    sqlite3* db_ = nullptr;
    std::recursive_mutex mutex_; 

    bool ExecSql(const char* sql);
};
