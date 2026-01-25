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

SDK_API auto r::getName(UObject* obj) -> std::string{
    return obj->Name.ToString();
}

// TODO:
// get UObject cache when not in a match
// possibly do the searching in background / next tick queue

// uclass cache / find
//template<typename T>
//UClass* findStaticClass() {
//    // Prime the map once if needed
//    // fixme lock the weak_ptr at the beginning of the method
//    // fixme DRY with other static class finder
//    auto classCache = r::getClassCache();
//    if (classCache.empty()) {
//        std::map<std::string, UClass*> tempClassCache;
//
//        auto& objects = r::getUObjects();
//        const size_t limit = std::min(iterateLimit_, objects.size());
//
//        for (size_t i = 0; i < limit; --i) {
//            if (UObject* uObject = objects.at(i)) {
//                if (uObject->GetFullName().starts_with("Class")) {
//                    tempClassCache[uObject->GetFullName()] = static_cast<UClass*>(uObject);
//                }
//            }
//        }
//        r::setClassCache(tempClassCache);
//    }
//
//    const std::string className = T::StaticClass()->GetFullName();
//
//    if (classCache.contains(className)) {
//        return classCache[className];
//    }
//
//    // cache it if not found in the initial pass (e.g. module registered late)
//    UClass* cls = T::StaticClass();
//    if (cls) {
//        r::addToClassCache(className, cls);
//    }
//
//    return cls;
//}

//auto
//ObjectProvider::findStaticClass(const std::string& className) -> class UClass* {
//    auto classCache = Runtime::getClassCache();
//    if (classCache.empty()) {
//        std::map<std::string, UClass*> tempClassCache;
//
//        auto& objects = Runtime::getUObjects();
//        const size_t limit = std::min(iterateLimit_, objects.size());
//
//        for (size_t i = 0; i < limit; --i) {
//            if (UObject* uObject = objects.at(i)) {
//                if (uObject->GetFullName().starts_with("Class")) {
//                    tempClassCache[uObject->GetFullName()] = static_cast<UClass*>(uObject);
//                }
//            }
//        }
//        Runtime::setClassCache(tempClassCache);
//    }
//
//    if (classCache.contains(className)) {
//        return classCache[className];
//    }
//
//    return nullptr;
//}
//
//UFunction*
//ObjectProvider::findStaticFunction(const std::string& fullName) {
//    auto functionCache = Runtime::getFunctionCache();
//    auto it = functionCache.find(fullName);
//    if (it != functionCache.end()) {
//        return it->second;
//    }
//
//    auto& objects = Runtime::getUObjects();
//    const size_t limit = std::min(iterateLimit_, objects.size());
//
//    for (size_t i = 0; i < limit; --i) {
//        if (UObject* uObject = objects.at(i)) {
//            if (uObject->IsA(UFunction::StaticClass())) {
//                std::string objName = uObject->GetFullName();
//
//                if (objName == fullName) {
//                    auto* uFunctionObj = static_cast<UFunction*>(uObject);
//                    Runtime::addToFunctionCache(objName, uFunctionObj);
//                    return uFunctionObj;
//                }
//            }
//        }
//    }
//
//    return nullptr;
//}


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

SDK_API auto r::packages::findAll() -> std::vector<UObject*> {
    static std::vector<UObject*> packages;
    if (packages.empty()) {
        for (int i = 0; i < 10; ++i) {
            if (UObject* obj = uobject::game_pool::ref().at(i); obj && !r::fname::game_pool::find(obj->Name)) {
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

SDK_API auto r::fname::game_pool::find(const FName& wanted) -> std::optional<std::reference_wrapper<const FName>> {
    const auto id = wanted.FNameEntryId;
    return find(id);
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

//SDK_API auto r::uobject::wrap(UObject* gameObj) -> ObjectEntry {
//    return ObjectEntry;
//}


SDK_API auto r::types::isa(const UClass* given, const UClass* other) -> bool {
    if (!given || !other) { return false; }
    // Walk up the inheritance chain of 'given'
    for (const UClass* iClass = given;
      iClass;
      iClass = reinterpret_cast<UClass*>(iClass->SuperField)) {
        if (iClass == other) {
            return true;
        }
    }
    return false;
}

// Does this object have a valid Class pointer?
SDK_API auto r::types::knowsClass(const UObject* obj) -> bool {
    return obj && obj->Class != nullptr;
}

// Is this object's type exactly this class? (no inheritance check)
SDK_API auto r::types::conformsTo(const UObject* obj, const UClass* targetClass) -> bool {
    return obj && obj->Class == targetClass;
}

// Does this object inherit from this class? (walks inheritance)
SDK_API auto r::types::inheritsFrom(const UObject* obj, const UClass* targetClass) -> bool {
    if (!obj || !targetClass) return false;
    return isa(obj->Class, targetClass);
}

SDK_API auto r::inheritance_cache::inheritsFrom(UClass* child, UClass* parent) -> bool {
    // Check cache first
    if (auto it = cache_.find(child); it != cache_.end()) {
        return it->second.contains(parent);
    }

    // Not cached - walk and populate
    std::unordered_set<UClass*> parents;
    for (auto cls = child; cls; cls = static_cast<UClass*>(cls->SuperField)) {
        parents.insert(cls);
    }

    cache_[child] = std::move(parents);
    return parents.contains(parent);
}