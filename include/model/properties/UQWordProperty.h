#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UQWordPropertyEntry final : public PropertyEntry, LayoutTraits<UInterfaceProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;
    [[nodiscard]] auto getCanonicalType() const -> std::string override { return "uint64_t"; }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "UQWordPropertyEntry"; }
    [[nodiscard]] auto getDefaultClassName() const -> std::string override { return "UQWordProperty"; }
    [[nodiscard]] bool isTriviallyCopyable() const override { return true; }
    [[nodiscard]] bool canConst() const override { return false; }
    [[nodiscard]] auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};