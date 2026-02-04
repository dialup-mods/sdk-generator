#include "Runtime.h"
#include "SDK.h"
#include "Schema.h"
#include "StringUtil.h"

using r = Runtime;

std::unordered_map<
    std::string,
    UClass*,
    TransparentHash,
    TransparentEq
> r::classCache_{};

TArray<FNameEntry*>* r::fNameEntries_{nullptr};
TArray<UObject*>* r::uObjects_{nullptr};
Runtime* r::instance_{nullptr};
std::map<std::string, UFunction*> r::functionCache_{};
std::vector<UObject*> r::uObjectsCache_{};

std::unordered_map<std::type_index, void*> r::instanceCache_{};
std::unordered_map<UClass*, UObject*> r::classToCDO_{};
ClassNameToClassCache r::classNameToClass_{};

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

// StaticClass should return the UClass* whose CDO is a UNotificationManager_TA instance.

// fixme dumb and bad
SDK_API auto r::uclass::find(std::string_view classFullName) -> UClass* {
    if (classCache_.empty()) {
        for (UObject* uObject : uobject::game_pool::ref()) {
            if (uObject) {
                auto fullName = uObject->GetFullName();
                // seriously wtf
                if (fullName.starts_with("Class")) {
                    classCache_.emplace(std::move(fullName),
                        static_cast<UClass*>(uObject));
                }
            }
        }
    }

    auto it = classCache_.find(classFullName);
    return it != classCache_.end() ? it->second : nullptr;
}

// fixme also dumb and bad
SDK_API auto r::ufunction::find(const std::string& functionFullName) -> UFunction* {
    if (functionCache_.empty()) {
        for (int32_t i = 0; i < uObjects_->size() - 1; i++) {
            if (UObject* uObject = uObjects_->at(i)) {
                // also wtf
                if (std::string objectFullName = uObject->GetFullName(); objectFullName.starts_with("Function")) {
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

SDK_API auto r::fname::game_pool::find(const int32_t id) -> std::optional<std::reference_wrapper<const FName>> {
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

// the engine's pre-existing `unknown` FName
SDK_API auto r::fname::unknown() -> const FName& {
    static FName unknown = []() {
        // shouldn't happen, but
        // fallback to "None" if "Unknown" doesn't exist
        return game_pool::find(L"Unknown").value_or(none());
    }();
    return unknown;
}

// the engine's pre-existing `none` FName
SDK_API auto r::fname::none() -> const FName& {
    static FName none = []() {
        return FName{ 0, 0 };
    }();
    return none;
}

SDK_API auto r::fname::game_pool::getWString(const int32_t id) -> std::optional<std::wstring> {
    const auto& entries = ref();
    if (id >= 0 && id < entries.size() && entries[id]) {
        return std::wstring(entries[id]->Name);
    }
    return std::nullopt;
}

SDK_API auto r::fname::game_pool::getString(const int32_t id) -> std::optional<std::string> {
    if (auto wstr = getWString(id)) {
        return util::string::enshrinken(wstr.value());
    }
    return std::nullopt;
}

SDK_API auto r::fname::game_pool::getWString(const FName& wanted) -> std::optional<std::wstring> {
    if (auto fnameMaybe = find(wanted)) {
        return getWString(fnameMaybe.value().get().FNameEntryId);
    }
    return std::nullopt;
}

SDK_API auto r::fname::game_pool::getString(const FName& wanted) -> std::optional<std::string> {
    if (const auto fnameMaybe = find(wanted)) {
        return getString(fnameMaybe.value().get().FNameEntryId);
    }
    return std::nullopt;
}

SDK_API auto r::uobject::game_pool::isPopulated() -> bool {
    if (!ptr()->empty()) {
        return true;
    }
    return false;
}

//SDK_API auto r::uobject::wrap(UObject* gameObj) -> ObjectEntry {
//    return ObjectEntry;
//}

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

    const auto ret = parents.contains(parent);
    cache_[child] = std::move(parents);
    return ret;
}


SDK_API auto r::types::isa(const UClass* given, const UClass* other) -> bool {
    if (!given || !other) { return false; }

    std::unordered_set<const UClass*> visited;  // Detect cycles

    for (const UClass* cls = given; cls; cls = reinterpret_cast<UClass*>(cls->SuperField)) {
        if (visited.contains(cls)) {
            printf("ERROR: Cycle detected in inheritance chain at %p\n", cls);
            return false;
        }
        visited.insert(cls);

        if (cls == other) return true;
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
SDK_API auto r::types::inheritsFrom(UObject* obj, UClass* targetClass) -> bool {
    if (!obj || !targetClass) return false;
    return isa(obj->Class, targetClass);
}

SDK_API auto r::types::getNamePrefix(UObject* obj) -> std::string {
    if (!obj || !obj->Class) return "U";

    if (conformsTo(obj, uclass::find("Class Core.ScriptStruct"))) { return "F"; }
    if (inheritsFrom(obj, uclass::find("Class Engine.Actor"))) { return "A"; }

    return "U";
}

SDK_API auto r::uobject_utils::getName(const UObject* obj) -> std::string {
    return obj->Name.ToString();
}

SDK_API auto r::uobject_utils::getNameCPP(UObject* obj) -> std::string {
    return r::types::getNamePrefix(obj) + getName(obj);
}

SDK_API auto r::uobject_utils::getFullName(const UObject* obj) -> std::string {
    std::string fullName = getName(obj);
    for (auto outer = obj->Outer; outer; outer = outer->Outer) {
        fullName = getName(outer) + "." + fullName;
    }
    return obj->Class->Name.ToString() + " " + fullName;
}

SDK_API auto r::uobject_utils::hasAnyFlags(const UObject* obj, const EObjectFlags flags) -> bool {
    return (obj->ObjectFlags & flags) != 0;
}

SDK_API auto r::uobject_utils::hasAllFlags(const UObject* obj, const EObjectFlags flags) -> bool {
    return (obj->ObjectFlags & flags) == flags;
}

SDK_API auto r::uobject_utils::getPackage(const UObject* obj) -> UObject* {
    UObject* pkg = nullptr;
    for (auto outer = obj->Outer; outer; outer = outer->Outer) {
        pkg = outer;
    }
    return pkg;
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