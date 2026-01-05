#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>

#include <Windows.h>

#include "fmt/format.h"
#include "fmt/xchar.h"

#include "ConfigManager.h"
#include "MessageBox.h"

// Logger: a logger built on ~ v i b e s ~
//
//   A precision tool.
//    An elegant weapon.
//
// Features:
//   - RAII for modern, safe init/teardown.
//   - Ref-counted. File opens on first instance, closes on last destruction.
//   - ConfigManager is a proper singleton. It's been done. Why copy-paste when you can.. flex? 💪(ѻ◡⚆)👍
//   - Totally over-engineered for a log file that should just be opened once.
//   - Encourages weird, unnecessary stack allocation of the logger.
//   - Tees output to console/stdout in a streamlined and totally not hacky way.
//   - `log()` has an optional `flush` argument but real talk we flush every. single. time.
//
// "Features":
//   - Awkward to use. Makes your brain tickle a bit.
//   - Does not fit any existing pattern in this codebase.
//   - Building was fun. It's so simple. It's so stupid.
//   - No consideration was given to future-self who gets to implement and maintain code that uses this
//   - It's already, predictably, used in places that aren't properly holding a ref! :D
//
// Usage:
//
//   {
//       Logger scoped;  // log file opens here
//       Logger::log("Wait, but why?");
//   }  // file closes *exactly* here

static std::mutex consoleMutex;

inline auto
enwiden(const std::string& input) -> std::wstring {
    if (input.empty()) return L"";

    int sizeRequired = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, nullptr, 0);
    if (sizeRequired == 0) return L"[enwiden error]";

    std::wstring result(sizeRequired, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, input.c_str(), -1, &result[0], sizeRequired);

    // Remove null terminator Windows sticks in there
    result.pop_back();

    return result;
}

inline auto
enshrinken(const std::wstring& input) -> std::string {
    if (input.empty()) return "";

    int sizeRequired = WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (sizeRequired == 0) return "[enshrinken error]";

    std::string result(sizeRequired, '\0');
    WideCharToMultiByte(CP_UTF8, 0, input.c_str(), -1, &result[0], sizeRequired, nullptr, nullptr);

    // Remove null terminator
    result.pop_back();

    return result;
}

class Logger {
public:
    Logger() = default;
    Logger(const Logger&) = delete;
    auto operator=(const Logger&) -> Logger& = delete;

    static void create() {
        if (!instance_) instance_ = new Logger();
    }

    static void yeet() {
        close();
        delete instance_;
        instance_ = nullptr;
    }

    static auto instance() -> Logger& {
        return *instance_;
    }

    static void open() {
        if (!handle_) {
            const auto config = ConfigManager::instance();
            handle_ = fopen(config.getLogFile().string().c_str(), "w"); // NOLINT
            if (!handle_) {
                messagebox::error("Failed to create log file: " + config.getLogFile().string());
            }
        }
    }

    static void close() {
        if (handle_) {
            fclose(handle_); // NOLINT(*-owning-memory)
            handle_ = nullptr;
        }
    }

    template <typename... Args>
    static void logImpl(const bool newline, fmt::format_string<Args...> fmtstr, Args&&... args) {
        std::lock_guard lock(consoleMutex);

        thread_local fmt::memory_buffer buf;
        buf.clear();

        fmt::format_to(std::back_inserter(buf), fmtstr, std::forward<Args>(args)...);
        if (newline) {
            buf.push_back('\n');
        }

        auto outStr = to_string(buf);

        fmt::print("{}", outStr);
        if (handle_) {
            fmt::print(handle_, "{}", outStr);
        }

        std::cout << std::flush;
        if (handle_) {
            fflush(handle_);
        }
    }

    template <typename... Args>
    static void log(fmt::format_string<Args...> fmtstr, Args&&... args) {
        logImpl(true, fmtstr, std::forward<Args>(args)...);
    }

    template <typename... Args>
    static void logNoNewline(fmt::format_string<Args...> fmtstr, Args&&... args) {
        logImpl(false, fmtstr, std::forward<Args>(args)...);
    }

    //static void log(const std::wstring& ws, const bool newline = true) {
    //    logImpl(newline, "{}", enshrinken(ws));
    //}

    //static void log(const std::wstring& str, const bool bFlush = true, const bool newline = true) {
    //    std::wstring formattedMsg = fmt::format(L"{}{}", str, newline ? L"\n" : L"\b");
    //    std::wcout << formattedMsg.c_str();

    //    if (!handle_) { return; }
    //    fprintf(handle_, enshrinken(formattedMsg).c_str()); // NOLINT(*-pro-type-vararg)
    //    if (bFlush) { fflush(handle_); }
    //}

    template <typename... Args>
    static void print(const bool newline, fmt::format_string<Args...> fmtstr, Args&&... args) {
        std::lock_guard lock(consoleMutex);

        thread_local fmt::memory_buffer buf;
        buf.clear();

        fmt::format_to(std::back_inserter(buf), fmtstr, std::forward<Args>(args)...);
        if (newline) {
            buf.push_back('\n');
        }

        fmt::print("{}", fmt::to_string(buf));
        std::cout << std::flush;
    }

    template <typename... Args>
    static void print(fmt::format_string<Args...> fmtstr, Args&&... args) {
        print(true, fmtstr, std::forward<Args>(args)...);
    }

    static void print(const std::string& str, const bool newline = true) {
        print(newline, "{}", str);
    }

    static void print(const std::wstring& ws, const bool newline = true) {
        print(newline, "{}", enshrinken(ws));
    }

    static void print(const std::wstring& str, const bool bFlush = true, const bool newline = true) {
        std::wstring formattedMsg = fmt::format(L"{}{}", str, newline ? L"\n" : L"\b");
        std::wcout << formattedMsg.c_str();

        if (!handle_) { return; }
        fprintf(handle_, enshrinken(formattedMsg).c_str()); // NOLINT(*-pro-type-vararg)
        if (bFlush) { fflush(handle_); }
    }

    static void printRaw(std::string_view sv, bool newline = true, bool flush = true) {
        std::lock_guard lock(consoleMutex);
        std::cout << sv;
        if (newline) std::cout << '\n';
        if (flush)   std::cout << std::flush;
    }
private:
    ~Logger() = default;

    static inline FILE* handle_{nullptr};
    static inline Logger* instance_{nullptr};
};
