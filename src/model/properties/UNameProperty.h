#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UNamePropertyEntry final : public PropertyEntry, LayoutTraits<UNameProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;
    [[nodiscard]] auto getCanonicalType() const -> std::string override { return "FName"; }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "UNamePropertyEntry"; }
    [[nodiscard]] auto getDefaultClassName() const -> std::string override { return "UNameProperty"; }
    [[nodiscard]] bool isTriviallyCopyable() const override { return false; }
    [[nodiscard]] bool canConst() const override { return false; }
    [[nodiscard]] auto getSize() const -> ptrdiff_t override { return sizeof(FName); }
    [[nodiscard]] auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};