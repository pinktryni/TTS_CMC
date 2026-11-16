#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <bcrypt.h>
#include "md5.h"
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

#pragma comment(lib, "Bcrypt.lib")

namespace pwdhash {
    inline std::string BytesToHex(const std::vector<BYTE>& data) {
        std::ostringstream oss;
        for (BYTE b : data)
            oss << std::hex 
                << std::setw(2) 
                << std::setfill('0') 
                << (int)b;
        return oss.str();
    }
    inline std::vector<BYTE> HexToBytes(const std::string& hex) {
        std::vector<BYTE> out;
        for (size_t i = 0; i + 1 < hex.size(); i += 2) {
            BYTE b = (BYTE)strtol(hex.substr(i, 2).c_str(), nullptr, 16);
            out.push_back(b);
        }
        return out;
    }
    inline std::vector<BYTE> GenerateSalt(ULONG lenBytes = 16) {
        std::vector<BYTE> salt(lenBytes);
        BCryptGenRandom(nullptr, salt.data(), lenBytes, BCRYPT_USE_SYSTEM_PREFERRED_RNG);
        return salt;
    }
    inline std::string HashPassword(const std::string& password, const std::vector<BYTE>& salt) {
        MD5 md5;
        md5.add(salt.data(), salt.size());
        md5.add(password.data(), password.size());
        return md5.getHash(); 
    }
    inline void HashNewPassword(const std::string& password, std::string& outHashHex, std::string& outSaltHex) {
        auto salt = GenerateSalt();
        outHashHex = HashPassword(password, salt);
        outSaltHex = BytesToHex(salt);
    }
    inline bool VerifyPassword(const std::string& password, const std::string& storedHashHex, const std::string& storedSaltHex) {
        auto salt = HexToBytes(storedSaltHex);
        std::string computed = HashPassword(password, salt);
        return computed == storedHashHex;
    }
}