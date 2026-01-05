//
// Schema.h - Unreal Engine type definitions
//
// See docs/SCHEMA.md for design philosophy and decorator usage.
//
#pragma once
#include <Windows.h> // for wide char conversion
#include <string>

#include "Runtime.h"
#include "Flags.h"

#if defined(SDK_BUILD)
    #define SDK_API __declspec(dllexport)
#else
    #define SDK_API __declspec(dllimport)
#endif

#pragma pack(push, 0x4)

/// @final
template<typename T>
class TIterator {
    T* IteratorData;

public:
    explicit TIterator(T* inElementPointer)
        : IteratorData(inElementPointer) {}

    TIterator& operator++() {
        ++IteratorData;
        return *this;
    }

    TIterator operator++(int) {
        TIterator copy = *this;
        ++IteratorData;
        return copy;
    }

    auto operator->() -> T* { return IteratorData; }
    auto operator*() -> T& { return *IteratorData; }

    auto operator==(const TIterator& other) const -> bool { return (*this == other); }
    auto operator!=(const TIterator& other) const -> bool { return !(*this == other); }
};

// alias for convenience
// usage:
//   TSimpleIterator<int> it;
/// @final
template<typename T>
using TSimpleIterator = TIterator<TArray<T>>;

// Safe wrapper that prevents accidental copies
/// @final
template<typename T>
class TArrayView {
    T* Data;
    int32_t Count;

public:
    TArrayView(T* data, int32_t count) : Data(data), Count(count) {}

    // Delete copy/move - force const&
    TArrayView(const TArrayView&) = delete;
    TArrayView& operator=(const TArrayView&) = delete;

    // Iteration
    T* begin() { return Data; }
    T* end() { return Data + Count; }
    const T* begin() const { return Data; }
    const T* end() const { return Data + Count; }

    // Access
    T& operator[](int32_t i) { return Data[i]; }
    const T& operator[](int32_t i) const { return Data[i]; }

    int32_t size() const { return Count; }
    bool empty() const { return Count == 0; }
};

/// @final
template<typename T>
class TArray {
    /**
     * TArray - Game memory wrapper
     *
     * IMPORTANT: TArrays reference game-managed memory.
     * - DO NOT copy TArray objects manually
     * - Use const auto& in range-based for loops
     * - Use .view() for safer iteration
     *
     * Examples:
     *   ✓ for (const auto& item : array) { }
     *   ✓ for (const auto& item : array.view()) { }
     *   ✗ for (auto item : array) { }  // WILL CRASH
     */
private:
    T* ArrayData{};
    int32_t ArrayCount{};
    int32_t ArrayMax{};

public:
    TArray() = default;
    ~TArray() = default;  // does NOTHING

    // Must allow copy for generated classes
    TArray(const TArray&) = default;
    TArray& operator=(const TArray&) = default;

    // Safe iteration via view
    TArrayView<T> view() { return TArrayView<T>(ArrayData, ArrayCount); }
    TArrayView<const T> view() const { return TArrayView<const T>(ArrayData, ArrayCount); }

    auto operator[](int32_t i) -> T& { return ArrayData[i]; }
    auto operator[](int32_t i) const -> const T& { return ArrayData[i]; }

    auto data() -> T* { return ArrayData; }
    auto data() const -> T* { return ArrayData; }
    auto size() const -> int32_t { return ArrayCount; }
    auto capacity() const -> int32_t { return ArrayMax; }
    auto empty() const -> bool { return ArrayCount == 0; }

    auto at(int32_t i) -> T& { return (*this)[i]; }
    auto at(int32_t i) const -> const T& { return (*this)[i]; }
    bool isValidIndex(const int32_t index) const {
        return index >= 0 && index < size();
    }

    T* begin() { return data(); }
    T* end() { return data() + size(); }
    const T* begin() const { return data(); }
    const T* end() const { return data() + size(); }
};

// view only
/// @final
template<typename TKey, typename TValue>
class SDK_API TMap {
public:
    struct TPair {
        TKey Key;
        TValue Value;
        int32_t* HashNext;
    };

    using Iterator = TIterator<TPair>;

    // Public so you can read game's TMap layout
    TArray<TPair> Elements;
    uintptr_t IndirectData;
    int32_t InlineData[4];
    int32_t NumBits;
    int32_t MaxBits;
    int32_t FirstFreeIndex;
    int32_t NumFreeIndices;
    int64_t InlineHash;
    int32_t* Hash;
    int32_t HashCount;

    TMap() = default;

    // Read-only operations
    const TValue* find(const TKey& key) const {
        for (const TPair& pair : Elements) {
            if (pair.Key == key) {
                return &pair.Value;
            }
        }
        return nullptr;
    }

    const TPair& at_index(int32_t index) const {
        return Elements[index];
    }

    int32_t size() const { return Elements.size(); }
    bool empty() const { return Elements.empty(); }

    Iterator begin() { return Elements.begin(); }
    Iterator end() { return Elements.end(); }

    // No operator[] - use find() instead
    // No modifications - read-only for game maps
};

/// @final
class SDK_API FNameEntry {
  public:
    uint64_t Flags{0};
    int32_t Index{-1};
    FNameEntry* HashNext{};
    wchar_t Name[0x400]{ {} }; // NOLINT(*-avoid-magic-numbers)

    FNameEntry() = default;
    ~FNameEntry() = default;

    auto GetFlags() const -> uint64_t { return Flags; }
    auto GetIndex() const -> int32_t { return Index; }
    auto GetWideName() const -> const wchar_t* { return &Name[0]; }
    [[nodiscard]] auto ToWideString() const -> std::wstring {
        return GetWideName();
    }

    [[nodiscard]] auto ToString() const -> std::string {
        const std::wstring wstr = ToWideString();
        if (wstr.empty())
            return {};

        const int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), str.data(), sizeNeeded, nullptr, nullptr);
        return str;
    }
};

/// @final
class SDK_API FName {
    int32_t FNameEntryId{-1};
    int32_t InstanceNumber{0};

  public:
    FName() = default;

    auto operator=(const FName& other) -> FName& = default;
    auto operator==(const FName& other) const -> bool {
        return ((FNameEntryId == other.FNameEntryId) && (InstanceNumber == other.InstanceNumber));
    }
    auto operator!=(const FName& other) const -> bool { return !(*this == other); }

    // no constructors
    // use Runtime::getFNameEntry()
    // FNames are baked into the game
    // They are not modifiable
    // The engine can't/won't use plugin-defined FNames
    // DO NOT construct copies of game-owned objects with FName

    auto ToString() const -> std::string {
        return Runtime::getFNameEntryName(FNameEntryId);
    }

    auto IsValid() const -> bool {
        if ((FNameEntryId < 0 || FNameEntryId > Runtime::getFNameEntriesPtr()->size())) {
            return false;
        }
        return true;
    }
};

/// @final
class SDK_API FString {
    const wchar_t* ArrayData{};
    int32_t ArrayCount{};
    int32_t ArrayMax{};

  public:
    FString() = default;
    explicit FString(const wchar_t* other) {
        assign(other);
    }
    ~FString() = default;

    auto assign(const wchar_t* other) -> FString& {
        ArrayCount = other ? (1 + static_cast<int32_t>(wcslen(other))) : 0;
        ArrayMax = ArrayCount;
        ArrayData = (ArrayCount > 0 ? other : nullptr);
        return *this;
    }

    [[nodiscard]] auto ToWideString() const -> std::wstring {
        if (!empty()) {
            return c_str();
        }
        return L"";
    }

    [[nodiscard]] auto ToString() const -> std::string {
        std::wstring wstr = ToWideString();
        if (wstr.empty()) return {};

        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        std::string str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), str.data(), size_needed, nullptr, nullptr);
        return str;
    }

    auto c_str() const -> const wchar_t* { return ArrayData; }

    [[nodiscard]] auto empty() const -> bool {
        if (ArrayData) {
            return (ArrayCount == 0);
        }
        return true;
    }

    auto length() const -> int32_t { return ArrayCount; }
    auto size() const -> int32_t { return ArrayMax; }

    auto operator=(const wchar_t* other) -> FString& { assign(other); return *this; }
    auto operator=(const FString& other) -> FString& { assign(other.c_str()); return *this; }

    auto operator==(const FString& other) const -> bool {
        if (ArrayData == other.ArrayData) { return true; }
        if (!ArrayData || !other.ArrayData) { return false; }
        return (wcscmp(ArrayData, other.ArrayData) == 0);
    }

    auto operator!=(const FString& other) const -> bool {
        if (ArrayData == other.ArrayData) { return false; }
        if (!ArrayData || !other.ArrayData) { return true; }
        return (wcscmp(ArrayData, other.ArrayData) != 0);
    }
};

// THIS STRUCT CAN BE GAME SPECIFIC
/// @final
struct SDK_API FScriptDelegate {
    UObject* Object;
    FName FunctionName;
};

/// @final
struct SDK_API FPointer {
    void* Ptr;
    explicit operator bool() const {
        return Ptr != nullptr;
    }
};

/// @final
struct SDK_API FQWord {
    int32_t A;
    int32_t B;
};

class SDK_API UObject {
  public:
    FPointer VfTableObject{};
    uint8_t Pad{};
    int32_t ObjectInternalInteger{};
    UObject* Outer{};
    FName Name{};
    class UClass* Class{};
    int64_t ObjectFlags{};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.Object"); }

    /// @inject-methods
    auto HasAnyFlags(EObjectFlags Flags) const -> bool;
    auto HasAllFlags(EObjectFlags Flags) const -> bool;

    auto GetName() const -> std::string;
    auto GetNameCPP() -> std::string;
    auto GetFullName() const -> std::string;
    auto GetPackageObj() const -> UObject*;

    auto IsA(const UClass* uClass) const -> bool;
    auto IsA(int32_t objInternalInteger) const -> bool;

    template<typename T>
    auto IsA() -> bool {
        if (std::is_base_of_v<UObject, T>) {
            return IsA(T::StaticClass());
        }
        return false;
    }
    /// @end-inject

    void ProcessEvent(UFunction* function, void* params, void* result = nullptr);
};

/// @replace
class SDK_API UField : public UObject {
  public:
    UField* Next{nullptr};
    UField* SuperField{nullptr};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.Field"); }
};

/// @replace
class SDK_API UEnum : public UField {
  public:
    TArray<FName> Names{};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.Enum"); }
};

/// @replace
class SDK_API UConst : public UField {
  public:
    FString Value{};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.Const"); }
};

/// @replace
class SDK_API UProperty : public UField {
  public:
    int32_t ArrayDim{};
    int32_t ElementSize{};
    uint64_t PropertyFlags{};
    int32_t Offset{};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.Property"); }
};

/// @replace
class SDK_API UStruct : public UField {
  public:
    UField* SuperField{nullptr};
    UField* Children{nullptr};
    int32_t PropertySize{};
    int32_t MinAlignment{};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.Struct"); }
};

/// @replace
class SDK_API UFunction : public UStruct {
  public:
    uint64_t FunctionFlags{};
    uint16_t iNative{};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.Function"); }

    static auto FindFunction(const std::string& functionFullName) -> UFunction*;
};

/// @replace
class SDK_API UScriptStruct : public UStruct {
  public:
    uint8_t UnknownData00[0x01]{}; // [USE THIS CLASSES PROPERTYSIZE IN RECLASS TO DETERMINE THE SIZE OF THE UNKNOWNDATA] // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.ScriptStruct"); }
};

/// @replace
class SDK_API UState : public UStruct {
  public:
    uint8_t UnknownData00[0x01]{}; // [USE THIS CLASSES PROPERTYSIZE IN RECLASS TO DETERMINE THE SIZE OF THE UNKNOWNDATA] // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.State"); }
};

/// @replace
class SDK_API UClass : public UState {
  public:
    uint8_t UnknownData00[0x01]{};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.Class"); }
};

/// @replace
class SDK_API UStructProperty : public UProperty {
  public:
    UStruct* Struct{};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.StructProperty"); }
};

/// @replace
class SDK_API UStrProperty : public UProperty {
  public:
    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.StrProperty"); }
};

/// @replace
class SDK_API UQWordProperty : public UProperty {
  public:
    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.QWordProperty"); }
};

/// @replace
class SDK_API UObjectProperty : public UProperty {
  public:
    UClass* PropertyClass{nullptr};
    uint8_t UnknownData00[0x8]{}; // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.ObjectProperty"); }
};

/// @replace
class SDK_API UClassProperty : public UObjectProperty {
  public:
    UClass* MetaClass{nullptr};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.ClassProperty"); }
};

/// @replace
class SDK_API UComponentProperty : public UObjectProperty {
  public:
    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.ComponentProperty"); }
};

/// @replace
class SDK_API UNameProperty : public UProperty {
  public:
    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.NameProperty"); }
};

/// @replace
class SDK_API UMapProperty : public UProperty {
  public:
    UProperty* Key{nullptr};
    UProperty* Value{nullptr};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.MapProperty"); }
};

/// @replace
class SDK_API UIntProperty : public UProperty {
  public:
    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.IntProperty"); }
};

/// @replace
class SDK_API UInterfaceProperty : public UProperty {
  public:
    UClass* InterfaceClass{nullptr};
    uint8_t UnknownData00[0x8]{}; // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.InterfaceProperty"); }
};

/// @replace
class SDK_API UFloatProperty : public UProperty {
  public:
    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.FloatProperty"); }
};

/// @replace
class SDK_API UDelegateProperty : public UProperty {
  public:
    UFunction* DelegateFunction{};
    UFunction* SourceDelegate{};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.DelegateProperty"); }
};

/// @replace
class SDK_API UByteProperty : public UProperty {
  public:
    UEnum* Enum{nullptr};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.ByteProperty"); }
};

/// @replace
class SDK_API UBoolProperty : public UProperty {
  public:
    uint32_t BitMask{}; // could be uint32_t or uint64_t

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.BoolProperty"); }
};

/// @replace
class SDK_API UArrayProperty : public UProperty {
  public:
    UProperty* Inner{};

    static auto StaticClass() -> UClass* { return Runtime::findClass("Class Core.ArrayProperty"); }
};

#pragma pack(pop)