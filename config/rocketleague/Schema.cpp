#include <cassert>

#include "Runtime.h"
#include "SDK.h"
#include "Schema.h"

using r = Runtime;

#pragma pack(push, 0x8)

SDK_API auto
UObject::GetName() const -> std::string {
    return r::uobject_utils::getName(this);
}

SDK_API auto
UObject::GetNameCPP() -> std::string {
    return r::types::getNamePrefix(this) + this->GetName();
}

SDK_API auto
UObject::GetFullName() const -> std::string {
    return r::uobject_utils::getFullName(this);
}

SDK_API auto
UObject::GetPackageObj() const -> UObject* {
    return r::uobject_utils::getPackage(this);
}

SDK_API auto
UObject::HasAnyFlags(EObjectFlags flags) const -> bool {
    return r::uobject_utils::hasAnyFlags(this, flags);
}

SDK_API auto
UObject::HasAllFlags(EObjectFlags flags) const -> bool {
    return r::uobject_utils::hasAllFlags(this, flags);
}

SDK_API auto
FName::ToString() const -> std::string {
    return r::fname::game_pool::getString(FNameEntryId).value_or("<unknown>");
}

SDK_API auto
FName::IsValid() const -> bool {
    if ((FNameEntryId < 0 || FNameEntryId > r::fname::game_pool::ref().size())) {
        return false;
    }
    return true;
}