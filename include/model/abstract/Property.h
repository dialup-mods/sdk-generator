#pragma once
#include <string>

#include "Object.h"
#include "Schema.h"
#include "Runtime.h"

using r = Runtime;

class PropertyEntry : public ObjectEntry {
public:
    using ObjectEntry::ObjectEntry;

    auto asProperty() const -> UProperty* { return static_cast<UProperty*>(getObject()); }

    auto isArray() const -> bool { return getArrayDim() > 1; }

    virtual auto getSize() const -> ptrdiff_t { return asProperty()->ElementSize; }

    auto getArrayDim() const -> ptrdiff_t { return asProperty()->ArrayDim; }
    auto getElementSize() const -> ptrdiff_t { return asProperty()->ElementSize; }
    auto getOffset() const -> ptrdiff_t { return asProperty()->Offset; }
    auto getTotalSize() const -> ptrdiff_t { return getArrayDim() * getElementSize(); }
    auto getPropertyFlags() const -> uint64_t {
        if (!flags_) { flags_ = asProperty()->PropertyFlags; }
        return flags_;
    }

    auto isValidProperty() const -> bool {
        if (!getObject()) {
            Logger::instance().log("[WARNING] Invalid property {}: null object", getName());
            return false;
        }

        if (!r::types::inheritsFrom(getObject(), r::uclass::find("Class Core.Property"))) {
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

    auto isArgument() const -> bool override { return isValidProperty() && (getPropertyFlags() & CPF_Parm) && !(getPropertyFlags() & CPF_OutParm) && !(getPropertyFlags() & CPF_ReturnParm); }
    auto isOptional() const -> bool override { return isValidProperty() && getPropertyFlags() & CPF_OptionalParm; }
    auto isOutParam() const -> bool override { return isValidProperty() && (getPropertyFlags() & CPF_OutParm) && (getPropertyFlags() & CPF_Parm); }
    auto isReturnParam() const -> bool override { return isValidProperty() && getPropertyFlags() & CPF_ReturnParm; }
    auto isTriviallyCopyable() const -> bool override { return false; }
    virtual auto canConst() const -> bool { return false; }

    // we are at the lowest level, there's no more to iterate
    auto wasIterated() const -> bool override { return true; }

    // dependencies for topo sorting structs
    virtual auto getStructDependencyTypes() const -> const std::unordered_set<std::string>& = 0;

    auto getFormattedArgType() const -> std::string {
        std::string baseType = getCanonicalType();
        // fixme
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