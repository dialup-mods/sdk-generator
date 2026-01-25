#pragma once
//
// Interface to the game's runtime data
// kind of has become a dumping ground for things closely related to the engine
//
// but this is object cache, function name cache, class name cache.. and related methods
//
// FNameEntry to string gets a pass because those are baked in
//
// DO NOT make methods like fname::toString..
//
// instead, r::fname::wrap()->str()
//
// wrap() wraps the fname into an FNameEntry (soon to be renamed FNameView.. maybe)
//

#include <map>
#include <optional>
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
class FName;
template<typename T> class TArray;

class ObjectEntry;

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

    struct SDK_API uobject {
        struct SDK_API game_pool {
            static void set(TArray<UObject*>* objs) { uObjects_ = objs; }
            static auto hasUObjects() -> bool { return uObjects_ != nullptr; }
            static auto ptr() -> TArray<UObject*>* { return uObjects_; }
            static auto ref() -> TArray<UObject*>& { return *uObjects_; }
            static auto isPopulated() -> bool;
        };
        struct SDK_API cache {
            static auto ref() -> std::vector<UObject*>& { return uObjectsCache_; }
            static auto rawObjects() -> const std::vector<UObject*>& { return uObjectsCache_; }
        };

        static auto wrap(UObject*) -> ObjectEntry;
        static auto isa() -> bool;
    };

    struct SDK_API fname {
        static std::unordered_map<int32_t, FName> cache_;
        static std::unordered_map<std::wstring, int32_t> name_to_id_;

        struct SDK_API game_pool {
            static void set(TArray<FNameEntry*>* names) { fNameEntries_ = names; }
            static auto ref() -> TArray<FNameEntry*>& { return *fNameEntries_; }
            static auto ptr() -> TArray<FNameEntry*>* { return fNameEntries_; }
            static auto isValid() -> bool;
            static auto find(int32_t) -> std::optional<std::reference_wrapper<const FName>>;
            static auto find(const wchar_t*) -> std::optional<std::reference_wrapper<const FName>>;
            static auto getString(int32_t) -> std::optional<std::string>;
            static auto getWString(int32_t) -> std::optional<std::wstring>;
        };

        struct SDK_API cache {

        };

        static auto wrap(UObject*) -> ObjectEntry;
        static auto toWString(const FName&) -> std::optional<std::wstring>;
        static auto unknown() -> const FName&;
        static auto none() -> const FName&;
        //static auto getBase(int32_t index) -> FNameEntry*;
        //static auto getBaseStr(int32_t index) -> std::string;
    };

    struct SDK_API ufunction {
        struct SDK_API cache {
            static auto ref() -> std::map<std::string, UFunction*>& { return functionCache_; }
            static void add(const std::string& name, UFunction* fn) { functionCache_[name] = fn; }
        };
        static auto find(const std::string& functionFullName) -> UFunction*;
        static auto wrap(UObject*) -> ObjectEntry;
    };

    struct SDK_API uclass {
        struct SDK_API cache {
            static void set(std::map<std::string, UClass*>&& cache) { classCache_ = std::move(cache); }
            static auto ref() -> std::map<std::string, UClass*>& { return classCache_; }
            static void add(const std::string& name, UClass* cls) { classCache_[name] = cls; }
        };
        static auto find(const std::string& classFullName) -> UClass*;
        static auto wrap(UObject*) -> ObjectEntry;
    };

    struct SDK_API process_event {
        static void call(UObject* self, UFunction* function, void* params, void* unusedResult = nullptr);
    };

    struct SDK_API packages {
        static auto findAll() -> std::vector<UObject*>;
    };

    struct SDK_API types {
        static auto knowsClass();
        static auto conformsTo(obj, ClassId::UClass);
        static auto inheritsFrom(obj, ClassId::UClass);
        //if (!knowsClass(className)) {
        //    DIALUP_ASSERT("Unknown class queried; treating as UObject");
        //}
    };
};