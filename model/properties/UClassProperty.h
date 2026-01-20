#pragma once
#include <cstdint>
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UClassPropertyEntry final : public PropertyEntry, LayoutTraits<UClassProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;
    [[nodiscard]] auto getCanonicalType() const -> std::string override { return "UClass*"; }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "UClassPropertyEntry"; }
    [[nodiscard]] auto getDefaultClassName() const -> std::string override { return "UClassProperty"; }
    [[nodiscard]] bool isTriviallyCopyable() const override { return false; }
    [[nodiscard]] bool canConst() const override { return false; }
    [[nodiscard]] auto getSize() const -> ptrdiff_t override { return sizeof(uintptr_t); }
    [[nodiscard]] auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};
