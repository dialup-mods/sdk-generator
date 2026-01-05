#include <cassert>
#include "Runtime.h"
#include "SDK.h"
#include "Schema.h"

class UObject;

#pragma pack(push, 0x8)

void SDK_API UObject::ProcessEvent(class UFunction* uFunction, void* uParams, void* uResult) {
	sdk_internal::getVirtualFunction<void(*)(class UObject*, class UFunction*, void*)>(this, 67)(this, uFunction, uParams);
}

std::string SDK_API UObject::GetName() const {
    return this->Name.ToString();
}

std::string SDK_API UObject::GetNameCPP() {
    std::string nameCPP;

    if (this->IsA<UClass>()) {
        auto uClass = reinterpret_cast<UClass*>(this);

        while (uClass) {
            if (std::string className = uClass->GetName(); className == "Actor") {
                nameCPP += "A";
                break;
            } else if (className == "Object") {
                nameCPP += "U";
                break;
            }

            uClass = reinterpret_cast<UClass*>(uClass->SuperField);
        }
    } else {
        nameCPP += "F";
    }

    nameCPP += this->GetName();
    return nameCPP;
}

std::string SDK_API UObject::GetFullName() const {
    std::string fullName = this->GetName();

    for (const UObject* uOuter = this->Outer; uOuter; uOuter = uOuter->Outer) {
        fullName = (uOuter->GetName() + "." + fullName);
    }

    fullName = (this->Class->GetName() + " " + fullName);
    return fullName;
}

auto SDK_API UObject::GetPackageObj() const -> UObject* {
    UObject* uPackage = nullptr;

    for (UObject* uOuter = this->Outer; uOuter; uOuter = uOuter->Outer) {
        uPackage = uOuter;
    }
    return uPackage;
}

bool SDK_API UObject::IsA(const UClass* uClass) const {
    if (uClass) {
        for (const UClass* uSuperClass = reinterpret_cast<UClass*>(this->Class); uSuperClass;
            uSuperClass = reinterpret_cast<UClass*>(uSuperClass->SuperField)) {
            if (uSuperClass == uClass) {
                return true;
            }
            }
    }
    return false;
}

auto SDK_API UObject::IsA(const int32_t objInternalInteger) const -> bool {
    if (const UClass* uClass = Runtime::getUObjects().at(objInternalInteger)->Class) {
        return this->IsA(uClass);
    }
    return false;
}

bool SDK_API UObject::HasAnyFlags(EObjectFlags Flags) const {
    return (ObjectFlags & Flags) != 0;
}

bool SDK_API UObject::HasAllFlags(EObjectFlags Flags) const {
    return (ObjectFlags & Flags) == Flags;
}