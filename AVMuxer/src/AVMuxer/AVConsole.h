
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>
#include <string>

struct LogLevel
{
    enum {
        Fatal,
        Error,
        Warning,
        Info,
        Verbose,
        Debug,
        Maximum
    };
};

template <typename Writer, typename CharType = char>
class AVConsoleBase
{
public:
    typedef CharType    char_type;
    typedef Writer      writer_t;

    static const bool kIsWideChar = (sizeof(CharType) > 1);

    AVConsoleBase() : level_(LogLevel::Info) {
    }

    AVConsoleBase(const std::string & filename)
        : level_(LogLevel::Info),
          writer_(filename) {
        //
    }
    ~AVConsoleBase() {
        //
    }

    int getLogLevel() const {
        return level_;
    }

    void setLogLevel(int level) {
        level_ = level;
    }

    void fatal(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        logPrint(LogLevel::Fatal, fmt, args);
        va_end(args);
    }

    void error(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        logPrint(LogLevel::Error, fmt, args);
        va_end(args);
    }

    void warning(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        logPrint(LogLevel::Warning, fmt, args);
        va_end(args);
    }

    void info(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        logPrint(LogLevel::Info, fmt, args);
        va_end(args);
    }

    void verbose(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        logPrint(LogLevel::Info, fmt, args);
        va_end(args);
    }

    void debug(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        logPrint(LogLevel::Info, fmt, args);
        va_end(args);
    }

    void print(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        debugPrintv(fmt, args);
        va_end(args);
    }

    void println(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        debugPrintv(fmt, args);
#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS)
        debugPrint("\r\n");
#else
        debugPrint("\n");
#endif
        va_end(args);
    }

protected:
    const char * getLevelName(int level) {
        switch (level) {
            case LogLevel::Fatal:
                return "Fatal";
            case LogLevel::Error:
                return "Error";
            case LogLevel::Warning:
                return "Warning";
            case LogLevel::Info:
                return "Info";
            case LogLevel::Verbose:
                return "Verbose";
            case LogLevel::Debug:
                return "Debug";
            default:
                return "Unknown";
        }
    }

    const wchar_t * getLevelNameW(int level) {
        switch (level) {
            case LogLevel::Fatal:
                return L"Fatal";
            case LogLevel::Error:
                return L"Error";
            case LogLevel::Warning:
                return L"Warning";
            case LogLevel::Info::
                return L"Info";
            case LogLevel::Verbose:
                return L"Verbose";
            case LogLevel::Debug:
                return L"Debug";
            default:
                return L"Unknown";
        }
    }

    void debugPrintv(const char_type * fmt, va_list args)
    {
        static const size_t kBufSize = 2048;
        static char_type szBuffer[kBufSize] = { 0 };

        //const char_type * buffer   = (const char_type *)&szBuffer[0];
        const char *      buffer_a = (const char *)&szBuffer[0];
        const wchar_t *   buffer_w = (const wchar_t *)&szBuffer[0];

#ifdef _MSC_VER
        // Use a bounded buffer size to prevent buffer overruns. Limit count to
        // character size minus one to allow for a NULL terminating character.
        HRESULT hr;
        if (kIsWideChar)
            hr = StringCchVPrintfW((STRSAFE_LPWSTR)buffer_w, kBufSize - 1, (const wchar_t *)fmt, args);
        else
            hr = StringCchVPrintfA((STRSAFE_LPSTR)buffer_a, kBufSize - 1, (const char *)fmt, args);
#else
        int ret = vsnprintf(szBuffer, kBufSize - 1, fmt, args);
        if (ret < 0) {
            printf("vsnprintf() error: %d\n", errno);
        }
#endif
        // Ensure that the formatted string is NULL-terminated
        szBuffer[kBufSize - 1] = '\0';

        writer_.print(szBuffer);
    }

    void debugPrint(const char_type * fmt, ...)
    {
        // Format the input string
        va_list args;
        va_start(args, fmt);

        debugPrintv(fmt, args);
        va_end(args);
    }

    void logPrintv(int level, const char_type * fmt, va_list args)
    {
        if (level >= level_) {
            if (kIsWideChar)
                debugPrint((const char_type *)"[%s]: ", (const char_type *)getLevelName(level));
            else
                debugPrint((const char_type *)L"[%s]: ", (const char_type *)getLevelNameW(level));

            debugPrintv(fmt, args);
        }
    }

    void logPrint(int level, const char_type * fmt, ...)
    {
        if (level >= level_) {
            if (kIsWideChar)
                debugPrint((const char_type *)"[%s]: ", (const char_type *)getLevelName(level));
            else
                debugPrint((const char_type *)L"[%s]: ", (const char_type *)getLevelNameW(level));

            va_list args;
            va_start(args, fmt);

            debugPrintv(fmt, args);
            va_end(args);
        }
    }

protected:
    int         level_;
    writer_t    writer_;
};

struct ConsoleWriter
{
    ConsoleWriter() {
        //
    }
    ConsoleWriter(std::string filename) {
        //
    }
    ~ConsoleWriter() {
        //
    }

    void print(const char * text) {
        ::printf(text);
    }
};

using AVConsole = AVConsoleBase<ConsoleWriter, char>;
