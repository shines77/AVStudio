#pragma once

#include <stdint.h>

#include <cstdint>
#include <string>
#include <cwchar>   // For std::mbsrtowcs(), std::wcsrtombs(), ...
#include <locale>   // For std::set_locale()
#include <memory>   // For std::unique_ptr<T>

class string_utils_cxx
{
public:
    static const std::size_t kInvalidValue = static_cast<std::size_t>(-1);

    static std::wstring ansi_to_unicode(const std::string & ansi_str)
    {
        std::wstring ret;
        // Must be setting locale to system default local.
        // The value is "English_United States.1252" on Windows,
        // and the value is "en_US.UTF-8" or "zh_CN.GBK" on Linux.
        const char * orig_locale = std::setlocale(LC_ALL, "");

        std::mbstate_t state = {};
        const char * src = ansi_str.data();
        size_t len = std::mbsrtowcs(nullptr, &src, 0, &state);
        if (len != kInvalidValue) {
            std::unique_ptr<wchar_t[]> buff(new wchar_t[len + 1]);
            len = std::mbsrtowcs(buff.get(), &src, len, &state);
            if (len != kInvalidValue) {
                ret.assign(buff.get(), len);
            }
        }

        // Restore the original locale
        std::setlocale(LC_ALL, orig_locale);
        return ret;
    }

#ifdef _UNICODE
    static std::wstring ansi_to_unicode_t(const std::string & ansi_str) {
        return ansi_to_unicode(str);
    }
#else
    static std::string ansi_to_unicode_t(const std::string & ansi_str) {
        return str;
    }
#endif

    static std::string unicode_to_ansi(const std::wstring & wstr)
    {
        std::string ret;
        // Must be setting locale to system default local.
        // The value is "English_United States.1252" on Windows,
        // and the value is "en_US.UTF-8" or "zh_CN.GBK" on Linux.
        const char * orig_locale = std::setlocale(LC_ALL, "");

        std::mbstate_t state = {};
        const wchar_t * src = wstr.data();
        size_t len = std::wcsrtombs(nullptr, &src, 0, &state);
        if (len != kInvalidValue) {
            std::unique_ptr<char[]> buff(new char[len + 1]);
            len = std::wcsrtombs(buff.get(), &src, len, &state);
            if (len != kInvalidValue) {
                ret.assign(buff.get(), len);
            }
        }

        // Restore the original locale
        std::setlocale(LC_ALL, orig_locale);
        return ret;
    }

#ifdef _UNICODE
    static std::string unicode_to_ansi_t(const std::wstring & wstr) {
        return unicode_to_ansi(wstr);
    }
#else
    static std::string unicode_to_ansi_t(const std::string & wstr) {
        return str;
    }
#endif

    static std::wstring utf8_to_unicode(const std::string & utf8_str)
    {
        std::wstring ret;
        // Must be setting locale to UTF-8 first
        const char * orig_locale = std::setlocale(LC_ALL, "en_US.UTF-8");

        std::mbstate_t state = {};
        const char * src = utf8_str.data();
        size_t len = std::mbsrtowcs(nullptr, &src, 0, &state);
        if (len != kInvalidValue) {
            std::unique_ptr<wchar_t[]> buff(new wchar_t[len + 1]);
            len = std::mbsrtowcs(buff.get(), &src, len, &state);
            if (len != kInvalidValue) {
                ret.assign(buff.get(), len);
            }
        }

        // Restore the original locale
        std::setlocale(LC_ALL, orig_locale);
        return ret;
    }

    static std::string unicode_to_utf8(const std::wstring & wstr)
    {
        std::string ret;
        // Must be setting locale to UTF-8 first
        const char * orig_locale = std::setlocale(LC_ALL, "en_US.UTF-8");

        std::mbstate_t state = {};
        const wchar_t * src = wstr.data();
        size_t len = std::wcsrtombs(nullptr, &src, 0, &state);
        if (len != kInvalidValue) {
            std::unique_ptr<char[]> buff(new char[len + 1]);
            len = std::wcsrtombs(buff.get(), &src, len, &state);
            if (len != kInvalidValue) {
                ret.assign(buff.get(), len);
            }
        }

        // Restore the original locale
        std::setlocale(LC_ALL, orig_locale);
        return ret;
    }

    static std::string ansi_to_utf8(const std::string & ansi_str)
    {
        std::wstring unicode_str = ansi_to_unicode(ansi_str);
        std::string utf8_str = unicode_to_utf8(unicode_str);
        return utf8_str;
    }

    static std::string untf8_to_ansi(const std::string & utf8_str)
    {
        std::wstring unicode_str = utf8_to_unicode(utf8_str);
        std::string ansi_str = unicode_to_ansi(unicode_str);
        return ansi_str;
    }

private:
    string_utils_cxx() {}
    ~string_utils_cxx() {}
};
