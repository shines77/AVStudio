
#include "stdafx.h"
#include "utils.h"

#include <tchar.h>
#include <memory.h>
#include <vadefs.h>
#include <strsafe.h>
#include <debugapi.h>

#include <string>
#include <cwchar>
#include <cerrno>
#include <locale>
#include <memory>

int s_log_level = LOG_INFO;

const char * get_level_nameA(int level)
{
    switch (level) {
        case LOG_FATAL:
            return "Fatal";
        case LOG_ERROR:
            return "Error";
        case LOG_WARNING:
            return "Warning";
        case LOG_INFO:
            return "Info";
        case LOG_VERBOSE:
            return "Verbose";
        case LOG_DEBUG:
            return "Debug";
        default:
            return "Unknown";
    }
}

const wchar_t * get_level_nameW(int level)
{
    switch (level) {
        case LOG_FATAL:
            return L"Fatal";
        case LOG_ERROR:
            return L"Error";
        case LOG_WARNING:
            return L"Warning";
        case LOG_INFO:
            return L"Info";
        case LOG_VERBOSE:
            return L"Verbose";
        case LOG_DEBUG:
            return L"Debug";
        default:
            return L"Unknown";
    }
}

void set_log_level(int level)
{
    s_log_level = level;
}

void debug_printv(const char * fmt, va_list args)
{
    static const size_t kBufSize = 2048;
    static char szBuffer[kBufSize] = { 0 };

    // Use a bounded buffer size to prevent buffer overruns. Limit count to
    // character size minus one to allow for a NULL terminating character.
    HRESULT hr = StringCchVPrintfA(szBuffer, kBufSize - 1, fmt, args);

    // Ensure that the formatted string is NULL-terminated
    szBuffer[kBufSize - 1] = '\0';

    ::OutputDebugStringA(szBuffer);
}

void debug_print(const char * fmt, ...)
{
    // Format the input string
    va_list args;
    va_start(args, fmt);

    debug_printv(fmt, args);
    va_end(args);
}

void debug_printv_w(const wchar_t * fmt, va_list args)
{
    static const size_t kBufSize = 2048;
    static wchar_t szBuffer[kBufSize] = { 0 };

    // Use a bounded buffer size to prevent buffer overruns. Limit count to
    // character size minus one to allow for a NULL terminating character.
    HRESULT hr = StringCchVPrintfW(szBuffer, kBufSize - 1, fmt, args);

    // Ensure that the formatted string is NULL-terminated
    szBuffer[kBufSize - 1] = '\0';

    ::OutputDebugStringW(szBuffer);
}

void debug_print_w(const wchar_t * fmt, ...)
{
    // Format the input string
    va_list args;
    va_start(args, fmt);

    debug_printv_w(fmt, args);
    va_end(args);
}

void log_print(int level, const char * fmt, ...)
{
    if (level >= s_log_level) {
        debug_print("[%s]: ", get_level_nameA(level));

        // Format the input string
        va_list args;
        va_start(args, fmt);

        debug_printv(fmt, args);
        va_end(args);
    }
}

void log_print_w(int level, const wchar_t * fmt, ...)
{
    if (level >= s_log_level) {
        debug_print_w(L"[%s]: ", get_level_nameW(level));

        // Format the input string
        va_list args;
        va_start(args, fmt);

        debug_printv_w(fmt, args);
        va_end(args);
    }
}

#ifdef _UNICODE
std::wstring Ansi2Unicode(const std::string & ansiStr)
{
    int len = ansiStr.size();
    int unicode_len = ::MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, NULL, 0);
    wchar_t * unicode_p = new wchar_t[unicode_len + 1];
    ::memset(unicode_p, 0, (unicode_len + 1) * sizeof(wchar_t));
    ::MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, (LPWSTR)unicode_p, unicode_len);
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
    int ansi_len = ::WideCharToMultiByte(CP_ACP, 0, unicodeStr.c_str(), -1, NULL, 0, NULL, NULL);
    char * ansi_p = new char[ansi_len + 1];
    ::memset(ansi_p, 0, (ansi_len + 1) * sizeof(char));
    ::WideCharToMultiByte(CP_ACP, 0, unicodeStr.c_str(), -1, (LPSTR)ansi_p, ansi_len, NULL, NULL);
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

std::string Ansi2Utf8(const std::string & ansiStr)
{
    int len = ansiStr.size();
    // From ansi transform to UTF-16LE
    int unicode_len = ::MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, NULL, 0);
    if (unicode_len == 0) {
        return "";
    }
    wchar_t * unicode_p = new wchar_t[unicode_len + 1];
    ::memset(unicode_p, 0, (unicode_len + 1) * sizeof(wchar_t));
    ::MultiByteToWideChar(CP_ACP, 0, ansiStr.c_str(), -1, (LPWSTR)unicode_p, unicode_len);

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

// C++11: Unicode ת ANSI
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

// C++11: ANSI ת Unicode
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
