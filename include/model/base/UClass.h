#pragma once
#include <string>
#include <vector>
#include <map>

#include "fmt/format.h"

#include "Object.h"
#include "ObjectStore.h"
#include "Property.h"
#include "StructLike.h"
#include "UFunction.h"
#include "UStruct.h"
#include "UStructProperty.h"

class ClassEntry final : public StructLikeEntry {
public:
    explicit ClassEntry(UObject* obj) : StructLikeEntry(obj) {
        iterateDependencies();
        iterateProperties();
    }

    auto getCanonicalType() const -> std::string override { return "class"; }

    auto getCacheType() const -> std::string override { return "ClassEntry"; }
    auto getOffset() const -> ptrdiff_t { return hasSuperField() ? getSuperFieldPropertySize() : 0; }

    auto getSuperFieldAsClass() const -> UClass* {
        return static_cast<UClass*>(asStruct()->SuperField);
    }

    bool isUObjectDerived() const {
        for (UStruct* cur = asStruct(); cur; cur = static_cast<UStruct*>(cur->SuperField)) {
            if (cur == UObjectClass()) {
                return true;
            }
        }
        return false;
    }

    auto resolveCppBase() const -> UClass* {
        UClass* super = getSuperFieldAsClass();

        if (super && super != asClass()) {
            return super;
        }

        if (isUObjectDerived()) {
            return UObjectClass();
        }

        return nullptr;
    }

    auto getSuperFieldPropertySize() const -> ptrdiff_t {
        if (asClass()->SuperField) {
            if (const auto entry = ObjectStore::instance().add(asClass()->SuperField, getFullName())->as<StructLikeEntry>();
                entry->isValid()) {
                return entry->getPropertySize();
            }
        }
        return 0;
    }

    auto superClassSize() const -> ptrdiff_t {
        const ptrdiff_t uSuperClassSize = getPropertySize() - getSuperFieldPropertySize();
        return uSuperClassSize;
    }

    auto getSuperFieldFullName() const -> std::string {
        if (auto* superFieldEntry = ObjectStore::instance().add(asClass()->SuperField, getFullName())) {
            return superFieldEntry->getFullName();
        }
        return "";
    };

    auto getMinAlignment() const -> ptrdiff_t override { return asStruct() ? asStruct()->MinAlignment : 0; }
    auto getPropertySize() const -> ptrdiff_t override {
        return asStruct() ? asStruct()->PropertySize : 0;
    }

    void walkStructDependencies(UStruct* s, const std::string& origin) {
        if (!s) return;

        auto* classEntry = ObjectStore::instance().add(s, origin)->as<UStructEntry>();
        if (!classEntry) return;

        for (UField* child = s->Children; child; child = child->Next) {
            auto* entry = ObjectStore::instance().add(child, classEntry->getFullName());

            if (auto* sp = entry->as<UStructPropertyEntry>()) {
                if (auto* targetStruct = sp->getStructEntryAsObjectEntry()) {
                    walkStructDependencies(static_cast<UStruct*>(targetStruct->getObject()), classEntry->getFullName());
                }
            }
            else if (auto* p = entry->as<PropertyEntry>()) {
                if (p->isValidProperty()) {
                    properties_.insert(p);
                }
            }
        }
    }

     void iterateDependencies() override {
        const UClass* uClass = asClass();
        if (!uClass) return;

        for (UField* field = uClass->Children; field; field = field->Next) {
            if (field->IsA<UProperty>()) {
                if (auto* cached = ObjectStore::instance().add(field, getFullName())->as<PropertyEntry>()) {
                    if (cached->isValidProperty()) {
                        //Logger::instance().log("adding property {} to class {}", cached->getFullName(), getFullName());
                        properties_.insert(cached);
                        continue;
                    }
                }
            }

            if (field->IsA<UFunction>()) {
                if (auto* cached = ObjectStore::instance().add(field, getFullName())->as<UFunctionEntry>()) {
                    //Logger::instance().log("adding method {} to class {}", cached->getFullName(), getFullName());
                    methods_.insert(cached);
                    continue;
                }
            }

            auto* cached = ObjectStore::instance().add(field, getFullName());
            //Logger::instance().log("orphan {} (type: {}) found in {}", field->Name.ToString(), field->Class->GetName(), getFullName());
            orphans_.insert(cached);
        }
    }

    auto getSortedMethods() const -> std::vector<UFunctionEntry*> {
        std::vector out(methods_.begin(), methods_.end());
        std::ranges::sort(out, [](auto* a, auto* b) {
            return a->getName() < b->getName();
        });
        return out;
    }

    auto getMethodNames() const -> std::unordered_set<std::string> {
        std::unordered_set<std::string> out;
        for (const auto* fn : methods_) {
            out.insert(fn->getNameCPP());
        }
        return out;
    }

    void emit(FILE* file, const std::string& package) override {};

    void emitClassSignature(FILE* file) const {
        // inheritance eligibility depends solely on:
        //   ~ Does the parent define memory? ~

        if (UClass* base = resolveCppBase(); base && base != asClass()) {
            // annoying mode size/offset comment
            //fprintf(file, "// 0x%04X (0x%04llX - 0x%04llX)\n",
            //    static_cast<int>(uSuperClassSize)
            //    , getSuperFieldPropertySize()
            //    , getPropertySize()
            //);

            // signature for derived class
            fprintf(file, "class SDK_API %s : public %s {\n",
                getNameCPP().c_str(),
                ObjectStore::instance()
                    .add(base, getFullName())
                    ->getNameCPP()
                    .c_str()
            );
        } else {
            // annoying mode size/offset comment
            //fprintf(file, "// 0x%04X\n", static_cast<int>(getPropertySize()));
            //fprintf(file, "class DIALUP_API %s {\n", classNameCPP.c_str());

            fprintf(file, "class SDK_API %s {\n", getNameCPP().c_str());
        }
        fprintf(file, "  public:\n");
    }

    void emitClassName(FILE* file) const {
        fmt::print(file, "    static constexpr auto className = \"{}\";\n", getFullName());
    }

    void emitProperties(FILE* file) const {
        ptrdiff_t cursorPosition = getOffset();
        uint32_t unknownDataIndex = 0;
        std::map<std::string, uint32_t> nameMap;
        std::map<std::string, uint32_t> bitfieldMap;
        std::vector<std::string> outputLines;

        const auto sortedProperties = getSortedProperties();

        if (!sortedProperties.empty()) {
            fputs("\n", file);
        }

        for (const PropertyEntry* prop : sortedProperties) {
            if (!prop->isValidProperty()) continue;

            auto propName = prop->getSanitizedName();
            const auto arrayDim = prop->getArrayDim();
            const auto type = prop->getCanonicalType();
            const auto flagStr = FlagStrings::getInterestingPropertyFlagsString(prop->getPropertyFlags());

            // insert pre-padding if needed
            const auto offset = prop->getOffset();
            if (cursorPosition < offset) {
                if (const auto padSize = offset - cursorPosition; padSize >= getGameAlignment()) {
                    outputLines.push_back(createUnknownDataFieldStr(unknownDataIndex++, cursorPosition, padSize, "MISSED OFFSET"));
                }
            }

            if (arrayDim <= 1) {
                if (nameMap.contains(propName)) {
                    propName += std::to_string(nameMap[propName]++);
                } else {
                    nameMap[propName] = 1;
                }
            }
            //else {
            //    Logger::instance().log("{} {}[{}];\n", type, propName, arrayDim);
            //}

            outputLines.push_back(
                fmt::format(
                    "    {} {}{}{};{}\n"
                    // fixme use package arg
                    , type == "SearchStatusOwner" ? "ESearchStatusOwner" : type
                    , propName
                    , prop->getType() == EClassTypes::UBoolProperty ? ": 1" : ""
                    , (prop->isArray() ? fmt::format("[{}]", prop->getArrayDim()).c_str() : "")
                    , flagStr.empty() ? "" : fmt::format(" // {}", flagStr)
            ));

            // annoying mode
            //fprintf(file, "    %s %s; // Offset: 0x%04X | Size: 0x%02X | Flags: 0x%08llX %s\n",
            //    type.c_str(),
            //    finalName.c_str(),
            //    static_cast<uint32_t>(offset),
            //    static_cast<uint32_t>(totalSize),
            //    static_cast<unsigned long long>(flags),
            //    flagString.c_str()
            //);

            cursorPosition = offset + prop->getTotalSize();
        }

        for (const auto& line : outputLines) {
            fputs(line.c_str(), file);
        }
    }

    void emitMethods(FILE* file) const {
        const auto sortedMethods = getSortedMethods();
        if (!sortedMethods.empty()) { fputs("\n", file); }

        for (UFunctionEntry* fn : sortedMethods) {
            fn->emitClassMethods(file);
        }
    }

    // fixme not needed methinks
    //void emitFindFunctionDef(FILE* file) const {
    //    if (asClass() == UObject::StaticClass()) {
    //        if (ConfigManager::instance().getProcessEventMethod() == "vtable") {
    //            fprintf(file, "    void ProcessEvent(UFunction* uFunction, void* uParams, void* uResult = nullptr);\n");
    //        // fixme?
    //        //} else if (GConfig::GetProcessEventIndex() != -1) {
    //        //    //GenerateVirtualFunctions(file);
    //        }
    //        //fprintf(file
    //        //    , "    static UFunction* SDK::findFunction(const std::string& %s);\n"
    //        //    , getFullName().c_str()
    //        //);
    //    }
    //}

    void emitStaticClasses(FILE* file) const {
        fputs("    static UClass* StaticClass() {\n", file);
        // fixme
        //if (GConfig::UsingConstants()) {
        //    fprintf(file, "return reinterpret_cast<UClass*>(UObject::GObjObjects()->at(%s));",
        //        GCache::GetConstant(unrealObj).first.c_str());
        //} else {
            fmt::print(file, "return r::uclass::find(\"{}\");",
                getFullName()
            );
        //}
        fputs("}\n", file);
    }

    void emitClose(FILE* file) const {
        fputs("};\n\n", file);
    }


private:
    UStruct* uStruct_{nullptr};
    std::unordered_set<UFunctionEntry*> methods_;
    std::unordered_set<ObjectEntry*> orphans_;
};