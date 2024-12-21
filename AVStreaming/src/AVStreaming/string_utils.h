#pragma once

#include <stdint.h>
#include <tchar.h>
#include <memory.h>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>    // For MultiByteToWideChar(), WideCharToMultiByte(), and so on.

#include <string>

#ifdef _UNICODE
namespace std {
    using tstring = wstring;
}
#else
namespace std {
    using tstring = string;
}
#endif // _UNICODE

class string_utils
{
public:
    static std::wstring ansi_to_unicode(const std::string & ansi_str)
    {
        std::size_t ansi_len = ansi_str.size();
        int unicode_len = ::MultiByteToWideChar(CP_ACP, 0, ansi_str.c_str(), -1, NULL, 0);
        wchar_t * unicode_p = new wchar_t[unicode_len + 1];
        ::memset(unicode_p, 0, (unicode_len + 1) * sizeof(wchar_t));
        ::MultiByteToWideChar(CP_ACP, 0, ansi_str.c_str(), -1, (LPWSTR)unicode_p, unicode_len);
        std::wstring str_unicode(unicode_p);
        delete[] unicode_p;
        return str_unicode;
    }

#ifdef _UNICODE
    static std::wstring ansi_to_unicode_t(const std::string & str) {
        return ansi_to_unicode(str);
    }
#else
    static std::string ansi_to_unicode_t(const std::string & str) {
        return str;
    }
#endif

    static std::string unicode_to_ansi(const std::wstring & unicode_str)
    {
        std::size_t unicode_len = unicode_str.size();
        int ansi_len = ::WideCharToMultiByte(CP_ACP, 0, unicode_str.c_str(), -1, NULL, 0, NULL, NULL);
        char * ansi_p = new char[ansi_len + 1];
        ::memset(ansi_p, 0, (ansi_len + 1) * sizeof(char));
        ::WideCharToMultiByte(CP_ACP, 0, unicode_str.c_str(), -1, (LPSTR)ansi_p, ansi_len, NULL, NULL);
        std::string str_ansi(ansi_p);
        delete[] ansi_p;
        return str_ansi;
    }

#ifdef _UNICODE
    static std::string unicode_to_ansi_t(const std::wstring & wstr) {
        return unicode_to_ansi(wstr);
    }
#else
    static std::string unicode_to_ansi_t(const std::string & str) {
        return str;
    }
#endif

    static std::wstring utf8_to_unicode(const std::string & utf8_str)
    {
        std::size_t utf8_len = utf8_str.size();
        int unicode_len = ::MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, NULL, 0);
        wchar_t * unicode_p = new wchar_t[unicode_len + 1];
        ::memset(unicode_p, 0, (unicode_len + 1) * sizeof(wchar_t));
        ::MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, (LPWSTR)unicode_p, unicode_len);
        std::wstring str_unicode(unicode_p);
        delete[] unicode_p;
        return str_unicode;
    }

    static std::string unicode_to_utf8(const std::wstring & unicode_str)
    {
        std::size_t unicode_len = unicode_str.size();
        int utf8_len = ::WideCharToMultiByte(CP_UTF8, 0, unicode_str.c_str(), -1, NULL, 0, NULL, NULL);
        char * utf8_p = new char[utf8_len + 1];
        ::memset(utf8_p, 0, (utf8_len + 1) * sizeof(char));
        ::WideCharToMultiByte(CP_UTF8, 0, unicode_str.c_str(), -1, (LPSTR)utf8_p, utf8_len, NULL, NULL);
        std::string str_utf8(utf8_p);
        delete[] utf8_p;
        return str_utf8;
    }

    static std::string ansi_to_utf8(const std::string & ansi_str)
    {
        std::size_t ansi_len = ansi_str.size();
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

    static std::string utf8_to_ansi(const std::string & utf8_str)
    {
        std::size_t len = utf8_str.size();
        // From utf8 transform to UTF-16LE
        int unicode_len = ::MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, NULL, 0);
        if (unicode_len == 0) {
            return "";
        }
        wchar_t * unicode_p = new wchar_t[unicode_len + 1];
        ::memset(unicode_p, 0, (unicode_len + 1) * sizeof(wchar_t));
        ::MultiByteToWideChar(CP_ACP, 0, utf8_str.c_str(), -1, (LPWSTR)unicode_p, unicode_len);

        // From UTF-16LE transform to ansi
        int ansi_len = WideCharToMultiByte(CP_ACP, 0, unicode_p, -1, NULL, 0, NULL, NULL);
        if (ansi_len == 0) {
            delete[] unicode_p;
            return "";
        }

        char * ansi_p = new char[ansi_len + 1];
        ::memset(ansi_p, 0, (ansi_len + 1) * sizeof(char));
        ::WideCharToMultiByte(CP_ACP, 0, unicode_p, -1, ansi_p, ansi_len, NULL, NULL);

        std::string str_ansi(ansi_p);
        delete[] unicode_p;
        delete[] ansi_p;

        return str_ansi;
    }

    static int unicode_to_tchar(TCHAR * dest, size_t size, const wchar_t * src)
    {
#ifdef _UNICODE
        errno_t err = ::wcscpy_s(dest, size - 1, src);
        return (int)err;
#else
        int out_len = ::WideCharToMultiByte(CP_ACP, 0, src, -1, dest, size, "", NULL);
        return out_len;
#endif
    }

private:
    string_utils() {}
    ~string_utils() {}
};
