#include <memory>
#include <optional>

#include "fmt/format.h"

#include "EClassTypes.h"
#include "Logger.h"
#include "RuntimeGen.h"
#include "Schema.h"
#include "TypeRules.h"
#include "WaveWorker.h"

#include "Object.h"
#include "UClass.h"
#include "UConst.h"
#include "UEnum.h"
#include "UObject.h"
#include "UScriptStruct.h"

#include "UArrayProperty.h"
#include "UBoolProperty.h"
#include "UByteProperty.h"
#include "UClassProperty.h"
#include "UDelegateProperty.h"
#include "UFloatProperty.h"
#include "UFunction.h"
#include "UIntProperty.h"
#include "UInterfaceProperty.h"
#include "UMapProperty.h"
#include "UNameProperty.h"
#include "UObjectProperty.h"
#include "UQWordProperty.h"
#include "UStrProperty.h"
#include "UStructProperty.h"

ObjectStore* ObjectStore::instance_ = nullptr;

ObjectStore::ObjectStore() = default;

auto
ObjectStore::isProbablyValidPtr(const uintptr_t ptr) -> bool {
    if (!ptr) {
        storeInvalidObject(ptr, InvalidUObjectReason::NullObject);
    }

    if (!(ptr > 0x10000 && ptr < 0x7FFFFFFFFFFF)) { // NOLINT(*-avoid-magic-numbers)
        storeInvalidObject(ptr, InvalidUObjectReason::InvalidAddress);
        return false;
    }
    return true;
}

void
ObjectStore::storeInvalidObject(UObject* rawObj, const InvalidUObjectReason reason) {
    invalidObjects_[reinterpret_cast<uintptr_t>(rawObj)] = reason;
}

void
ObjectStore::storeInvalidObject(uintptr_t ptr, const InvalidUObjectReason reason) {
    invalidObjects_[ptr] = reason;
}

auto
ObjectStore::isProbablyValidUObject(UObject* rawObj) -> bool {
    auto addr = reinterpret_cast<uintptr_t>(rawObj);
    if (!isProbablyValidPtr(addr)) {
        return false;
    }

    if (rawObj->Outer && !isProbablyValidPtr(reinterpret_cast<uintptr_t>(rawObj->Outer))) {
        storeInvalidObject(rawObj, InvalidUObjectReason::InvalidOuterAddress);
        return false;
    }

    if (!rawObj->Class) {
        storeInvalidObject(rawObj, InvalidUObjectReason::NullClass);
        return false;
    }

    if (static_cast<std::string_view>(rawObj->Name.ToString()).find("Default__") != std::string::npos) {
        storeInvalidObject(rawObj, InvalidUObjectReason::DefaultObject);
        return false;
    }

    return true;
}

auto
ObjectStore::getTotalGObjObjectsCount() const -> size_t {
    return Runtime::instance().getRawObjects().size();
}

auto
ObjectStore::getTotalSeenCount() const -> size_t {
    return seen_.size();
}

auto
ObjectStore::getInvalidCount() const -> size_t {
    return invalidPtrs_.size();
}

void ObjectStore::iterateObjects(const std::function<bool(UObject*)>& fn) {
    for (UObject* obj : Runtime::getUObjects()) {
        if (!obj) continue;
        if (!fn(obj)) break;
    }
}

void
ObjectStore::initialize() {
    int i = 0;
    WaveWorker wave(std::chrono::milliseconds(30));
    wave.start();

    iterateObjects([&](UObject* raw) {

        // if it's invalid, go ahead and add the pointer to the `seen` list
        // so we never try to turn it into a cache entry, and instead
        // short-circuit the `add()` logic
        if (!isProbablyValidPtr(reinterpret_cast<uintptr_t>(raw))) {
            seen_.insert(raw);
            i++;
            return true; // continue to next iteration
        }

        try {
            add(raw, "GObjObjects");
        } catch (const std::exception& e) {
            Logger::log("[ERROR] Exception in add() at index {}: {}", i, e.what());
        } catch (...) {
            Logger::log("[ERROR] Unknown error in add() at index {}", i);
        }

        i++;
        return true;
    });
    wave.stop();
}

auto ObjectStore::existingEntryFor(const UObject* obj) const -> ObjectEntry* {
    if (!obj) return nullptr;
    for (const auto& entry : all_) {
        if (entry && entry->getObject() == obj) {
            return entry.get();
        }
    }
    return nullptr;
}

//bool isTransientOrJunk(UObject* obj) {
//    return obj->HasAnyFlags(RF_Transient | RF_DefaultSubObject) ||
//           obj->GetName().Contains("__Default");
//}

//auto ObjectStore::shouldReplace(UObject* existing, UObject* candidate) -> bool {
//    return false;
//}

//bool shouldReplace(ObjectEntry* existing, UObject* newObj) {
//    // Create the new entry type to compare
//    auto newEntry = createEntry(newObj);
//
//    // Prefer more specific types
//    if (typeid(*existing) == typeid(ObjectEntry) &&
//        typeid(*newEntry) != typeid(ObjectEntry)) {
//        return true; // Replace generic with specific
//    }
//
//    // Or use some kind of type hierarchy ranking
//    return getTypeSpecificity(newEntry) > getTypeSpecificity(existing);
//}

//void ObjectStore::maybeUpgradeUFunction(ObjectEntry* entry, UObject* rawObj) {
//    if (auto* existingEntry = objectToEntry_[rawObj]) {
//        // Is it a UFunctionEntry?
//        if (auto* existingFunc = dynamic_cast<UFunctionEntry*>(existingEntry)) {
//            // Create a temporary one to compare
//            UFunctionEntry temp(rawObj);
//
//            if (temp.argumentCount() > existingFunc->argumentCount()) {
//                Logger::log("Upgrading UFunctionEntry {} from {} → {} args",
//                            temp.getFunctionName(),
//                            existingFunc->argumentCount(),
//                            temp.argumentCount());
//
//                auto newEntry = std::make_unique<UFunctionEntry>(rawObj);
//                objectToEntry_[rawObj] = newEntry.get();
//                all_.emplace_back(std::move(newEntry));
//                return objectToEntry_[rawObj];
//            }
//
//            return existingFunc;
//        }
//
//        return existingEntry;
//    }
//}



// add an object to the store and return the resulting ObjectEntry*
// returns ObjectEntry* from object store if it exists
// early returns nullptr if the ObjectEntry can't be created (invalid)
// early returns nullptr if the address has been seen before but no valid entry exists
auto ObjectStore::add(UObject* rawObj, const std::string& origin = "") -> ObjectEntry* {
    //if (!isValid(rawObj)) {
    //    Logger::log("⚠️ Skipping object with null class from origin: " + origin);
    //    return nullptr;
    //}
    if (seen_.contains(rawObj)) {
        auto* seenEntry = objectToEntry_[rawObj];

//        if (auto* seenFunc = dynamic_cast<UFunctionEntry*>(seenEntry)) {
//            UFunctionEntry temp(rawObj);
//            if (temp.getArguments().size() > seenFunc->getArguments().size()) {
//                Logger::log("Upgrading UFunctionEntry {} from {} to {} args",
//                            temp.getFunctionName(),
//                            seenFunc->getArguments().size(),
//                            temp.getArguments().size());
//
//                auto upgraded = std::make_unique<UFunctionEntry>(rawObj);
//                all_.emplace_back(std::move(upgraded));
//                ObjectEntry* ptr = all_.back().get();
//                objectToEntry_[rawObj] = ptr;
//                return ptr;
//            }
//        }

        // seenEntry->addReferrer(origin);
        return seenEntry;
    }

    seen_.insert(rawObj);

    if (!isProbablyValidUObject(rawObj)) {
        seen_.insert(rawObj);
        return nullptr;
    }

    std::unique_ptr<ObjectEntry> entry;

    //Logger::log("class: " + rawObj->Class->GetFullName() + "\n  origin: " + origin);
    //Logger::log("UClass::StaticClass(): " + UClass::StaticClass()->GetName());
    //Logger::log("UScriptStruct::StaticClass(): " + UScriptStruct::StaticClass()->GetName());
    if (rawObj->IsA(UArrayProperty::StaticClass())) {
        entry = std::make_unique<UArrayPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UStrProperty::StaticClass())) {
        entry = std::make_unique<UStrPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UIntProperty::StaticClass())) {
        entry = std::make_unique<UIntPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UFloatProperty::StaticClass())) {
        entry = std::make_unique<UFloatPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UDelegateProperty::StaticClass())) {
        entry = std::make_unique<UDelegatePropertyEntry>(rawObj);
    } else if (rawObj->IsA(UNameProperty::StaticClass())) {
        entry = std::make_unique<UNamePropertyEntry>(rawObj);
    } else if (rawObj->IsA(UStructProperty::StaticClass())) {
        entry = std::make_unique<UStructPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UClassProperty::StaticClass())) {
        entry = std::make_unique<UClassPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UObjectProperty::StaticClass())) {
        entry = std::make_unique<UObjectPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UMapProperty::StaticClass())) {
        entry = std::make_unique<UMapPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UInterfaceProperty::StaticClass())) {
        entry = std::make_unique<UInterfacePropertyEntry>(rawObj);
    } else if (rawObj->IsA(UQWordProperty::StaticClass())) {
        entry = std::make_unique<UQWordPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UBoolProperty::StaticClass())) {
        entry = std::make_unique<UBoolPropertyEntry>(rawObj);
    } else if (rawObj->IsA(UByteProperty::StaticClass())) {
        entry = std::make_unique<UBytePropertyEntry>(rawObj);
    } else if (rawObj->IsA(UEnum::StaticClass())) {
        entry = std::make_unique<EnumEntry>(rawObj);
    } else if (rawObj->IsA(UClass::StaticClass())) {
        entry = std::make_unique<ClassEntry>(rawObj);
        nameToEntry_[rawObj->Name.ToString()] = entry.get();
    } else if (rawObj->IsA(UFunction::StaticClass())) {
        entry = std::make_unique<UFunctionEntry>(rawObj);
    } else if (rawObj->IsA(UScriptStruct::StaticClass())) {
        entry = std::make_unique<UScriptStructEntry>(rawObj);
        // fixme getFullName is empty
        nameToStruct_[entry->getFullName()] = entry.get();
    } else if (rawObj->IsA(UState::StaticClass())) {
        //Logger::log("[WARN] UState. Not yet implemented. {}", entry->getFullName());
        return nullptr;
        // not implemented yet
        //entry = std::make_unique<UState>(rawObj);
    } else if (rawObj->IsA(UStruct::StaticClass())) {
        Logger::log("[WARN] Raw UStruct.");
        entry = std::make_unique<UStructEntry>(rawObj);
        nameToStruct_[entry->getFullName()] = entry.get();
    } else if (rawObj->IsA(UConst::StaticClass())) {
        entry = std::make_unique<ConstEntry>(rawObj);
        nameToStruct_[entry->getFullName()] = entry.get();
    } else if (rawObj->IsA(UObject::StaticClass())) {
        entry = std::make_unique<UObjectEntry>(rawObj);
    } else {
        entry = std::make_unique<ObjectEntry>(rawObj);
        // fixme getFullName will crash on invalid elements
        Logger::log("[WARN] Using fallback type for: {}", entry->getFullName());
    }

    entry->setOrigin(origin);
    //Logger::log("entry: " + entry->asString());

    if (!entry->isValid()) {
        // fixme getFullName will crash on invalid elements
        Logger::log("[WARN] Invalid object detected: {}", entry->getFullName());
        return nullptr;
    }

    // populate cached attrs
    // note: may be needed in case object dies mid-run
    //Logger::instance().log("{}", entry->asString());

    all_.emplace_back(std::move(entry));

    ObjectEntry* ptr = all_.back().get();
    objectToEntry_[rawObj] = ptr;


    return ptr;
}

auto ObjectStore::getStructByName(const std::string& given) -> UScriptStructEntry* {
    for (auto& [storedName, entry] : nameToStruct_) {
        if (storedName == given) {
            return static_cast<UScriptStructEntry*>(entry);
        }
    }
    return nullptr;
}

auto ObjectStore::getAllStructEntries() -> std::vector<UScriptStructEntry*> {
    std::vector<UScriptStructEntry*> result;
    for (auto& entry : instance().getAll()) {
        if (entry->getType() == EClassTypes::UScriptStruct) {
            result.emplace_back(static_cast<UScriptStructEntry*>(entry.get()));
        }
        //else if (entry->getType() == EClassTypes::UStruct) {
        //    Logger::log("[ERROR] skipping UStruct, {}", entry->getFullName());
        //}
    }
    return result;
}

auto ObjectStore::getAllEnumEntries() -> std::vector<EnumEntry*> {
    std::vector<EnumEntry*> result;
    for (auto& entry : instance().getAll()) {
        if (entry->getType() == EClassTypes::UEnum) {
            result.emplace_back(static_cast<EnumEntry*>(entry.get()));
        }
    }
    return result;
}

auto ObjectStore::getAllClassEntries() -> std::vector<ClassEntry*> {
    std::vector<ClassEntry*> result;
    for (auto& entry : instance().getAll()) {
        if (entry->getType() == EClassTypes::UClass) {
            result.emplace_back(static_cast<ClassEntry*>(entry.get()));
        }
    }
    return result;
}

auto ObjectStore::getAllConstEntries() -> std::vector<ConstEntry*> {
    std::vector<ConstEntry*> result;
    for (auto& entry : instance().getAll()) {
        if (entry->getType() == EClassTypes::UConst) {
            result.emplace_back(static_cast<ConstEntry*>(entry.get()));
        }
    }
    return result;
}

auto ObjectStore::getAllFunctionEntries() -> std::vector<UFunctionEntry*> {
    std::vector<UFunctionEntry*> result;
    for (auto& entry : instance().getAll()) {
        if (entry->getType() == EClassTypes::UFunction) {
            result.emplace_back(static_cast<UFunctionEntry*>(entry.get()));
        }
    }
    return result;
}

auto
ObjectStore::countStructsWithName(const std::string_view sanitizedName) -> int {
    // fixme performance -- maybe use the structs map
    return std::ranges::count_if(all_, [&](const std::unique_ptr<ObjectEntry>& entry) {
        return entry->getSanitizedName() == sanitizedName && entry->getType() == EClassTypes::UStruct;
    });
}

// ReSharper disable once CppParameterMayBeConstPtrOrRef
auto ObjectStore::get(UObject* obj) const -> ObjectEntry* {
    if (!obj) return nullptr;
    auto it = objectToEntry_.find(obj);
    return it != objectToEntry_.end() ? it->second : nullptr;
}

auto ObjectStore::getClassEntryByName(std::string& name) -> ClassEntry* {
    for (auto& entry : instance().getAll()) {
        if (entry->getType() == EClassTypes::UClass) {
            return static_cast<ClassEntry*>(entry.get());
        }
    }
    return nullptr;
}