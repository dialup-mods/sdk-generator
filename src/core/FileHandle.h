#pragma once
#include <filesystem>
#include <fstream>
#include <iostream>

#include "MessageBox.h"

class Logger {
    FILE* handle_ = nullptr;

public:
    Logger(const std::filesystem::path& path) {
        handle_ = fopen(path.string().c_str(), "w");
        if (!handle_) {
            messagebox::error("Could not open log file!");
        }
    }

    ~Logger() {
        if (handle_) fclose(handle_);
    }

    void log(const std::string& msg, bool newline = true) {
        if (handle_ && !msg.empty()) {
            fprintf(handle_, "%s%s", msg.c_str(), newline ? "\n" : "");
            fflush(handle_);
        }
    }

    bool isOpen() const { return handle_ != nullptr; }
};