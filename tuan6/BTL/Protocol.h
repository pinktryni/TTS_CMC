#pragma once
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <string>
#include <vector>
#include <stdexcept>

#pragma comment(lib, "Ws2_32.lib")

namespace proto {
    inline bool SendAll(SOCKET sock, const char* data, int len) {
        int sent = 0;
        while (sent < len) {
            int n = send(sock, data + sent, len - sent, 0);
            if (n == SOCKET_ERROR || n == 0) return false;
            sent += n;
        }
        return true;
    }
    inline bool RecvAll(SOCKET sock, char* data, int len) {
        int recvd = 0;
        while (recvd < len) {
            int n = recv(sock, data + recvd, len - recvd, 0);
            if (n == SOCKET_ERROR || n == 0) return false; 
            recvd += n;
        }
        return true;
    }

    inline bool SendMessage(SOCKET sock, const std::string& jsonText) {
        uint32_t len = static_cast<uint32_t>(jsonText.size());
        uint32_t netLen = htonl(len);
        if (!SendAll(sock, reinterpret_cast<const char*>(&netLen), sizeof(netLen)))
            return false;
        if (len == 0) return true;
        return SendAll(sock, jsonText.data(), static_cast<int>(len));
    }

    inline bool RecvMessage(SOCKET sock, std::string& outJson, uint32_t maxLen = 1 * 1024 * 1024) {
        uint32_t netLen = 0;
        if (!RecvAll(sock, reinterpret_cast<char*>(&netLen), sizeof(netLen)))
            return false;
        uint32_t len = ntohl(netLen);
        if (len > maxLen) return false; 
        if (len == 0) {
            outJson.clear();
            return true;
        }
        std::vector<char> buf(len);
        if (!RecvAll(sock, buf.data(), static_cast<int>(len)))
            return false;
        outJson.assign(buf.data(), len);
        return true;
    }
}
