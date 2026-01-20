#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UFloatPropertyEntry final : public PropertyEntry, LayoutTraits<UFloatProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;
    [[nodiscard]] auto getCanonicalType() const -> std::string override { return "float"; }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "UFloatPropertyEntry"; }
    [[nodiscard]] auto getDefaultClassName() const -> std::string override { return "UFloatProperty"; }
    [[nodiscard]] bool isTriviallyCopyable() const override { return true; }
    [[nodiscard]] bool canConst() const override { return false; }
    [[nodiscard]] auto getSize() const -> ptrdiff_t override { return sizeof(float); }
    [[nodiscard]] auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};
