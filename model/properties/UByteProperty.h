#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UBytePropertyEntry final : public PropertyEntry, LayoutTraits<UByteProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;
    [[nodiscard]] auto asByteProperty() const -> UByteProperty* { return static_cast<UByteProperty*>(getObject()); }
    [[nodiscard]] auto getCanonicalType() const -> std::string override {
        if (asByteProperty()->Enum) {
            return asByteProperty()->Enum->GetName();
        }
        return "uint8_t";
    }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "UBytePropertyEntry"; }
    [[nodiscard]] auto getDefaultClassName() const -> std::string override { return "UByteProperty"; }
    [[nodiscard]] bool isTriviallyCopyable() const override { return true; }
    [[nodiscard]] bool canConst() const override { return false; }
    [[nodiscard]] auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};