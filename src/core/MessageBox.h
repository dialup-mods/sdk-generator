#pragma once
#include <string>
#include <Windows.h>

namespace messagebox {

inline void
ext(const std::string& message, const uint32_t flags) {
    MessageBoxA(nullptr, message.c_str(), "DialUp SDK Generator", flags);
}

inline void
info(const std::string& message) {
    ext(message, (MB_OK | MB_ICONINFORMATION));
}

inline void
warn(const std::string& message) {
    ext(message, (MB_OK | MB_ICONWARNING));
}

inline void
error(const std::string& message) {
    ext(message, (MB_OK | MB_ICONERROR));
}

}