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

class ObjectEntry;

#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "SDK.h"
#include "Schema.h"

#if defined(SDK_BUILD)
#define SDK_API __declspec(dllexport)
#else
#define SDK_API __declspec(dllimport)
#endif

struct TransparentHash {
    using is_transparent = void;
    size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
};

struct TransparentEq {
    using is_transparent = void;
    bool operator()(std::string_view a, std::string_view b) const noexcept {
        return a == b;
    }
};

#include "Flags.h"

using ClassCache = std::unordered_map<
        std::string,
        UClass*,
        TransparentHash,
        TransparentEq
>;

class SDK_API Runtime {
    static Runtime* instance_;

    static TArray<FNameEntry*>* fNameEntries_;
    static TArray<UObject*>* uObjects_;

    static ClassCache classCache_;

    static std::map<std::string, UFunction*> functionCache_;
    static std::vector<UObject*> uObjectsCache_;

public:
    Runtime() = default;
    ~Runtime() = default;

    static void create();
    static auto instance() -> Runtime&;
    static void yeet();

    static auto getName(UObject* obj) -> std::string;

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

        template <typename T>
        static auto classOf(UObject*) -> UClass* {
            static UClass* cached = [] {
                UClass* cls = uclass::find(T::className);
                if (!cls) {
                    printf("can't find\n");
                }
                return cls;
            }();
            return cached;
        };

        #ifdef false
        template<typename T>
        static auto getInstanceOf() -> T* {
            static_assert(std::is_base_of_v<UObject, T>);

            UClass* wanted = classOf<T>();
            if (!wanted) return nullptr;

            for (UObject* obj : game_pool::ref()) {
                if (!obj) continue;
                if (obj->ObjectFlags & RF_DefaultOrArchetypeFlags) continue;

                if (obj->Class == wanted) {
                    return static_cast<T*>(obj);
                }
            }
            return nullptr;
        }

        static auto isSubclassOf(const UObject* obj, const UClass* base) -> bool {
            // SuperField is UClass* here by invariant
            for (UClass* cls = obj->Class; cls; cls = static_cast<UClass*>(cls->SuperField)) {
                if (cls == base) return true;
            }
            return false;
        }
        #endif

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
            static auto find(const FName& wanted) -> std::optional<std::reference_wrapper<const FName>>;
            static auto getString(int32_t) -> std::optional<std::string>;
            static auto getWString(int32_t) -> std::optional<std::wstring>;
            static auto getString(const FName&) -> std::optional<std::string>;
            static auto getWString(const FName&) -> std::optional<std::wstring>;
        };

        struct SDK_API cache {

        };

        static auto wrap(UObject*) -> ObjectEntry;
        static auto toString(const FName&) -> std::optional<std::string>;
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
            static void set(ClassCache cache) { classCache_ = std::move(cache); }
            static auto ref() -> ClassCache& { return classCache_; }
        };
        static auto find(std::string_view classFullName) -> UClass*;
        static auto wrap(UObject*) -> ObjectEntry;
    };

    struct SDK_API process_event {
        static void call(UObject* self, UFunction* function, void* params, void* unusedResult = nullptr);
    };

    struct SDK_API packages {
        static auto findAll() -> std::vector<UObject*>;
    };

    struct SDK_API types {
        static auto isa(const UClass* given, const UClass* other) -> bool;
        static auto knowsClass(const UObject* obj) -> bool;
        static auto conformsTo(const UObject* obj, const UClass* targetClass) -> bool;
        static auto inheritsFrom(UObject* obj, UClass* targetClass) -> bool;
        static auto getNamePrefix(UObject* obj) -> std::string;
        // if (!knowsClass(className)) {
        //    DIALUP_ASSERT("Unknown class queried; treating as UObject");
        //}
    };

    struct SDK_API inheritance_cache {
        // Map<ChildClass, Set<ParentClasses>>
        static inline std::unordered_map<UClass*, std::unordered_set<UClass*>> cache_;

        static auto inheritsFrom(UClass* child, UClass* parent) -> bool;
    };

    struct SDK_API uobject_utils {
        static auto getName(const UObject* obj) -> std::string;
        static auto getNameCPP(UObject* obj) -> std::string;
        static auto getFullName(const UObject* obj) -> std::string;
        static auto getPackage(const UObject* obj) -> UObject*;
        static auto hasAnyFlags(const UObject* obj, EObjectFlags flags) -> bool;
        static auto hasAllFlags(const UObject* obj, EObjectFlags flags) -> bool;
    };
};