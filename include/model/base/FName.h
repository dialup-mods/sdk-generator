#pragma once
#include <cstddef>
#include <string>

#include "Object.h"
#include "LayoutTraits.h"

class FNameEntryEntry: public ObjectEntry, LayoutTraits<FNameEntry, FName> {
public:
    using ObjectEntry::ObjectEntry;

    auto getDefaultClassName() const -> std::string override { return "FNameEntry"; }
    auto getCanonicalType() const -> std::string override { return "FNameEntry"; }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "FNameEntry"; }
};