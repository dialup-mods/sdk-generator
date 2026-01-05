#pragma once
#include <cstdint>
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UIntPropertyEntry final : public PropertyEntry, LayoutTraits<UIntProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;
    [[nodiscard]] auto getCanonicalType() const -> std::string override { return "int32_t"; }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "UIntPropertyEntry"; }
    [[nodiscard]] auto getDefaultClassName() const -> std::string override { return "UIntProperty"; }
    [[nodiscard]] auto isTriviallyCopyable() const -> bool override { return true; }
    [[nodiscard]] auto canConst() const -> bool override { return false; }
    [[nodiscard]] auto getSize() const -> ptrdiff_t override { return sizeof(int32_t); }
    [[nodiscard]] auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};