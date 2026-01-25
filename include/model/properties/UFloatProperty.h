#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UFloatPropertyEntry final : public PropertyEntry, LayoutTraits<UFloatProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;

    auto getType() const -> EClassTypes override { return EClassTypes::UFloatProperty; }
    auto getCanonicalType() const -> std::string override { return "float"; }
    auto getCacheType() const -> std::string override { return "UFloatPropertyEntry"; }
    auto getDefaultClassName() const -> std::string override { return "UFloatProperty"; }
    bool isTriviallyCopyable() const override { return true; }
    bool canConst() const override { return false; }
    auto getSize() const -> ptrdiff_t override { return sizeof(float); }
    auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};
