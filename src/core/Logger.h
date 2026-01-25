#pragma once
#include <Windows.h>
#include <filesystem>
#include <iostream>
#include <mutex>

#include "fmt/format.h"
#include "fmt/xchar.h"

#include "ConfigManager.h"
#include "MessageBox.h"
#include "StringUtil.h"

static std::mutex consoleMutex;

class Logger {
    Logger() = default;
    ~Logger() { close(); }

public:
    static auto instance() -> Logger& {
        static Logger instance;
        return instance;
    }

    Logger(Logger&&) = delete;
    Logger(const Logger&) = delete;
    auto operator=(Logger&&) -> Logger& = delete;
    auto operator=(const Logger&) -> Logger& = delete;

    void open() {
        if (!handle_) {
            printf("Log file: %s\n", ConfigManager::instance().getLogFile().string().c_str());
            handle_ = fopen(ConfigManager::instance().getLogFile().string().c_str(), "w"); // NOLINT
            if (!handle_) {
                messagebox::error("Failed to create log file: " + ConfigManager::instance().getLogFile().string());
            }
        }
    }

    void close() {
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
        if (instance().handle_) {
            fmt::print(instance().handle_, "{}", outStr);
        }

        std::cout << std::flush;
        if (instance().handle_) {
            fflush(instance().handle_);
        }
    }

    template <typename... Args>
    void log(fmt::format_string<Args...> fmtstr, Args&&... args) {
        logImpl(true, fmtstr, std::forward<Args>(args)...);
    }

    template <typename... Args>
    void logNoNewline(fmt::format_string<Args...> fmtstr, Args&&... args) {
        logImpl(false, fmtstr, std::forward<Args>(args)...);
    }

    //static void log(const std::wstring& ws, const bool newline = true) {
    //    logImpl(newline, "{}", util::string::enshrinken(ws));
    //}

    //static void log(const std::wstring& str, const bool bFlush = true, const bool newline = true) {
    //    std::wstring formattedMsg = fmt::format(L"{}{}", str, newline ? L"\n" : L"\b");
    //    std::wcout << formattedMsg.c_str();

    //    if (!handle_) { return; }
    //    fprintf(handle_, util::string::enshrinken(formattedMsg).c_str()); // NOLINT(*-pro-type-vararg)
    //    if (bFlush) { fflush(handle_); }
    //}

    template <typename... Args>
    void print(const bool newline, fmt::format_string<Args...> fmtstr, Args&&... args) {
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
    void print(fmt::format_string<Args...> fmtstr, Args&&... args) {
        print(true, fmtstr, std::forward<Args>(args)...);
    }

    void print(const std::string& str, const bool newline = true) {
        print(newline, "{}", str);
    }

    void print(const std::wstring& ws, const bool newline = true) {
        print(newline, "{}", util::string::enshrinken(ws));
    }

    void print(const std::wstring& str, const bool bFlush = true, const bool newline = true) {
        std::wstring formattedMsg = fmt::format(L"{}{}", str, newline ? L"\n" : L"\b");
        std::wcout << formattedMsg.c_str();

        if (!handle_) { return; }
        fmt::print(handle_, "{}", util::string::enshrinken(formattedMsg));
        if (bFlush) { fflush(handle_); }
    }

    void printRaw(std::string_view sv, bool newline = true, bool flush = true) {
        std::lock_guard lock(consoleMutex);
        std::cout << sv;
        if (newline) std::cout << '\n';
        if (flush)   std::cout << std::flush;
    }

private:
    static inline FILE* handle_{nullptr};
};