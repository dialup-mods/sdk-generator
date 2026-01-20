#pragma once
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#if defined(SDK_BUILD)
#define SDK_API __declspec(dllexport)
#else
#define SDK_API __declspec(dllimport)
#endif

class UClass;
class UFunction;
class FNameEntry;
class UObject;
template<typename T> class TArray;

class SDK_API Runtime {
    static Runtime* instance_;

    static TArray<FNameEntry*>* fNameEntries_;
    static TArray<UObject*>* uObjects_;

    static std::map<std::string, UClass*> classCache_;
    static std::map<std::string, UFunction*> functionCache_;
    static std::vector<UObject*> uObjectsCache_;

public:
    Runtime() = default;
    ~Runtime() = default;

    static void create();
    static auto instance() -> Runtime&;
    static void yeet();

    static void setUObjects(TArray<UObject*>* objs) { uObjects_ = objs; }

    static auto hasUObjects() -> bool { return uObjects_ != nullptr; }
    static auto hasFNames() -> bool { return fNameEntries_ != nullptr; }

    static auto getUObjectsPtr() -> TArray<UObject*>* { return uObjects_; }

    // unsafe -- assumes initialized
    static auto getUObjects() -> TArray<UObject*>& { return *instance().uObjects_; }

    static auto findClass(const std::string& classFullName) -> UClass*;
    static auto findFunction(const std::string& functionFullName) -> UFunction*;

    static auto areFNameEntriesValid() -> bool;
    static auto areUObjectsPopulated() -> bool;

    static void setFNameEntries(TArray<FNameEntry*>* names) { fNameEntries_ = names; }
    static auto getFNameEntries() -> TArray<FNameEntry*>&;

    static auto getFNameEntriesPtr() -> TArray<FNameEntry*>* { return fNameEntries_; }
    static auto getFNameEntry(int32_t index) -> FNameEntry*;

    static auto getFNameEntryName(int32_t index) -> std::string;
    static void callProcessEvent(UObject* obj, UFunction* fn, void* params);
    static auto findPackages() -> std::vector<UObject*>;

    static auto getRawObjects() -> const std::vector<UObject*>& { return uObjectsCache_; }

    static auto getObjectCache() -> std::vector<UObject*>& { return uObjectsCache_; }
    static auto getFunctionCache() -> std::map<std::string, UFunction*>& { return functionCache_; }

    static void setClassCache(const std::map<std::string, UClass*>& cache) { classCache_ = std::move(cache); }
    static auto getClassCache() -> std::map<std::string, UClass*>& { return classCache_; }
    static void addToClassCache(const std::string& name, UClass* cls) { classCache_[name] = cls; }
    static void addToFunctionCache(const std::string& name, UFunction* fn) { functionCache_[name] = fn; }
};

namespace sdk_internal {
template<typename T>
auto getVirtualFunction(const void* instance, const size_t index) -> T {
    return reinterpret_cast<T>(
        (*reinterpret_cast<void***>(const_cast<void*>(instance)))[index]
    );
}
}