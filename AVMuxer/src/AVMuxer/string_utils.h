#pragma once

#include <tchar.h>
#include <inttypes.h>
#include <memory.h>
#include <vadefs.h>
#include <strsafe.h>
#include <debugapi.h>

#include <string>
#include <cwchar>
#include <cerrno>
#include <locale>
#include <memory>

#ifdef _UNICODE
namespace std {
    using tstring = wstring;
}
#else
namespace std {
    using tstring = string;
}
}
#endif // _UNICODE

class string_utils
{
    #ifdef _UNICODE
    static std::wstring ansi_to_unicode(const std::string & ansi_str)
    {
        std::size_t len = ansi_str.size();
        int unicode_len = ::MultiByteToWideChar(CP_ACP, 0, ansi_str.c_str(), -1, NULL, 0);
        wchar_t * unicode_p = new wchar_t[unicode_len + 1];
        ::memset(unicode_p, 0, (unicode_len + 1) * sizeof(wchar_t));
        ::MultiByteToWideChar(CP_ACP, 0, ansi_str.c_str(), -1, (LPWSTR)unicode_p, unicode_len);
        std::wstring str_w = unicode_p;
        delete[] unicode_p;
        return str_w;
    }
    #else
    static std::string ansi_to_unicode(const std::string & str)
    {
        return str;
    }
    #endif // _UNICODE

    #ifdef _UNICODE
    static std::string unicode_to_ansi(const std::wstring & unicode_str)
    {
        std::size_t len = unicode_str.size();
        int ansi_len = ::WideCharToMultiByte(CP_ACP, 0, unicode_str.c_str(), -1, NULL, 0, NULL, NULL);
        char * ansi_p = new char[ansi_len + 1];
        ::memset(ansi_p, 0, (ansi_len + 1) * sizeof(char));
        ::WideCharToMultiByte(CP_ACP, 0, unicode_str.c_str(), -1, (LPSTR)ansi_p, ansi_len, NULL, NULL);
        std::string str = ansi_p;
        delete[] ansi_p;
        return str;
    }
    #else
    static std::string unicode_to_ansi(const std::string & str)
    {
        return str;
    }
    #endif // _UNICODE

    static std::string ansi_to_utf8(const std::string & ansi_str)
    {
        int len = ansi_str.size();
        // From ansi transform to UTF-16LE
        int unicode_len = ::MultiByteToWideChar(CP_ACP, 0, ansi_str.c_str(), -1, NULL, 0);
        if (unicode_len == 0) {
            return "";
        }
        wchar_t * unicode_p = new wchar_t[unicode_len + 1];
        ::memset(unicode_p, 0, (unicode_len + 1) * sizeof(wchar_t));
        ::MultiByteToWideChar(CP_ACP, 0, ansi_str.c_str(), -1, (LPWSTR)unicode_p, unicode_len);

        // From UTF-16LE transform to UTF-8
        int utf8_len = WideCharToMultiByte(CP_UTF8, 0, unicode_p, -1, NULL, 0, NULL, NULL);
        if (utf8_len == 0) {
            delete[] unicode_p;
            return "";
        }

        char * utf8_p = new char[utf8_len + 1];
        ::memset(utf8_p, 0, (utf8_len + 1) * sizeof(char));
        ::WideCharToMultiByte(CP_UTF8, 0, unicode_p, -1, utf8_p, utf8_len, NULL, NULL);

        std::string str_utf8(utf8_p);
        delete[] unicode_p;
        delete[] utf8_p;

        return str_utf8;
    }

private:
    string_utils() {}
    ~string_utils() {}
};
