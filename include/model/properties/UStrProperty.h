#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UStrPropertyEntry final : public PropertyEntry, LayoutTraits<UStrProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;

    auto getType() const -> EClassTypes override { return EClassTypes::UStrProperty; }
    auto getCanonicalType() const -> std::string override { return "FString"; }
    auto getCacheType() const -> std::string override { return "UStrPropertyEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UStrProperty"; }
    bool isTriviallyCopyable() const override { return false; }
    bool canConst() const override { return false; }
    auto getSize() const -> ptrdiff_t override { return sizeof(FString); }
    auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};