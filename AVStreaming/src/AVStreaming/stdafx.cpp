
// stdafx.cpp : 只包括标准包含文件的源文件
// AVStreaming.pch 将作为预编译头
// stdafx.obj 将包含预编译类型信息

#include "stdafx.h"

#include <tchar.h>
#include <memory.h>

#include <string>
#include <cwchar>
#include <cerrno>
#include <locale>
#include <memory>

#ifdef _UNICODE
std::wstring Ansi2Unicode(const std::string & ansiStr)
{
    int len = ansiStr.size();
    int unicode_len = MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, NULL, 0);
    wchar_t * unicode_p = new wchar_t[unicode_len + 1];
    ::memset(unicode_p, 0, (unicode_len + 1) * sizeof(wchar_t));
    MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, (LPWSTR)unicode_p, unicode_len);
    std::wstring str_w = unicode_p;
    delete[] unicode_p;
    return str_w;
}
#else
std::string Ansi2Unicode(const std::string & str)
{
    return str;
}
#endif // _UNICODE

#ifdef _UNICODE
std::string Unicode2Ansi(const std::wstring & unicodeStr)
{
    int len = unicodeStr.size();
    int ansi_len = WideCharToMultiByte(CP_ACP, 0, unicodeStr.c_str(), -1, NULL, 0, NULL, NULL);
    char * ansi_p = new char[ansi_len + 1];
    ::memset(ansi_p, 0, (ansi_len + 1) * sizeof(char));
    WideCharToMultiByte(CP_ACP, 0, unicodeStr.c_str(), -1, (LPSTR)ansi_p, ansi_len, NULL, NULL);
    std::string str = ansi_p;
    delete[] ansi_p;
    return str;
}
#else
std::string Unicode2Ansi(const std::string & str)
{
    return str;
}
#endif // _UNICODE

// C++11: Unicode 转 ANSI
std::string UnicodeToAnsi_cxx11(const std::wstring &wstr) {
    std::string ret;
    std::mbstate_t state = {};
    const wchar_t * src = wstr.data();
    size_t len = std::wcsrtombs(nullptr, &src, 0, &state);
    if (static_cast<size_t>(-1) != len) {
        std::unique_ptr<char[]> buff(new char[len + 1]);
        len = std::wcsrtombs(buff.get(), &src, len, &state);
        if (static_cast<size_t>(-1) != len) {
            ret.assign(buff.get(), len);
        }
    }
    return ret;
}

// C++11: ANSI 转 Unicode
std::wstring AnsiToUnicode_cxx1(const std::string &str) {
    std::wstring ret;
    std::mbstate_t state = {};
    const char * src = str.data();
    size_t len = std::mbsrtowcs(nullptr, &src, 0, &state);
    if (static_cast<size_t>(-1) != len) {
        std::unique_ptr<wchar_t[]> buff(new wchar_t[len + 1]);
        len = std::mbsrtowcs(buff.get(), &src, len, &state);
        if (static_cast<size_t>(-1) != len) {
            ret.assign(buff.get(), len);
        }
    }
    return ret;
}
