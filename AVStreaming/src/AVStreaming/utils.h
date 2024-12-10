
#include <string>

enum log_level {
    LOG_FATAL,
    LOG_ERROR,
    LOG_WARNING,
    LOG_INFO,
    LOG_VERBOSE,
    LOG_DEBUG,
    LOG_MAXIMUM
};

#ifdef _UNICODE
#define debug_prin_t    debug_print_w
#define log_print_t     log_print_w
#else
#define debug_prin_t    debug_print
#define log_print_t     log_print
#endif

void set_log_level(int level);

void debug_print(const char * fmt, ...);
void debug_print_w(const wchar_t * fmt, ...);

void log_print(int level, const char * fmt, ...);
void log_print_w(int level, const wchar_t * fmt, ...);

extern int s_log_level;

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

#ifdef _UNICODE
std::wstring Ansi2Unicode(const std::string & ansiStr);
std::string Unicode2Ansi(const std::wstring & unicodeStr);
#else
std::string Ansi2Unicode(const std::string & str)
std::string Unicode2Ansi(const std::string & str);
#endif // _UNICODE

std::string Ansi2Utf8(const std::string & ansiStr);
