#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UObjectPropertyEntry : public PropertyEntry, LayoutTraits<UObjectProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;

    [[nodiscard]] auto getEmitType(const std::string& currentPackage) const -> std::string& override {
        emitTypeStr_ = "UObject*";
        const auto* objProperty = static_cast<UObjectProperty*>(getObject());
        if (!objProperty) { return emitTypeStr_; }
        if (!objProperty->PropertyClass) { emitTypeStr_ = "<bad>"; return emitTypeStr_; }

        emitTypeStr_ = objProperty->PropertyClass->GetNameCPP() + "*";

        //Logger::instance().log("uobjproperty current package: {}, getPackageName(): {}", currentPackage, getPackageName());
        // fixme don't emit on known types
        //if (currentPackage != getPackageName()) {
          emitTypeStr_.insert(0, "class ");
        //}
        return emitTypeStr_;
    }

    [[nodiscard]] auto getCanonicalType() const -> std::string override {
        const auto* objProp = static_cast<UObjectProperty*>(getObject());
        if (!objProp || !objProp->PropertyClass) {
            return "UObject*"; // or fallback
        }

        return "class " + objProp->PropertyClass->GetNameCPP() + "*";
        //return objProp->PropertyClass->GetNameCPP() + "*";
    }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "UObjectPropertyEntry"; }
    [[nodiscard]] auto getDefaultClassName() const -> std::string override { return "UObjectProperty"; }
    [[nodiscard]] bool isTriviallyCopyable() const override { return false; }
    [[nodiscard]] bool canConst() const override { return false; }
    [[nodiscard]] auto getSize() const -> ptrdiff_t override { return sizeof(uintptr_t); }
    [[nodiscard]] auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};