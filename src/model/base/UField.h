#pragma once
#include <cstddef>
#include <string>

#include "Object.h"
#include "LayoutTraits.h"

class UFieldEntry : public ObjectEntry, LayoutTraits<UField, UObject> {
public:
    using ObjectEntry::ObjectEntry;
    [[nodiscard]] auto getCanonicalType() const -> std::string override { return "UField"; }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "UFieldEntry"; }
    [[nodiscard]] auto getDefaultClassName() const -> std::string override { return "UField"; }
};