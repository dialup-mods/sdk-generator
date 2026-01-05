#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UBoolPropertyEntry final : public PropertyEntry, LayoutTraits<UBoolProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;
    [[nodiscard]] auto getCanonicalType() const -> std::string override { return "uint32_t"; }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "UBoolPropertyEntry"; }
    [[nodiscard]] auto getDefaultClassName() const -> std::string override { return "UBoolProperty"; }
    [[nodiscard]] bool isTriviallyCopyable() const override { return false; }
    [[nodiscard]] bool canConst() const override { return false; }
    [[nodiscard]] auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};