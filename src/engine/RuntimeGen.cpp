#include "RuntimeGen.h"

#include "ConfigManager.h"
#include "Logger.h"
#include "Schema.h"
#include "TArrayValidator.h"

auto RuntimeGen::populate() -> bool {
    auto logAddress = [&](const char* label, uintptr_t ptr) {
        Logger::instance().log("{:<17}{:>15}", label, fmt::format("{:#x}", ptr));
    };

    uintptr_t fNameEntriesAddress = 0;
    const uintptr_t baseAddress = memory::getBaseAddress();
    logAddress("Base address:", baseAddress);

    { // FNameEntries
        const auto fNameEntriesMethod = ConfigManager::instance().getFNameEntriesMethod();

        if (fNameEntriesMethod == "pattern") {
            const auto fNameEntriesPattern = ConfigManager::instance().getFNameEntriesPattern();
            const auto fNameEntriesMask = ConfigManager::instance().getFNameEntriesMask();

            if (fNameEntriesPattern.empty() || fNameEntriesMask.empty()) {
                Logger::instance().log("[ERROR] FNameEntries pattern or mask is not set.");
                return false;
            }

            fNameEntriesAddress = memory::findPattern(fNameEntriesPattern, fNameEntriesMask);
            logAddress("FNameEntries address:", fNameEntriesAddress);
            if (!memory::isReadable(fNameEntriesAddress)) {
                Logger::instance().log("[ERROR] FNameEntries pattern not found.");
                return false;
            }

            Runtime::setFNameEntries(reinterpret_cast<TArray<FNameEntry*>*>(fNameEntriesAddress));
            logAddress("FNameEntries offset:", memory::getOffset(Runtime::getFNameEntriesPtr()));

        } else if (fNameEntriesMethod == "offset") {
            Runtime::setFNameEntries(reinterpret_cast<TArray<FNameEntry*>*>(baseAddress + ConfigManager::instance().getFNameEntriesOffset()));
            logAddress("FNameEntries address:", reinterpret_cast<uintptr_t>(Runtime::getFNameEntriesPtr()));
            logAddress("FNameEntries offset:", memory::getOffset(Runtime::getFNameEntriesPtr()));
        }

        if (Runtime::getFNameEntries().empty()) {
            Logger::instance().log("[ERROR] FNameEntries is null or empty");
            return false;
        }
    }

    { // UObjects
        const auto uObjectsMethod = ConfigManager::instance().getUObjectsMethod();

        // fixme check against nullptr before setting UObjects
        if (uObjectsMethod == "offsetFromFNameEntries") {
            const uintptr_t uObjectsAddress = fNameEntriesAddress + ConfigManager::instance().getUObjectsOffset();
            logAddress("UObjects address:", uObjectsAddress);

            if (!memory::isReadable(uObjectsAddress)) {
                Logger::instance().log("[ERROR] Could not read memory at address.");
                return false;
            }
            Runtime::setUObjects(reinterpret_cast<TArray<UObject*>*>(uObjectsAddress));
            logAddress("UObjects offset:", memory::getOffset(Runtime::getUObjectsPtr()));

        } else if (uObjectsMethod == "offset") {
            const uintptr_t uObjectsAddress = baseAddress + ConfigManager::instance().getUObjectsOffset();
            if (!memory::isReadable(uObjectsAddress)) {
                Logger::instance().log("[ERROR] Could not read memory at address.");
                return false;
            }
            Runtime::setUObjects(reinterpret_cast<TArray<UObject*>*>(baseAddress + ConfigManager::instance().getUObjectsOffset()));
            logAddress("UObjects address:", reinterpret_cast<uintptr_t>(Runtime::getUObjectsPtr()));
            logAddress("UObjects offset:", memory::getOffset(Runtime::getUObjectsPtr()));

        } else if (uObjectsMethod == "pattern") {
            const uintptr_t gObjectAddress = memory::findPattern(ConfigManager::instance().getUObjectsPattern(), ConfigManager::instance().getUObjectsMask());
            const uintptr_t uObjectsAddress = baseAddress + ConfigManager::instance().getUObjectsOffset();
            if (!memory::isReadable(uObjectsAddress)) {
                Logger::instance().log("[ERROR] Could not read memory at address.");
                return false;
            }
            logAddress("UObjects address:", reinterpret_cast<uintptr_t>(Runtime::getUObjectsPtr()));
            Runtime::setUObjects(reinterpret_cast<TArray<UObject*>*>(gObjectAddress));
            logAddress("UObjects offset:", memory::getOffset(Runtime::getUObjectsPtr()));
        }
    }

    { // validate
        if (!isProbablyValidTArray(*Runtime::getFNameEntriesPtr())) {
            Logger::instance().log("FNameEntries is probably not a valid TArray.");
            return false;
        }
        if (!Runtime::areFNameEntriesValid()) {
            Logger::instance().log("FNameEntries are invalid.");
        }

        if (!isProbablyValidTArray(*Runtime::getUObjectsPtr())) {
            Logger::instance().log("UObjects is probably not a valid TArray.");
            return false;
        }
        if (!Runtime::areUObjectsPopulated()) {
            Logger::instance().log("UObjects are invalid.");
            return false;
        }
    }

    { // cache
        auto& globalObjects = *Runtime::getUObjectsPtr();
        Runtime::getObjectCache().clear();
        Runtime::getObjectCache().reserve(globalObjects.size());

        size_t count = 0;
        for (UObject* obj : globalObjects) {
            Runtime::getObjectCache().push_back(obj);
            ++count;
        }
    }

    return true;
}

void RuntimeGen::dumpUObjects() {
    auto file = fopen(ConfigManager::instance().getObjectDumpFilepath().string().c_str(), "w"); // NOLINT
    if (!file) return;

    for (int32_t i = 0; i < Runtime::getUObjects().size() - 1; ++i) {
        if (UObject* uObject = Runtime::getUObjects().at(i)) {
            std::string name = uObject->GetFullName();
            if (!name.empty()) {
                fmt::print(
                    file
                    , "UObject[{:#06d}]  {:<40}  {:#018X}\n"
                    , uObject->ObjectInternalInteger
                    , name.c_str()
                    , reinterpret_cast<uintptr_t>(uObject)
                );
            }
        }
    }
    fclose(file);
}

void RuntimeGen::dumpFNames() {
    auto file = fopen(ConfigManager::instance().getFNameEntriesDumpFilepath().string().c_str(), "w"); // NOLINT
    if (!file) return;

    for (const auto nameEntry : *Runtime::getFNameEntriesPtr()) {
        if (nameEntry) {
            std::string name = nameEntry->ToString();
            if (!name.empty()) {
                fmt::print(
                    file
                    , "Name[{:#06d}]  {:<40}  {:#016X}\n"
                    , nameEntry->GetIndex()
                    , name.c_str()
                    , reinterpret_cast<uintptr_t>(nameEntry)
                );
            }
        }
    }
    fclose(file);
}
