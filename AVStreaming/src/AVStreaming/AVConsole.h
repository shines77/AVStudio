
#pragma once

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN 1
#endif
#include <windows.h>
#include <tchar.h>
#endif // _WIN32 || _WIN64

#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>

#include <cstdio>
#include <iostream>
#include <fstream>
#include <codecvt>
#include <locale>
#include <string>

#include "utils.h"
#include "string_utils.h"

// std::ofstream 使用 Unicode (UTF-16E) 格式输出, 不推荐
// 不定义该项，则转换为 Ansi (系统默认的 GBK) 格式输出.
// #define USE_UTF16E_FILE_STREAM

namespace av {

struct LogLevel
{
    enum {
        // Must be start with 0
        First = 0,
        Fatal = First,
        Error,
        Warning,
        Info,
        Verbose,
        Debug,      // Output only in debug mode
        Trace,      // Output always
        Last
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
        Failed = -1,
        Unknown,
        Openning,
        Closed
    };
};

template <typename Writer, typename CharType = char>
class ConsoleBase
{
public:
    typedef CharType                        char_type;
    typedef Writer                          writer_t;
    typedef ConsoleBase<Writer, CharType>   this_type;
    typedef std::basic_string<char_type>    string_type;

    static const bool kIsWideChar = (sizeof(CharType) > 1);

    ConsoleBase() : level_(LogLevel::Info), file_state_(FileStatus::Unknown) {
    }

    ConsoleBase(const std::string & filename, bool onlyAppend = true)
        : level_(LogLevel::Info), file_state_(FileStatus::Unknown) {
        open(filename, onlyAppend);
    }
    ~ConsoleBase() {
        flush();
        close();
    }

    int get_log_level() const {
        return level_;
    }

    void set_log_level(int level) {
        if (level >= LogLevel::Fatal && level < LogLevel::Last) {
            level_ = level;
        }
    }

    bool is_open() const {
        return (file_state_ == FileState::Openning);
    }

    string_type filename() const {
        return filename_;
    }

    bool is_stream_writer() const {
        return writer_.is_stream_writer();
    }

    int open(const string_type & filename, bool onlyAppend = true) {
        int state = FileState::Unknown;
        if (filename.empty()) {
            return state;
        }
        // If file is opened, close it.
        if (is_open()) {
            if (!filename_.empty()) {
                string_type lower_filename1(filename);
                string_type lower_filename2(filename_);
                str_to_lower(lower_filename1);
                str_to_lower(lower_filename2);
                // If filename is same, skip to close.
                if (lower_filename1 != lower_filename2) {
                    flush();
                    state = close();
                    filename_ = _T("");
                }
            }
        }
        if (!is_open()) {
            int status = writer_.open(filename, onlyAppend);
            state = (status == FileStatus::Succeeded || status == FileStatus::Opened) ?
                     FileState::Openning : FileState::Failed;
            file_state_ = state;
            filename_ = filename;
        }
        return state;
    }

    int close() {
        int state = FileState::Unknown;
        if (is_open()) {
            writer_.flush();
            writer_.close();
            state = FileState::Closed;
            file_state_ = state;
        }
        return state;
    }

    void flush() {
        if (is_open()) {
            writer_.flush();
        }
    }

    void fatal(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        write_log_args(LogLevel::Fatal, fmt, args);
        va_end(args);
    }

    void error(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        write_log_args(LogLevel::Error, fmt, args);
        va_end(args);
    }

    void warning(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        write_log_args(LogLevel::Warning, fmt, args);
        va_end(args);
    }

    void info(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        write_log_args(LogLevel::Info, fmt, args);
        va_end(args);
    }

    void verbose(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        write_log_args(LogLevel::Verbose, fmt, args);
        va_end(args);
    }

    void debug(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        write_log_args(LogLevel::Debug, fmt, args);
        va_end(args);
    }

    void trace(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        write_log_args(LogLevel::Trace, fmt, args);
        va_end(args);
    }

    bool try_fatal() {
        return try_write_log(LogLevel::Fatal);
    }

    bool try_error() {
        return try_write_log(LogLevel::Error);
    }

    bool try_warning() {
        return try_write_log(LogLevel::Warning);
    }

    bool try_info() {
        return try_write_log(LogLevel::Info);
    }

    bool try_verbose() {
        return try_write_log(LogLevel::Verbose);
    }

    bool try_debug() {
        return try_write_log(LogLevel::Debug);
    }

    bool try_trace() {
        return try_write_log(LogLevel::Trace);
    }

    void new_line() {
        writer_.new_line();
    }

    void print(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        write_args(fmt, args);
        va_end(args);
    }

    void println(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);

        write_args(fmt, args);
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
    const char * level_names[8] = {
        "Fatal",
        "Error",
        "Warning",
        "Info",
        "Verbose",
        "Debug",
        "Trace",
        "Unknown"
    };

    const wchar_t * level_names_w[8] = {
        L"Fatal",
        L"Error",
        L"Warning",
        L"Info",
        L"Verbose",
        L"Debug",
        L"Trace",
        L"Unknown"
    };

    const char * get_level_name(int level) {
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

    const wchar_t * get_level_nameW(int level) {
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

    bool can_write(int level) const {
        return (level != LogLevel::Trace) ? (level <= level_) : true;
    }

    bool try_write_log(int level) {
        if (can_write(level) && writer_.is_stream_writer()) {
            write_header(level);
            return true;
        }
        return false;
    }

    void write_header(int level) {
#if 1
        // Use array faster than switch version.
        level = (level >= LogLevel::First) ? level : LogLevel::First;
        level = (level <  LogLevel::Last)  ? level : LogLevel::Last;
        if (kIsWideChar)
            write((const char_type *)L"[%s]: ", (const char_type *)level_names_w[level]);
        else
            write((const char_type *)"[%s]: ", (const char_type *)level_names[level]);
#else
        if (kIsWideChar)
            write((const char_type *)L"[%s]: ", (const char_type *)get_level_nameW(level));
        else
            write((const char_type *)"[%s]: ", (const char_type *)get_level_name(level));
#endif
    }

    void write_direct(const char_type * text) {
        writer_.write(text);
    }

    void write_args(const char_type * fmt, va_list args) {
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

        writer_.write(szBuffer);
    }

    void write(const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        write_args(fmt, args);
        va_end(args);
    }

    void write_log_args(int level, const char_type * fmt, va_list args) {
        if (can_write(level)) {
            write_header(level);
            write_args(fmt, args);
            new_line();
        }
    }

    void write_log(int level, const char_type * fmt, ...) {
        va_list args;
        va_start(args, fmt);
        write_log_args(level, fmt, args);
        va_end(args);
    }

private:
    ConsoleBase(const this_type & src) { }
    ConsoleBase(this_type && src) { }

    ConsoleBase & operator = (const this_type & rhs) { }
    ConsoleBase & operator = (this_type && rhs) { }

protected:
    int         level_;
    int         file_state_;
    string_type filename_;
    string_type pattern_;
    writer_t    writer_;
};

template <typename CharType = char>
struct BasicWriter
{
    typedef CharType                     char_type;
    typedef std::basic_string<char_type> string_type;

    static const bool kIsStreamWriter = false;
    static const bool kIsWideChar = (sizeof(char_type) > 1);

    BasicWriter() { }
    ~BasicWriter() { }

    bool is_stream_writer() const {
        return kIsStreamWriter;
    }

    int open(const string_type & filename, bool onlyAppend = true) {
        /* Do nothing ! */
        return FileStatus::Succeeded;
    }
    void close() { /* Do nothing ! */ }
    void flush() { /* Do nothing ! */ }

    void write(const char_type * text) { /* Do nothing ! */ }
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
struct BasicStreamWriter
{
    typedef CharType                     char_type;
    typedef std::basic_string<char_type> string_type;

    static const bool kIsStreamWriter = true;
    static const bool kIsWideChar = (sizeof(char_type) > 1);

    BasicStreamWriter() { }
    ~BasicStreamWriter() { }

    bool is_stream_writer() const {
        return kIsStreamWriter;
    }

    int open(const string_type & filename, bool onlyAppend = true) {
        /* Do nothing ! */
        return FileStatus::Succeeded;
    }
    void close() { /* Do nothing ! */ }
    void flush() { /* Do nothing ! */ }

    void write(const char_type * text) { /* Do nothing ! */ }
    void new_line() { /* Do nothing ! */ }
};

template <typename CharType>
struct ConsoleWriter : public BasicWriter<CharType>
{
    typedef CharType char_type;

    void write(const char_type * text) {
        if (kIsWideChar)
            ::printf((const wchar_t *)text);
        else
            ::printf((const char *)text);
    }

    void new_line() {
#if defined(_WIN32) || defined(_WIN64) || defined(_WINDOWS)
        if (kIsWideChar)
            ::wprintf(L"\r\n");
        else
            ::printf("\r\n");
#else
        ::printf("\n");
#endif
    }
};

template <typename CharType>
struct StdWriter : public BasicWriter<CharType>
{
    typedef CharType char_type;

    void write(const char_type * text) {
        std::cout << text;
    }

    void new_line() {
        std::cout << std::endl;
    }
};

template <typename CharType>
struct StdFileWriter : public BasicStreamWriter<CharType>
{
    typedef CharType                     char_type;
    typedef std::basic_string<char_type> string_type;

    int open(const string_type & filename, bool onlyAppend = true) {
        int status = FileStatus::Unknown;
        if (!out_file_.is_open()) {
            if (onlyAppend)
                out_file_.open(filename, std::ios::out | std::ios::binary | std::ios::app);
            else
                out_file_.open(filename, std::ios::out | std::ios::binary | std::ios::trunc);
            if (out_file_.is_open()) {
#ifdef USE_UTF16E_FILE_STREAM
                // Setting file stream's locale to UTF-16
                if (kIsWideChar) {
                    out_file_.imbue(std::locale(std::locale(),
                                    new std::codecvt_utf16<wchar_t, 0x10ffff, std::consume_header>));
                }
#endif
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

    void write(const char_type * text) {
#ifdef USE_UTF16E_FILE_STREAM
        out_file_ << text;
#else
        out_file_ << string_utils::unicode_to_ansi(text);
#endif
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

#ifdef USE_UTF16E_FILE_STREAM
    std::basic_ofstream<char_type> out_file_;
#else
    std::ofstream out_file_;
#endif
};

#if defined(_WIN32) || defined(_WIN64)

struct OutputDebugWriter : public BasicWriter<TCHAR>
{
    void write(const TCHAR * text) {
        ::OutputDebugString(text);
    }

    void new_line() {
        ::OutputDebugString(_T("\r\n"));
    }
};

struct OutputDebugWriterA : public BasicWriter<char>
{
    void write(const char * text) {
        ::OutputDebugStringA(text);
    }

    void new_line() {
        ::OutputDebugStringA("\r\n");
    }
};

struct OutputDebugWriterW : public BasicWriter<wchar_t>
{
    void write(const wchar_t * text) {
        ::OutputDebugStringW(text);
    }

    void new_line() {
        ::OutputDebugStringW(L"\r\n");
    }
};

#endif // _WIN32 || _WIN64

using Console = ConsoleBase<ConsoleWriter<char>, char>;
using StdConsole = ConsoleBase<StdWriter<char>, char>;
using StdFileLog = ConsoleBase<StdFileWriter<char>, char>;

#if defined(_WIN32) || defined(_WIN64)
using ConsoleT = ConsoleBase<ConsoleWriter<TCHAR>, TCHAR>;
using StdConsoleT = ConsoleBase<StdWriter<TCHAR>, TCHAR>;
using StdFileLogT = ConsoleBase<StdFileWriter<TCHAR>, TCHAR>;

using OutputDebugConsole = ConsoleBase<OutputDebugWriterA, char>;
using OutputDebugConsoleT = ConsoleBase<OutputDebugWriter, TCHAR>;
#endif // _WIN32 || _WIN64

} // namespace av
