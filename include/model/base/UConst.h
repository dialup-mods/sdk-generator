#pragma once
#include <string>

#include "fmt/format.h"

#include "Object.h"
#include "LayoutTraits.h"

class ConstEntry final : public ObjectEntry, LayoutTraits<UConst, UField> {
public:
    using ObjectEntry::ObjectEntry;

    [[nodiscard]] auto getCanonicalType() const -> std::string override { return "UConst"; }
    [[nodiscard]] auto getCacheType() const -> std::string override { return "ConstEntry"; }
    [[nodiscard]] auto getDefaultClassName() const -> std::string override { return "UConst"; }

    [[nodiscard]] auto asConst() const -> UConst* {
        return static_cast<UConst*>(getObject());
    }

    void emit(FILE* file, const std::string& package) override {
        fmt::print(file
            , "#define {} {}\n"
            , getNameCPP()
            , asConst()->Value.ToString()
        );
    };
};