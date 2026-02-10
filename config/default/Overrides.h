#pragma once
#include <string>
#include <unordered_map>

namespace turdsofoverride {

inline const std::unordered_map<std::string, const char*> overrides_ = {
{ "TAsyncResult", R"(
// foo
// asdf
)" },
};

inline auto getOverride(const std::string& name) -> const char* {
    auto it = overrides_.find(name);
    if (it != overrides_.end()) {
        return it->second;
    }
    static const std::string empty;
    return empty.c_str();
}

} // namespace