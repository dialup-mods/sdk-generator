#pragma once
#include <string>

#include "LayoutTraits.h"
#include "Property.h"

class UDelegatePropertyEntry final : public PropertyEntry, LayoutTraits<UDelegateProperty, UProperty> {
public:
    using PropertyEntry::PropertyEntry;
    [[nodiscard]] auto getCanonicalType() const -> std::string override { return "FScriptDelegate"; }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "UDelegatePropertyEntry"; }
    [[nodiscard]] auto getDefaultClassName() const -> std::string override { return "UDelegateProperty"; }
    [[nodiscard]] bool isTriviallyCopyable() const override { return false; }
    [[nodiscard]] bool canConst() const override { return false; }
    [[nodiscard]] auto getSize() const -> ptrdiff_t override { return sizeof(FScriptDelegate); }
    [[nodiscard]] auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& override { return EMPTY_STR_SET; }
};