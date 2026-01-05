#pragma once
#include <string>
#include <vector>
#include <sstream>
#include <fmt/format.h>

namespace cowsay {
inline auto say(const std::string& message) -> std::string {
    std::string result;

    // Split message into lines if it's long
    std::vector<std::string> lines;
    std::istringstream iss(message);
    std::string line;
    while (std::getline(iss, line)) {
        lines.push_back(line);
    }

    // If single line is too long, wrap it
    if (lines.size() == 1 && lines[0].length() > 40) {
        std::string long_line = lines[0];
        lines.clear();
        for (size_t i = 0; i < long_line.length(); i += 40) {
            lines.push_back(long_line.substr(i, 40));
        }
    }

    // Find the maximum line length
    size_t max_length = 0;
    for (const auto& l : lines) {
        max_length = std::max(max_length, l.length());
    }

    // Top border
    result += fmt::format("/*{}\n", std::string(max_length + 2, '_'));

    // Message lines
    if (lines.size() == 1) {
        result += fmt::format("< {:<{}} >\n", lines[0], max_length);
    } else {
        for (size_t i = 0; i < lines.size(); ++i) {
            char left_char, right_char;
            if (i == 0) {
                left_char = '/'; right_char = '\\';
            } else if (i == lines.size() - 1) {
                left_char = '\\'; right_char = '/';
            } else {
                left_char = '|'; right_char = '|';
            }

            result += fmt::format("{} {:<{}} {}\n", left_char, lines[i], max_length, right_char);
        }
    }

    // Bottom border
    result += fmt::format(" {}\n", std::string(max_length + 2, '-'));

    // The cow
    result += "        \\   ^__^\n"
              "         \\  (oo)\\_______\n"
              "            (__)\\       )\\/\\\n"
              "                ||----w |\n"
              "                ||     ||\n"
              "*/\n";

    return result;
}
}

// Usage examples:
// fmt::print("{}", cowsay("Hello World!"));
// fmt::print("{}", cowsay("Generated {} objects!", objectCount));
// fmt::print(stderr, "{}", cowsay("ERROR: Something went wrong!"));