#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UBytePropertyEntry final : public PropertyEntry, LayoutTraits<UByteProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;

    auto asByteProperty() const -> UByteProperty* { return static_cast<UByteProperty*>(getObject()); }

    auto getType() const -> EClassTypes override { return EClassTypes::UByteProperty; }
    auto getCanonicalType() const -> std::string override {
        if (asByteProperty()->Enum) {
            return asByteProperty()->Enum->GetName();
        }
        return "uint8_t";
    }
    auto getCacheType() const -> std::string override { return "UBytePropertyEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UByteProperty"; }
    bool isTriviallyCopyable() const override { return true; }
    bool canConst() const override { return false; }
    auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};