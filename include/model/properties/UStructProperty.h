#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"
#include "ObjectStore.h"
#include "UStructProperty.h"

class UStructPropertyEntry final
  : public PropertyEntry, LayoutTraits<UStructProperty, UProperty>
{
  public:
    using PropertyEntry::PropertyEntry;
    auto asStructProperty() const -> UStructProperty* {
        if (auto* structProp = static_cast<UStructProperty*>(getObject())) {
            return structProp;
        }
        return nullptr;
    }

    auto getType() const -> EClassTypes override { return EClassTypes::UStructProperty; }
    auto getCacheType() const -> std::string override { return "UStructPropertyEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UStructProperty"; }

    auto getCanonicalType() const -> std::string override {
        const auto* structProp = static_cast<UStructProperty*>(getObject());
        return structProp->Struct->GetNameCPP();
    }

    auto getEmitType(const std::string& currentPackage) const -> std::string& override {
        const auto* structProperty = static_cast<UStructProperty*>(getObject());
        emitTypeStr_ = structProperty->Struct->GetNameCPP();

        //Logger::instance().log("current package: {}, getPackageName(): {}", currentPackage, getPackageName());
        // fixme, clean up the useless prefixes
        //if (currentPackage != getPackageName()) {
        //    //if (structProperty->Struct->IsA<UClass>()) {
        //    //    emitTypeStr_.insert(0, "class ");
        //    //} else if (structProperty->Struct->IsA<UScriptStruct>()) {
        //    //    emitTypeStr_.insert(0, "struct ");
        //    //}
        //}
        return emitTypeStr_;
    }

    auto isTriviallyCopyable() const -> bool override { return isArray(); }
    auto canConst() const -> bool override { return false; }

    auto getStructEntryAsObjectEntry() const -> ObjectEntry* {
        if (!asStructProperty() || !asStructProperty()->Struct) {
            return nullptr;
        }
        return ObjectStore::instance().add(asStructProperty()->Struct, "fixme");
    }

    // fixme
    auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override {
        if (const auto* structEntry = getStructEntryAsObjectEntry()) {
            dependencyTypes_.insert(structEntry->getFullName());
            return dependencyTypes_;
        }
        Logger::instance().log("[WARNING] some edge case probably");
        return EMPTY_STR_SET;
    }
};