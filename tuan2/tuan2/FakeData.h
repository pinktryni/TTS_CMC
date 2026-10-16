#pragma once
#include <map>

struct FakeData {
    static std::map<CString, CString>& Users() {
        static std::map<CString, CString> u = {
            { L"admin", L"123456" },
            { L"an",    L"an123" }
        };
        return u;
    }
};
