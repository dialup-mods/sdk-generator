#include "Runtime.h"
#include "SDK.h"
#include "Schema.h"
#include "StringUtil.h"

TArray<FNameEntry*>* r::fNameEntries_{nullptr};
TArray<UObject*>* r::uObjects_{nullptr};
Runtime* r::instance_{nullptr};
std::map<std::string, UClass*> r::classCache_{};
std::map<std::string, UFunction*> r::functionCache_{};
std::vector<UObject*> r::uObjectsCache_{};

std::unordered_map<int32_t, FName>        r::fname::cache_{};
std::unordered_map<std::wstring, int32_t> r::fname::name_to_id_{};

using tProcessEvent = void(__fastcall*)(UObject* self, UFunction* fn, void* params, void* result);
using r = Runtime;

SDK_API void r::create() {
    if (!instance_) instance_ = new Runtime();
}

SDK_API auto r::instance() -> Runtime& {
    if (!instance_) {
        printf("[ERROR] No instance exists. Did you call `create()`?\n");
    }
    return *instance_;
}

SDK_API void r::yeet() {
    delete instance_;
    instance_ = nullptr;
}


SDK_API auto r::uclass::find(const std::string& classFullName) -> UClass* {
    if (classCache_.empty()) {
        for (int32_t i = 0; i < uObjects_->size() - 1; i++) {
            if (UObject* uObject = uObjects_->at(i)) {
                if (std::string objectFullName = uObject->GetFullName(); objectFullName.starts_with("Class")) {
                    classCache_[objectFullName] = reinterpret_cast<UClass*>(uObject);
                }
            }
        }
    }

    if (classCache_.contains(classFullName)) {
        return classCache_[classFullName];
    }

    return nullptr;
}

SDK_API auto r::ufunction::find(const std::string& functionFullName) -> UFunction* {
    if (functionCache_.empty()) {
        for (int32_t i = 0; i < uObjects_->size() - 1; i++) {
            if (UObject* uObject = uObjects_->at(i)) {
                if (std::string objectFullName = uObject->GetFullName(); objectFullName.find("Function") == 0) {
                    functionCache_[objectFullName] = reinterpret_cast<UFunction*>(uObject);
                }
            }
        }
    }

    if (functionCache_.contains(functionFullName)) {
        return functionCache_[functionFullName];
    }

    return nullptr;
}

SDK_API void r::process_event::call(UObject* self, UFunction* function, void* params, void* unusedResult) {
    auto vtable = static_cast<void**>(uclass::find("Class Core.Object")->VfTableObject.Ptr);
    auto processEvent = reinterpret_cast<tProcessEvent>(vtable[67]);
    processEvent(self, function, params, unusedResult);
}

SDK_API void r::process_event::call(UObject* self, UFunction* function, void* params) {
    call(self, function, params, nullptr);
}

SDK_API auto r::packages::find() -> std::vector<UObject*> {
    static std::vector<UObject*> packages;
    if (packages.empty()) {
        for (int i = 0; i < 10; ++i) {
            if (UObject* obj = uobject::game_pool::ref().at(i); obj && !obj->GetName().empty()) {
                packages.emplace_back(obj);
            }
        }
    }
    return packages;
}

// FName
SDK_API auto r::fname::game_pool::isValid() -> bool {
    if (ref().empty()) {
    	// fixme, return error string
        return false;
    }

    if (ref().at(0)->ToString() != "None") {
    	// fixme, return error string
        return false;
    }

    if (ref().size() < 1000) {
    	// fixme, return error string
        return false;
    }

    return true;
}

SDK_API auto r::fname::game_pool::find(const wchar_t* wanted)
    -> std::optional<std::reference_wrapper<const FName>> {

    // Fast path: check reverse map
    if (auto it = name_to_id_.find(wanted); it != name_to_id_.end()) {
        return find(it->second);  // Use ID lookup (already cached)
    }

    // Slow path: search FNameEntries
    const auto& entries = ref();
    for (int32_t i = 0; i < entries.size(); ++i) {
        if (const auto& entry = entries[i]) {
            if (std::wstring_view(entry->Name) == wanted) {
                // Cache both directions
                name_to_id_[wanted] = i;
                auto [it, inserted] = cache_.try_emplace(i, FName{i, 0});
                return std::cref(it->second);
            }
        }
    }

    return std::nullopt;
}

SDK_API auto r::fname::game_pool::find(int32_t id) -> std::optional<std::reference_wrapper<const FName>> {
    const auto& entries = ref();
    if (id < 0 || id >= entries.size()) {
        return std::nullopt;
    }

    if (!entries[id]) {
        return std::nullopt;
    }

    // Lazy-populate cache
    auto [it, inserted] = cache_.try_emplace(id, FName{id, 0});
    return std::cref(it->second);
}

SDK_API auto r::fname::unknown() -> const FName& {
    static FName unknown = []() {
        // shouldn't happen, but
        // fallback to "None" if "Unknown" doesn't exist
        return game_pool::find(L"Unknown").value_or(none());
    }();
    return unknown;
}

SDK_API auto r::fname::none() -> const FName& {
    static FName none = []() {
        return FName{ 0, 0 };
    }();
    return none;
}

SDK_API auto r::fname::game_pool::getWString(int32_t id) -> std::optional<std::wstring> {
    const auto& entries = ref();
    if (id >= 0 && id < entries.size() && entries[id]) {
        return std::wstring(entries[id]->Name);
    }
    return std::nullopt;
}

SDK_API auto r::fname::game_pool::getString(int32_t id) -> std::optional<std::string> {
    auto wstr = getWString(id);
    if (wstr) {
        return util::string::enshrinken(wstr.value());
    }
    return std::nullopt;
}

SDK_API auto r::uobject::game_pool::isPopulated() -> bool {
    if (ptr()->empty()) {
        return false;
    }
    return true;
}