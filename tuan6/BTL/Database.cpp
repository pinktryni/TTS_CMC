#include "Database.h"
#include "PasswordHash.h"
#include <codecvt>
#include <locale>

Database::Database() {}
Database::~Database() { Close(); }

static std::string WideToUtf8(const std::wstring& w) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> conv;
    return conv.to_bytes(w);
}

bool Database::Open(const std::wstring& dbPath) {
    std::string pathUtf8 = WideToUtf8(dbPath);
    if (sqlite3_open(pathUtf8.c_str(), &db_) != SQLITE_OK)
        return false;

    const char* createUsers =
        "CREATE TABLE IF NOT EXISTS users ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  username TEXT UNIQUE NOT NULL,"
        "  password_hash TEXT NOT NULL,"
        "  salt TEXT NOT NULL,"
        "  created_at INTEGER NOT NULL"
        ");";

    const char* createMessages =
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  from_user TEXT NOT NULL,"
        "  to_user TEXT NOT NULL,"
        "  content TEXT NOT NULL,"
        "  timestamp INTEGER NOT NULL,"
        "  delivered INTEGER NOT NULL DEFAULT 0"
        ");";

    return ExecSql(createUsers) && ExecSql(createMessages);
}

void Database::Close() {
    if (db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool Database::ExecSql(const char* sql) {
    char* errMsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errMsg);
    if (rc != SQLITE_OK) {
        sqlite3_free(errMsg);
        return false;
    }
    return true;
}

bool Database::UserExists(const std::string& username) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id FROM users WHERE username = ?;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    sqlite3_finalize(stmt);
    return exists;
}

bool Database::RegisterUser(const std::string& username, const std::string& passwordHashHex, const std::string& saltHex) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    if (UserExists(username)) return false;
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO users (username, password_hash, salt, created_at) VALUES (?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, passwordHashHex.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, saltHex.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, (sqlite3_int64)time(nullptr));

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::VerifyLogin(const std::string& username, const std::string& passwordPlain, int& outUserId) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT id, password_hash, salt FROM users WHERE username = ?;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, username.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int id = sqlite3_column_int(stmt, 0);
        std::string storedHash = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        std::string storedSalt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));

        if (pwdhash::VerifyPassword(passwordPlain, storedHash, storedSalt)) {
            outUserId = id;
            ok = true;
        }
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool Database::SaveMessage(const std::string& from, const std::string& to, const std::string& content, long long timestamp, bool delivered) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "INSERT INTO messages (from_user, to_user, content, timestamp, delivered) VALUES (?, ?, ?, ?, ?);";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, from.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, to.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 4, timestamp);
    sqlite3_bind_int(stmt, 5, delivered ? 1 : 0);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<ChatMessage> Database::GetPendingMessages(const std::string& toUser) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    std::vector<ChatMessage> result;

    sqlite3_stmt* stmt = nullptr; 
    const char* sql = "SELECT id, from_user, to_user, content, timestamp, delivered "
                       "FROM messages WHERE to_user = ? AND delivered = 0 ORDER BY timestamp ASC;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return result;

    sqlite3_bind_text(stmt, 1, toUser.c_str(), -1, SQLITE_TRANSIENT);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ChatMessage m;
        m.id = sqlite3_column_int(stmt, 0);
        m.fromUser = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        m.toUser = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        m.content = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        m.timestamp = sqlite3_column_int64(stmt, 4);
        m.delivered = sqlite3_column_int(stmt, 5) != 0;
        result.push_back(m);
    }
    sqlite3_finalize(stmt);
    return result;
}

void Database::MarkDelivered(int messageId) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    sqlite3_stmt* stmt = nullptr;
    const char* sql = "UPDATE messages SET delivered = 1 WHERE id = ?;";
    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return;
    sqlite3_bind_int(stmt, 1, messageId);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}
