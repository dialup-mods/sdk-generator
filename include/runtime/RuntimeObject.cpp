#include "Runtime.h"
#include <functional>
using r = Runtime;

void Runtime::uobject::cache::buildClassNameCacheFromCDOs() {
    for (const UObject* obj : game_pool::ref()) {
        if (!obj) continue;

        if (obj->ObjectFlags & RF_ClassDefaultObject) {
            if (UClass* cls = obj->Class) {
                classNameToClass_.emplace(
                    cls->GetFullName(),
                    cls
                );
            }
        }
    }
}

void Runtime::uobject::cache::populateClassToCDO() {
    for (UObject* obj : game_pool::ref()) {
        if (!obj) continue;

        if (obj->ObjectFlags & RF_ClassDefaultObject) {
            if (obj->Class) {
                classToCDO_[obj->Class] = obj;
            }
        }
    }
}

auto Runtime::uobject::cache::findClassViaCDO(const std::function<bool(const UObject*)> &cdoPredicate) -> UClass* {
    for (const UObject* obj : game_pool::ref()) {
        if (!obj) continue;

        if (!(obj->ObjectFlags & RF_ClassDefaultObject))
            continue;

        // This is the class default object
        UClass* cls = obj->Class;
        if (!cls) continue;

        if (cdoPredicate(obj)) {
            return cls;
        }
    }
    return nullptr;
}

auto Runtime::uobject::cache::getClassNameToClassCache() -> ClassNameToClassCache {
    return classNameToClass_;
}

auto Runtime::uobject::resolveClass(const std::string_view className) -> UClass* {
    auto it = classNameToClass_.find(className);
    if (it != classNameToClass_.end())
        return it->second;

    return nullptr;
}

auto Runtime::uobject::getFirst(const std::string_view className) -> UObject* {
    const UClass* wantedClass = uclass::find(className);

    const auto& objects = game_pool::ref();
    for (int i = objects.size(); i-- > 0; ) {
        UObject* uObject = objects.at(i);
        if (!uObject) { continue; }
        if (uObject->ObjectFlags & RF_DefaultOrArchetypeFlags) { continue; }
        if (types::isa(uObject->Class, wantedClass)) {
            return uObject;
        }
    }
    return nullptr;
}

auto Runtime::uobject::getAll(const std::string_view className) -> std::vector<UObject*> {
    const auto* wantedClass = uclass::find(className);
    if (!wantedClass) {
        printf("Given class name is unknown. Cannot proceed with lookup");
        return {};
    }

    std::vector<UObject*> matchingObjs;
    const auto& objects = game_pool::ref();
    for (int i = objects.size(); i-- > 0; ) {
        UObject* uObject = objects.at(i);
        if (!uObject) { continue; }
        if (uObject->ObjectFlags & RF_DefaultOrArchetypeFlags) { continue; }
        if (types::isa(uObject->Class, wantedClass)) {
            matchingObjs.emplace_back(uObject);
        }
    }
    return matchingObjs;
}