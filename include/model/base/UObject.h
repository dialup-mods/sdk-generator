#pragma once
#include <string>

#include "Object.h"
#include "LayoutTraits.h"

class UObjectEntry final : public ObjectEntry, LayoutTraits<UObject, UObject> {
public:
    using ObjectEntry::ObjectEntry;
    std::string getDefaultClassName() const override { return "UObject"; }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "UObjectEntry"; }
};