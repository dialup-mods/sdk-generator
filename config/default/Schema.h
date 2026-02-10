#pragma once
#include <map>
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h> // for wide char conversion

#if defined(DIALUP_BUILD)
    #define DIALUP_API __declspec(dllexport)
#else
    #define DIALUP_API __declspec(dllimport)
#endif

// these definitions are used to generate the SDK
// and should match the actual game as closely as possible
// custom overrides for the actual generated SDK output are in Overrides.h

#include "Flags.h"

#pragma once
#include <string>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <Windows.h> // for wide char conversion

#include "Flags.h"

#pragma pack(push, 0x4)

template<typename TArrayType>
class TIterator {
  public:
    using ElementType = typename std::remove_cv_t<TArrayType>::ElementType;
    using ElementPointer = std::conditional_t<std::is_const_v<TArrayType>, const ElementType*, ElementType*>;
    using ElementReference = std::conditional_t<std::is_const_v<TArrayType>, const ElementType&, ElementType&>;
    using ElementConstReference = const ElementType&;

  private:
    ElementPointer IteratorData;

  public:
    explicit TIterator(ElementPointer inElementPointer)
      : IteratorData(inElementPointer) {}
    ~TIterator() = default;

    auto operator++() -> TIterator& {
        ++IteratorData;
        return *this;
    }

    auto operator++(int32_t) -> TIterator {
        TIterator iteratorCopy = *this;
        ++(*this);
        return iteratorCopy;
    }

    auto operator--() -> TIterator& {
        --IteratorData;
        return *this;
    }

    auto operator--(int32_t) -> TIterator {
        TIterator iteratorCopy = *this;
        --(*this);
        return iteratorCopy;
    }

    auto operator[](int32_t index) -> ElementReference { return *(IteratorData[index]); }
    auto operator->() -> ElementPointer { return IteratorData; }
    auto operator*() -> ElementReference { return *IteratorData; }

    auto operator==(const TIterator& other) const -> bool { return (IteratorData == other.IteratorData); }
    auto operator!=(const TIterator& other) const -> bool { return !(*this == other); }
};

template<typename InElementType>
class TArray {
  public:
    using ElementType = InElementType;
    using ElementPointer = ElementType*;
    using ElementReference = ElementType&;
    using ElementConstPointer = const ElementType*;
    using ElementConstReference = const ElementType&;
    using Iterator = TIterator<TArray<ElementType>>;

  private:
    ElementPointer ArrayData;
    int32_t ArrayCount{0};
    int32_t ArrayMax{0};

  public:
    TArray() : ArrayData(nullptr) {}
    ~TArray() = default;

    auto operator[](int32_t index) const -> ElementConstReference { return ArrayData[index]; }
    auto operator[](int32_t index) -> ElementReference { return ArrayData[index]; }
    auto at(int32_t index) const -> ElementConstReference { return ArrayData[index]; }
    auto at(int32_t index) -> ElementReference { return ArrayData[index]; }
    auto data() const -> ElementConstPointer { return ArrayData; }

    void push_back(ElementConstReference newElement) {
        if (ArrayCount >= ArrayMax) {
            ReAllocate(sizeof(ElementType) * (ArrayCount + 1));
        }

        new (&ArrayData[ArrayCount]) ElementType(newElement);
        ArrayCount++;
    }

    void push_back(ElementReference& newElement) {
        if (ArrayCount >= ArrayMax) {
            ReAllocate(sizeof(ElementType) * (ArrayCount + 1));
        }

        new (&ArrayData[ArrayCount]) ElementType(newElement);
        ArrayCount++;
    }

    void pop_back() {
        if (ArrayCount > 0) {
            ArrayCount--;
            ArrayData[ArrayCount].~ElementType();
        }
    }

    void clear() {
        for (int32_t i = 0; i < ArrayCount; i++) {
            ArrayData[i].~ElementType();
        }

        ArrayCount = 0;
    }

    auto size() const -> int32_t { return ArrayCount; }
    auto capacity() const -> int32_t { return ArrayMax; }

    auto empty() const -> bool {
        if (ArrayData) {
            return (size() == 0);
        }
        return true;
    }

    auto begin() -> Iterator { return Iterator(ArrayData); }
    auto end() -> Iterator { return Iterator(ArrayData + ArrayCount); }

  private:
    void ReAllocate(const int32_t newArrayMax) {
        auto newArrayData = static_cast<ElementPointer>(::operator new(newArrayMax * sizeof(ElementType)));
        const int32_t newNum = ArrayCount;
        if (ArrayCount >= ArrayMax) {
            ReAllocate(sizeof(ElementType) * (ArrayCount + 1));
        }
        for (int32_t i = 0; i < newNum; i++) {
            new (newArrayData + i) ElementType(std::move(ArrayData[i]));
        }
        for (int32_t i = 0; i < ArrayCount; i++) {
            ArrayData[i].~ElementType();
        }
        ::operator delete(ArrayData, ArrayMax * sizeof(ElementType));
        ArrayData = newArrayData;
        ArrayMax = newArrayMax;
    }
};

template<typename TKey, typename TValue>
class TMap {
    struct TPair {
        TKey Key;
        TValue Value;
        int32_t* HashNext; // should be added to DialUp
    };

  public:
    using ElementType = TPair;
    using ElementPointer = ElementType*;
    using ElementReference = ElementType&;
    using ElementConstReference = const ElementType&;
    using Iterator = TIterator<TArray<ElementType>>;

    TArray<ElementType> Elements; // 0x0000 (0x0010)
    uintptr_t IndirectData;       // 0x0010 (0x0008)
    int32_t InlineData[0x4];      // 0x0018 (0x0010) NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)
    int32_t NumBits;              // 0x0028 (0x0004)
    int32_t MaxBits;              // 0x002C (0x0004)
    int32_t FirstFreeIndex;       // 0x0030 (0x0004)
    int32_t NumFreeIndices;       // 0x0034 (0x0004)
    int64_t InlineHash;           // 0x0038 (0x0008)
    int32_t* Hash;                // 0x0040 (0x0008)
    int32_t HashCount;            // 0x0048 (0x0004)

    TMap()
      : IndirectData(NULL)
      , NumBits(0)
      , MaxBits(0)
      , FirstFreeIndex(0)
      , NumFreeIndices(0)
      , InlineHash(0)
      , Hash(nullptr)
      , HashCount(0) {}

    TMap(struct FMap_Mirror& other)
      : IndirectData(NULL)
      , NumBits(0)
      , MaxBits(0)
      , FirstFreeIndex(0)
      , NumFreeIndices(0)
      , InlineHash(0)
      , Hash(nullptr)
      , HashCount(0) {
        assign(other);
    }

    TMap(const TMap& other)
      : IndirectData(NULL)
      , NumBits(0)
      , MaxBits(0)
      , FirstFreeIndex(0)
      , NumFreeIndices(0)
      , InlineHash(0)
      , Hash(nullptr)
      , HashCount(0) {
        assign(other);
    }

    ~TMap() = default;

    auto assign(FMap_Mirror& other) -> TMap& {
        *this = *reinterpret_cast<TMap*>(&other); // NOLINT(*-pro-type-reinterpret-cast)
        return *this;
    }

    auto assign(const TMap& other) -> TMap& {
        Elements = other.Elements;
        IndirectData = other.IndirectData;
        InlineData[0] = other.InlineData[0];
        InlineData[1] = other.InlineData[1];
        InlineData[2] = other.InlineData[2];
        InlineData[3] = other.InlineData[3];
        NumBits = other.NumBits;
        MaxBits = other.MaxBits;
        FirstFreeIndex = other.FirstFreeIndex;
        NumFreeIndices = other.NumFreeIndices;
        InlineHash = other.InlineHash;
        Hash = other.Hash;
        HashCount = other.HashCount;
        return *this;
    }

    auto at(const TKey& key) -> TValue& {
        for (TPair& pair : Elements) {
            if (pair.Key == key) {
                return pair.Value;
            }
        }
        return {};
    }

    auto at(const TKey& key) const -> const TValue& {
        for (const TPair& pair : Elements) {
            if (pair.Key == key) {
                return pair.Value;
            }
        }
        return {};
    }

    auto at_index(int32_t index) -> TPair& { return Elements[index]; }
    auto at_index(int32_t index) const -> const TPair& { return Elements[index]; }
    auto size() const -> int32_t { return Elements.size(); }
    auto capacity() const -> int32_t { return Elements.capacity(); }
    auto empty() const -> bool { return Elements.empty(); }
    auto begin() -> Iterator { return Elements.begin(); }
    auto end() -> Iterator { return Elements.end(); }

    auto operator[](const TKey& key) -> TValue& { return at(key); }
    auto operator[](const TKey& key) const -> const TValue& { return at(key); }
    auto operator=(const FMap_Mirror& other) -> TMap& { assign(other); return *this; }
    auto operator=(const TMap& other) -> TMap& { assign(other); return *this; }
};

extern TArray<class UObject*>* GObjects;
extern TArray<class FNameEntry*>* GNames;

class FNameEntry {
  public:
    uint64_t Flags{0};
    int32_t Index{-1};
    uint32_t Pad0C{}; // for alignment on 64-bit
    FNameEntry* HashNext{};
    wchar_t Name[0x400]{ {} }; // NOLINT(*-avoid-magic-numbers)

    FNameEntry() {}

    ~FNameEntry() = default;

    [[nodiscard]] auto GetFlags() const -> uint64_t { return Flags; }
    [[nodiscard]] auto GetIndex() const -> int32_t { return Index; }
    [[nodiscard]] auto GetWideName() const -> const wchar_t* { return &Name[0]; }
    [[nodiscard]] auto ToWideString() const -> std::wstring {
        return GetWideName();
    }

    auto ToString() const -> std::string {
        std::wstring wstr = ToWideString();
        if (wstr.empty())
            return {};

        const int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        std::string str(sizeNeeded, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), str.data(), sizeNeeded, nullptr, nullptr);
        return str;
    }
};

class FName {
  public:
    using ElementType = const wchar_t;
    using ElementPointer = ElementType*;

  private:
    int32_t FNameEntryId;
    int32_t InstanceNumber;

  public:
    FName()
      : FNameEntryId(-1)
      , InstanceNumber(0) {}

    explicit FName(const int32_t id)
      : FNameEntryId(id)
      , InstanceNumber(0) {}

    explicit FName(const ElementPointer nameToFind)
      : FNameEntryId(-1)
      , InstanceNumber(0) {
        static std::vector<int32_t> foundNames{};

        for (const int32_t entryId : foundNames) {
            if (Names()->at(entryId) && wcscmp(&Names()->at(entryId)->Name[0], nameToFind) == 0) {
                FNameEntryId = entryId;
                return;
            }
        }

        for (int32_t i = 0; i < Names()->size(); i++) {
            if (Names()->at(i) && wcscmp(&Names()->at(i)->Name[0], nameToFind) == 0) {
                foundNames.push_back(i);
                FNameEntryId = i;
                return;
            }
        }
    }

    FName(const FName& name) = default;
    ~FName() = default;

    static auto Names() -> TArray<FNameEntry*>*;
    auto GetDisplayIndex() const -> int32_t { return FNameEntryId; }

    auto GetDisplayNameEntry() const -> const FNameEntry {
        if (IsValid()) {
            return *Names()->at(FNameEntryId);
        }
        return FNameEntry();
    }

    auto GetEntry() const -> FNameEntry* {
        if (IsValid()) {
            return Names()->at(FNameEntryId);
        }
        return nullptr;
    }

    [[nodiscard]] auto GetInstance() const -> int32_t { return InstanceNumber; }
    void SetInstance(const int32_t newNumber) { InstanceNumber = newNumber; }

    [[nodiscard]] auto ToString() const -> std::string {
        if (IsValid()) {
            return GetDisplayNameEntry().ToString();
        }
        return "UnknownName";
    }

    [[nodiscard]] auto IsValid() const -> bool {
        if ((FNameEntryId < 0 || FNameEntryId > Names()->size())) {
            return false;
        }
        return true;
    }

    auto operator=(const FName& other) -> FName& = default;
    auto operator==(const FName& other) const -> bool {
        return ((FNameEntryId == other.FNameEntryId) && (InstanceNumber == other.InstanceNumber));
    }
    auto operator!=(const FName& other) const -> bool { return !(*this == other); }
};

class FString {
  public:
    using ElementType = const wchar_t;
    using ElementPointer = ElementType*;

  private:
    ElementPointer ArrayData{};
    int32_t ArrayCount{};
    int32_t ArrayMax{};

  public:
    FString() = default;
    explicit FString(const ElementPointer other) {
        assign(other);
    }
    ~FString() = default;

    auto assign(const ElementPointer other) -> FString& {
        ArrayCount = other ? (1 + static_cast<int32_t>(wcslen(other))) : 0;
        ArrayMax = ArrayCount;
        ArrayData = (ArrayCount > 0 ? other : nullptr);
        return *this;
    }

    auto ToWideString() const -> std::wstring {
        if (!empty()) {
            return c_str();
        }
        return L"";
    }

    auto ToString() const -> std::string {
        std::wstring wstr = ToWideString();
        if (wstr.empty()) return {};

        int size_needed = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), nullptr, 0, nullptr, nullptr);
        std::string str(size_needed, 0);
        WideCharToMultiByte(CP_UTF8, 0, wstr.data(), static_cast<int>(wstr.size()), str.data(), size_needed, nullptr, nullptr);
        return str;
    }

    auto c_str() const -> ElementPointer { return ArrayData; }

    auto empty() const -> bool {
        if (ArrayData) {
            return (ArrayCount == 0);
        }
        return true;
    }

    [[nodiscard]] auto length() const -> int32_t { return ArrayCount; }
    [[nodiscard]] auto size() const -> int32_t { return ArrayMax; }

    auto operator=(const ElementPointer other) -> FString& { assign(other); return *this; }
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
struct FScriptDelegate {
    UObject* Object;
    uint8_t UnknownData[0x10]; // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)
    //FName FunctionName;
};

struct FPointer {
    uintptr_t Dummy;
};

struct FQWord {
    int32_t A;
    int32_t B;
};

class UObject {
  public:
    FPointer VfTableObject{};
    FPointer HashNext{};
    int64_t ObjectFlags{};
    FPointer HashOuterNext{};
    FPointer StateFrame{};
    UObject* Linker{};
    FPointer LinkerIndex{};
    int32_t ObjectInternalInteger{};
    int32_t NetIndex{};
    UObject* Outer{};
    FName Name{};
    class UClass* Class{};
    UObject* ObjectArchetype{};

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.Object");
        return uClassPointer;
    }

    static auto GObjObjects() -> TArray<UObject*>*;

    auto GetName() const -> std::string;
    auto GetNameCPP() -> std::string;
    auto GetFullName() const -> std::string;
    auto GetPackageObj() const -> UObject*;
    template<typename T>
    static auto FindObject(const std::string& objectFullName) -> T* {
        for (UObject* uObject : *GObjObjects()) {
            if (uObject && uObject->IsA(T::StaticClass())) {
                if (uObject->GetFullName() == objectFullName) {
                    return reinterpret_cast<T*>(uObject); // NOLINT(*-pro-type-reinterpret-cast)
                }
            }
        }
        return nullptr;
    }
    static auto FindClass(const std::string& classFullName) -> UClass*;
    auto IsA(const UClass* uClass) const -> bool;
    auto IsA(int32_t objInternalInteger) const -> bool;

    template<typename T>
    auto IsA() -> bool {
        if (std::is_base_of_v<UObject, T>) {
            return IsA(T::StaticClass());
        }
        return false;
    }

    // GENERATED_METHODS_UOBJECT

};

class UField : public UObject {
  public:
    UField* Next{nullptr};
    uint8_t UnknownData00[0x8]{}; // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.Field");
        return uClassPointer;
    };
};

class UEnum : public UField {
  public:
    TArray<FName> Names{};

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.Enum");
        return uClassPointer;
    };
};

class UConst : public UField {
  public:
    FString Value{};

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.Const");
        return uClassPointer;
    };
};

class UProperty : public UField {
  public:
    int32_t ArrayDim{};
    int32_t ElementSize{};
    uint64_t PropertyFlags{};
    uint8_t UnknownData00[0x10]{}; // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)
    uint32_t PropertySize{};
    uint8_t UnknownData01[0x4]{}; // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)
    int32_t Offset{};
    uint8_t UnknownData02[0x2C]{}; // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.Property");
        return uClassPointer;
    };
};

class UStruct : public UField {
  public:
    uint8_t UnknownData00[0x10]{}; // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)
    UField* SuperField{nullptr};
    UField* Children{nullptr};
    int32_t PropertySize{};
    int32_t MinAlignment{};
    uint8_t UnknownData01[0x98]{}; // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.Struct");
        return uClassPointer;
    };
};

class UFunction : public UStruct {
  public:
    uint64_t FunctionFlags{};
    uint16_t iNative{};
    uint16_t RepOffset{};
    FName FriendlyName{};
    uint8_t OperatorPrecedence{};
    uint8_t NumParms{};
    uint16_t ParmsSize{};
    unsigned long ReturnValueOffset{};
    uint8_t UnknownData00[0xC]{}; // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)
    FPointer Func{};

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.Function");
        return uClassPointer;
    };

    static auto FindFunction(const std::string& functionFullName) -> UFunction*;
};

class UScriptStruct : public UStruct {
  public:
    uint8_t UnknownData00[0x28]{}; // [USE THIS CLASSES PROPERTYSIZE IN RECLASS TO DETERMINE THE SIZE OF THE UNKNOWNDATA] // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.ScriptStruct");
        return uClassPointer;
    };
};

class UState : public UStruct {
  public:
    uint8_t UnknownData00[0x60]{}; // [USE THIS CLASSES PROPERTYSIZE IN RECLASS TO DETERMINE THE SIZE OF THE UNKNOWNDATA] // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)

  public:
    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.State");
        return uClassPointer;
    };
};

class UClass : public UState {
  public:
    uint8_t UnknownData00[0x228]{}; // [USE THIS CLASSES PROPERTYSIZE IN RECLASS TO DETERMINE THE SIZE OF THE UNKNOWNDATA] // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.Class");
        return uClassPointer;
    };
};

class UStructProperty : public UProperty {
  public:
    UStruct* Struct{};

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.StructProperty");
        return uClassPointer;
    };
};

class UStrProperty : public UProperty {
  public:
    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.StrProperty");
        return uClassPointer;
    };
};

class UQWordProperty : public UProperty {
  public:
    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.QWordProperty");
        return uClassPointer;
    };
};

class UObjectProperty : public UProperty {
  public:
    UClass* PropertyClass{nullptr};
    uint8_t UnknownData00[0x8]{}; // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.ObjectProperty");
        return uClassPointer;
    };
};

class UClassProperty : public UObjectProperty {
  public:
    UClass* MetaClass{nullptr};

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.ClassProperty");
        return uClassPointer;
    };
};

class UComponentProperty : public UObjectProperty {
  public:
    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.ComponentProperty");
        return uClassPointer;
    };
};

class UNameProperty : public UProperty {
  public:
    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.NameProperty");
        return uClassPointer;
    };
};

class UMapProperty : public UProperty {
  public:
    UProperty* Key{nullptr};
    UProperty* Value{nullptr};

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.MapProperty");
        return uClassPointer;
    };
};

class UIntProperty : public UProperty {
  public:
    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.IntProperty");
        return uClassPointer;
    };
};

class UInterfaceProperty : public UProperty {
  public:
    UClass* InterfaceClass{nullptr};
    uint8_t UnknownData00[0x8]{}; // NOLINT(*-avoid-c-arrays, *-avoid-magic-numbers)

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.InterfaceProperty");
        return uClassPointer;
    };
};

class UFloatProperty : public UProperty {
  public:
    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.FloatProperty");
        return uClassPointer;
    };
};

class UDelegateProperty : public UProperty {
  public:
    UFunction* DelegateFunction{};
    UFunction* SourceDelegate{};

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.DelegateProperty");
        return uClassPointer;
    };
};

class UByteProperty : public UProperty {
  public:
    UEnum* Enum{nullptr};

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.ByteProperty");
        return uClassPointer;
    };
};

class UBoolProperty : public UProperty {
  public:
    uint64_t BitMask {};

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.BoolProperty");
        return uClassPointer;
    };
};

class UArrayProperty : public UProperty {
  public:
    UProperty* Inner{};

    static auto StaticClass() -> UClass* {
        static UClass* uClassPointer = FindClass("Class Core.ArrayProperty");
        return uClassPointer;
    };
};

#pragma pack(pop)

// inlined to break circular dependencies

inline TArray<UObject*>*
UObject::GObjObjects() {
    return GObjects;
}

inline std::string
UObject::GetName() const {
    return this->Name.ToString();
}

inline std::string
UObject::GetNameCPP() {
    std::string nameCPP;

    if (this->IsA<UClass>()) {
        auto uClass = reinterpret_cast<UClass*>(this);

        while (uClass) {
            if (std::string className = uClass->GetName(); className == "Actor") {
                nameCPP += "A";
                break;
            } else if (className == "Object") {
                nameCPP += "U";
                break;
            }

            uClass = reinterpret_cast<UClass*>(uClass->SuperField);
        }
    } else {
        nameCPP += "F";
    }

    nameCPP += this->GetName();
    return nameCPP;
}

inline std::string
UObject::GetFullName() const {
    std::string fullName = this->GetName();

    for (const UObject* uOuter = this->Outer; uOuter; uOuter = uOuter->Outer) {
        fullName = (uOuter->GetName() + "." + fullName);
    }

    fullName = (this->Class->GetName() + " " + fullName);
    return fullName;
}

inline UObject*
UObject::GetPackageObj() const {
    UObject* uPackage = nullptr;

    for (UObject* uOuter = this->Outer; uOuter; uOuter = uOuter->Outer) {
        uPackage = uOuter;
    }

    return uPackage;
}

inline UClass*
UObject::FindClass(const std::string& classFullName) {
    static std::map<std::string, UClass*> classCache;

    if (classCache.empty()) {
        for (int32_t i = 0; i < GObjObjects()->size() - 1; i++) {
            if (UObject* uObject = GObjObjects()->at(i)) {
                if (std::string objectFullName = uObject->GetFullName(); objectFullName.find("Class") == 0) {
                    classCache[objectFullName] = reinterpret_cast<UClass*>(uObject);
                }
            }
        }
    }

    if (classCache.contains(classFullName)) {
        return classCache[classFullName];
    }

    return nullptr;
}

inline bool
UObject::IsA(const UClass* uClass) const {
    if (uClass) {
        for (const UClass* uSuperClass = reinterpret_cast<UClass*>(this->Class); uSuperClass;
            uSuperClass = reinterpret_cast<UClass*>(uSuperClass->SuperField)) {
            if (uSuperClass == uClass) {
                return true;
            }
        }
    }

    return false;
}

inline auto
UObject::IsA(const int32_t objInternalInteger) const -> bool {
    if (const UClass* uClass = GObjObjects()->at(objInternalInteger)->Class) {
        return this->IsA(uClass);
    }
    return false;
}

inline UFunction*
UFunction::FindFunction(const std::string& functionFullName) {
    static std::map<std::string, UFunction*> functionCache;

    if (functionCache.empty()) {
        for (int32_t i = 0; i < GObjObjects()->size() - 1; i++) {
            if (UObject* uObject = GObjObjects()->at(i)) {
                if (std::string objectFullName = uObject->GetFullName(); objectFullName.find("Function") == 0) {
                    functionCache[objectFullName] = reinterpret_cast<UFunction*>(uObject);
                }
            }
        }
    }

    if (functionCache.contains(functionFullName)) {
        return functionCache[functionFullName];
    }

    return nullptr;
}

inline TArray<FNameEntry*>*
FName::Names() {
    return GNames;
}

// seriously fuck globals
inline TArray<UObject*>* GObjects{};
inline TArray<FNameEntry*>* GNames{};