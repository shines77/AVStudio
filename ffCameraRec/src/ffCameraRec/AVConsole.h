
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>

#include <cstdio>
#include <iostream>
#include <fstream>
#include <string>
#include <algorithm>
#include <cctype>

namespace av {

struct LogLevel
{
    enum {
        Fatal,
        Error,
        Warning,
        Info,
        Verbose,
        Debug,      // output only in debug mode
        Trace,      // output always
        Maximum
    };
};

struct FileStatus {
    enum {
        Failed = -1,
        Unknown,
        Succeeded,
        Opened
    };
};

struct FileState {
    enum {
        Unknown,
        Openning,
        Closed
    };
};

void str_to_lower(std::string & str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::tolower(c); });
}

void str_to_upper(std::string & str)
{
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c) { return std::toupper(c); });
}

template <typename Writer, typename CharType = char>
class ConsoleBase
{
public:
    typedef CharType                        char_type;
    typedef Writer                          writer_t;
    typedef ConsoleBase<Writer, CharType>   this_type;

    static const bool kIsWideChar = (sizeof(CharType) > 1);

    ConsoleBase() : level_(LogLevel::Info), fileState_(FileStatus::Unknown) {
    }

    ConsoleBase(const std::string & filename, bool onlyAppend = true)
        : level_(LogLevel::Info), fileState_(FileStatus::Unknown) {
        open(filename, onlyAppend);
    }
    ~ConsoleBase() {
        flush();
        close();
    }

    int open(const std::string & filename, bool onlyAppend = true) {
        int state = FileState::Unknown;
        if (filename.empty()) {
            return state;
        }
        std::string filename_tmp(filename);
        str_to_lower(filename_tmp);
        if (filename_tmp != filename_) {
            flush();
            close();
        }
        if (fileState_ != FileState::Openning) {
            int status = writer_.open(filename, onlyAppend);
            fileState_ = (status == FileStatus::Succeeded) ? FileState::Openning : FileState::Unknown;
            filename_ = filename;
            str_to_lower(filename_);
        }
        return state;
    }

    int close() {
        int state = FileState::Unknown;
        if (fileState_ == FileState::Openning) {
            writer_.flush();
            writer_.close();
            fileState_ = FileState::Closed;
        }
        return state;
    }

    void flush() {
        if (fileState_ == FileState::Openning) {
            writer_.flush();
        }
    }

    int getLogLevel() const {
        return level_;
    }

    void setLogLevel(int level) {
        if (level >= LogLevel::Fatal && level < LogLevel::Maximum) {
            level_ = level;
        }
    }

    void fatal(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_print_args(LogLevel::Fatal, fmt, args);
        va_end(args);
    }

    void error(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_print_args(LogLevel::Error, fmt, args);
        va_end(args);
    }

    void warning(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_print_args(LogLevel::Warning, fmt, args);
        va_end(args);
    }

    void info(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        log_print_args(LogLevel::Info, fmt, args);
        va_end(args);
    }

    void verbose(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_print_args(LogLevel::Verbose, fmt, args);
        va_end(args);
    }

    void debug(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_print_args(LogLevel::Debug, fmt, args);
        va_end(args);
    }

    void trace(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_print_args(LogLevel::Trace, fmt, args);
        va_end(args);
    }

    bool can_output(int level) const {
        return (level != LogLevel::Trace) ? (level >= level_) : true;
    }

    bool try_fatal() {
        return try_log_write(LogLevel::Fatal);
    }

    bool try_error() {
        return try_log_write(LogLevel::Error);
    }

    bool try_warning() {
        return try_log_write(LogLevel::Warning);
    }

    bool try_info() {
        return try_log_write(LogLevel::Info);
    }

    bool try_verbose() {
        return try_log_write(LogLevel::Verbose);
    }

    bool try_debug() {
        return try_log_write(LogLevel::Debug);
    }

    bool try_trace() {
        return try_log_write(LogLevel::Trace);
    }

    void new_line() {
        writer_.new_line();
    }

    void print(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        output_args(fmt, args);
        va_end(args);
    }

    void println(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        output_args(fmt, args);
        va_end(args);

        new_line();
    }

    template <typename T>
    friend this_type & operator << (this_type & console, const T & data) {
        console.writer_ << data;
        return console;
    }

    template <typename T>
    friend this_type & operator << (this_type & console, T && data) {
        console.writer_ << std::forward<T>(data);
        return console;
    }

    // Support I/O operator like std::endl ...
    friend this_type & operator << (this_type & console,
                                    std::ostream & (*manip)(std::ostream &)) {
        console.writer_ << manip;
        return console;
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
            case LogLevel::Trace:
                return "Trace";
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
            case LogLevel::Info:
                return L"Info";
            case LogLevel::Verbose:
                return L"Verbose";
            case LogLevel::Debug:
                return L"Debug";
            case LogLevel::Trace:
                return L"Trace";
            default:
                return L"Unknown";
        }
    }

    bool try_log_write(int level) {
        if (can_output(level)) {
            output_header(level);
            return true;
        }
        return false;
    }

    void output_direct(const char_type * text) {
        writer_.print(text);
    }

    void output_args(const char_type * fmt, va_list args) {
        static const size_t kBufSize = 2048;
        static char_type szBuffer[kBufSize] = { 0 };

#ifdef _MSC_VER
        // Use a bounded buffer size to prevent buffer overruns. Limit count to
        // character size minus one to allow for a NULL terminating character.
        HRESULT hr;
        if (kIsWideChar)
            hr = StringCchVPrintfW((STRSAFE_LPWSTR)&szBuffer[0], kBufSize - 1, (const wchar_t *)fmt, args);
        else
            hr = StringCchVPrintfA((STRSAFE_LPSTR)&szBuffer[0], kBufSize - 1, (const char *)fmt, args);
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

    void output_header(int level) {
        if (kIsWideChar)
            output((const char_type *)L"[%s]: ", (const char_type *)getLevelNameW(level));
        else
            output((const char_type *)"[%s]: ", (const char_type *)getLevelName(level));
    }

    void output(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        output_args(fmt, args);
        va_end(args);
    }

    void log_print_args(int level, const char_type * fmt, va_list args) {
        if (can_output(level)) {
            output_header(level);
            output_args(fmt, args);
            new_line();
        }
    }

    void log_print(int level, const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        log_print_args(level, fmt, args);
        va_end(args);
    }

private:
    ConsoleBase(const this_type & src) { }
    ConsoleBase(this_type && src) { }

    ConsoleBase & operator = (const this_type & rhs) { }
    ConsoleBase & operator = (this_type && rhs) { }

protected:
    int         level_;
    int         fileState_;
    std::string filename_;
    std::string pattern_;
    writer_t    writer_;
};

template <typename CharType = char>
struct BasicWriter
{
    typedef CharType                     char_type;
    typedef std::basic_string<char_type> string_type;

    int open(const string_type & filename, bool onlyAppend = true) {
        /* Do nothing ! */
        return FileStatus::Succeeded;
    }
    void close() { /* Do nothing ! */ }
    void flush() { /* Do nothing ! */ }

    void print(const char_type * text) { /* Do nothing ! */ }
    void new_line() { /* Do nothing ! */ }

    template <typename T>
    friend BasicWriter & operator << (BasicWriter & self, const T & data) {
        return self;
    }

    template <typename T>
    friend BasicWriter & operator << (BasicWriter & self, T && data) {
        return self;
    }

    // Support I/O operator like std::endl ...
    friend BasicWriter & operator << (BasicWriter & self,
                                      std::ostream & (*manip)(std::ostream &)) {
        return self;
    }
};

template <typename CharType = char>
struct BasicFileWriter
{
    typedef CharType                     char_type;
    typedef std::basic_string<char_type> string_type;

    int open(const string_type & filename, bool onlyAppend = true) {
        /* Do nothing ! */
        return FileStatus::Succeeded;
    }
    void close() { /* Do nothing ! */ }
    void flush() { /* Do nothing ! */ }

    void print(const char_type * text) { /* Do nothing ! */ }
    void new_line() { /* Do nothing ! */ }
};

struct ConsoleWriter : public BasicWriter<char>
{
    ConsoleWriter() {
        //
    }

    ~ConsoleWriter() {
        //
    }

    void print(const char * text) {
        ::printf(text);
    }

    void new_line() {
#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS)
        ::printf("\r\n");
#else
        ::printf("\n");
#endif
    }
};

struct StdWriter : public BasicWriter<char>
{
    StdWriter() {
        //
    }

    ~StdWriter() {
        //
    }

    void print(const char * text) {
        std::cout << text;
    }

    void new_line() {
        std::cout << std::endl;
    }
};

struct StdFileWriter : public BasicFileWriter<char>
{
    StdFileWriter() {
        //
    }

    ~StdFileWriter() {
        //
    }

    int open(const string_type & filename, bool onlyAppend = true) {
        int status = FileStatus::Unknown;
        if (!out_file_.is_open()) {
            if (onlyAppend)
                out_file_.open(filename, std::ios::out | std::ios::app);
            else
                out_file_.open(filename, std::ios::out | std::ios::trunc);
            if (out_file_.is_open()) {
                filename_ = filename;
                status = FileStatus::Succeeded;
            }
            else if (out_file_.rdstate() & std::ios::failbit) {
                status = FileStatus::Failed;
            }
        }
        else {
            status = FileStatus::Opened;
        }
        return status;
    }

    void close() {
        if (out_file_.is_open()) {
            out_file_.close();
        }
    }

    void flush() {
        if (out_file_.is_open()) {
            out_file_.flush();
        }
    }

    void print(const char * text) {
        out_file_ << text;
    }

    void new_line() {
        out_file_ << std::endl;
    }

    template <typename T>
    friend StdFileWriter & operator << (StdFileWriter & self, const T & data) {
        self.out_file_ << data;
        return self;
    }

    template <typename T>
    friend StdFileWriter & operator << (StdFileWriter & self, T && data) {
        self.out_file_ << std::forward<T>(data);
        return self;
    }

    // Support I/O operator like std::endl ...
    friend StdFileWriter & operator << (StdFileWriter & self,
                                        std::ostream & (*manip)(std::ostream &)) {
        self.out_file_ << manip;
        return self;
    }

    std::string     filename_;
    std::ofstream   out_file_;
};

using Console = ConsoleBase<ConsoleWriter, char>;
using StdConsole = ConsoleBase<StdWriter, char>;
using StdFileLog = ConsoleBase<StdFileWriter, char>;

} // namespace av
