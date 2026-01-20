#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UStrPropertyEntry final : public PropertyEntry, LayoutTraits<UStrProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;
    [[nodiscard]] auto getCanonicalType() const -> std::string override { return "FString"; }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "UStrPropertyEntry"; }
    [[nodiscard]] auto getDefaultClassName() const -> std::string override { return "UStrProperty"; }
    [[nodiscard]] bool isTriviallyCopyable() const override { return false; }
    [[nodiscard]] bool canConst() const override { return false; }
    [[nodiscard]] auto getSize() const -> ptrdiff_t override { return sizeof(FString); }
    [[nodiscard]] auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};