#pragma once
#include <string>

#include "Object.h"
#include "Schema.h"

class PropertyEntry : public ObjectEntry {
public:
    using ObjectEntry::ObjectEntry;

    [[nodiscard]] auto asProperty() const -> UProperty* { return static_cast<UProperty*>(getObject()); }

    [[nodiscard]] auto isArray() const -> bool { return getArrayDim() > 1; }

    [[nodiscard]] virtual auto getSize() const -> ptrdiff_t { return asProperty()->ElementSize; }

    [[nodiscard]] auto getArrayDim() const -> ptrdiff_t { return asProperty()->ArrayDim; }
    [[nodiscard]] auto getElementSize() const -> ptrdiff_t { return asProperty()->ElementSize; }
    [[nodiscard]] auto getOffset() const -> ptrdiff_t { return asProperty()->Offset; }
    [[nodiscard]] auto getTotalSize() const -> ptrdiff_t { return getArrayDim() * getElementSize(); }
    [[nodiscard]] auto getPropertyFlags() const -> uint64_t {
        if (!flags_) { flags_ = asProperty()->PropertyFlags; }
        return flags_;
    }

    auto isValidProperty() const -> bool {
        if (!getObject()) {
            Logger::instance().log("[WARNING] Invalid property {}: null object", getName());
            return false;
        }

        if (!getObject()->IsA(UProperty::StaticClass())) {
            Logger::instance().log("[WARNING] Invalid property {}: not a UProperty", getName());
            return false;
        }

        if (!getElementSize()) {
            Logger::instance().log("[WARNING] Invalid property {}: zero element size", getName());
            return false;
        }

        if (getType() == EClassTypes::Unknown) {
            Logger::instance().log("[WARNING] Invalid property {}: unknown type - {}", getName(), getObject()->Class->GetName());
            return false;
        }

        //Logger::instance().log("[INFO] Valid property: {} (type: {})", getName(), static_cast<int>(getType()));
        return true;
    }

    // Some real layout-affecting properties can have:
    // ElementSize == 0
    // bitfields
    // bool-packed flags
    // engine-internal padding props
    // They still move offsets forward and therefore define memory, even if they don’t generate fields.
    // So for inheritance eligibility, element size must NOT be a gate.
    auto definesLayoutPast(ptrdiff_t parentSize) const -> bool {
        if (!getObject()) return false;
        if (!getObject()->IsA(UProperty::StaticClass())) return false;

        return getOffset() >= parentSize;
    }

    [[nodiscard]] auto isArgument() const -> bool override { return isValidProperty() && (getPropertyFlags() & CPF_Parm) && !(getPropertyFlags() & CPF_OutParm) && !(getPropertyFlags() & CPF_ReturnParm); }
    [[nodiscard]] auto isOptional() const -> bool override { return isValidProperty() && getPropertyFlags() & CPF_OptionalParm; }
    [[nodiscard]] auto isOutParam() const -> bool override { return isValidProperty() && (getPropertyFlags() & CPF_OutParm) && (getPropertyFlags() & CPF_Parm); }
    [[nodiscard]] auto isReturnParam() const -> bool override { return isValidProperty() && getPropertyFlags() & CPF_ReturnParm; }
    [[nodiscard]] auto isTriviallyCopyable() const -> bool override { return false; }
    [[nodiscard]] virtual auto canConst() const -> bool { return false; }

    // we are at the lowest level, there's no more to iterate
    [[nodiscard]] auto wasIterated() const -> bool override { return true; }

    // dependencies for topo sorting structs
    [[nodiscard]] virtual auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& = 0;

    // UFunction args
    [[nodiscard]] auto getFormattedArgType() const -> std::string {
        std::string baseType = getCanonicalType();
        if (getCanonicalType() == "SearchStatusOwner") {
            baseType = "ESearchStatusOwner";
        }
        if (isReturnParam()) { return baseType; }
        if (isOutParam()) { return baseType + "&"; }
        if (canConst()) { return "const " + baseType + "&"; }
        return baseType;
    }

protected:
    mutable std::unordered_set<std::string> dependencyTypes_;

private:
    mutable uint64_t flags_{0};
};