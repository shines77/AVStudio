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

class string_utils_cxx11
{
    #ifdef _UNICODE
    static std::wstring ansi_to_unicode(const std::string &str) {
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
    #else
    static std::string ansi_to_unicode(const std::string & str) {
        return str;
    }
    #endif // _UNICODE

    #ifdef _UNICODE
    static std::string unicode_to_ansi(const std::wstring &wstr) {
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
    #else
    static std::string unicode_to_ansi(const std::string & str) {
        return str;
    }
    #endif // _UNICODE

    static std::string ansi_to_utf8(const std::string & ansi_str) {
        std::wstring unicode_str = ansi_to_unicode(ansi_str);
        std::wstring utf8_str = unicode_to_utf8(unicode_str);
        return utf8_str;
    }

    static std::string untf8_to_ansi(const std::string & utf8_str) {
        std::wstring unicode_str = utf8_to_unicode(utf8_str);
        std::wstring ansi_str = unicode_to_ansi(unicode_str);
        return ansi_str;
    }

private:
    string_utils_cxx11() {}
    ~string_utils_cxx11() {}
};
