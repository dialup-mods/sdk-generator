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

            Runtime::fname::game_pool::set(reinterpret_cast<TArray<FNameEntry*>*>(fNameEntriesAddress));
            logAddress("FNameEntries offset:", memory::getOffset(Runtime::fname::game_pool::ptr()));

        } else if (fNameEntriesMethod == "offset") {
            Runtime::fname::game_pool::set(reinterpret_cast<TArray<FNameEntry*>*>(baseAddress + ConfigManager::instance().getFNameEntriesOffset()));
            logAddress("FNameEntries address:", reinterpret_cast<uintptr_t>(Runtime::fname::game_pool::ptr()));
            logAddress("FNameEntries offset:", memory::getOffset(Runtime::fname::game_pool::ptr()));
        }

        if (Runtime::fname::game_pool::ref().empty()) {
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
            Runtime::uobject::game_pool::set(reinterpret_cast<TArray<UObject*>*>(uObjectsAddress));
            logAddress("UObjects offset:", memory::getOffset(Runtime::uobject::game_pool::ptr()));

        } else if (uObjectsMethod == "offset") {
            const uintptr_t uObjectsAddress = baseAddress + ConfigManager::instance().getUObjectsOffset();
            if (!memory::isReadable(uObjectsAddress)) {
                Logger::instance().log("[ERROR] Could not read memory at address.");
                return false;
            }
            Runtime::uobject::game_pool::set(reinterpret_cast<TArray<UObject*>*>(baseAddress + ConfigManager::instance().getUObjectsOffset()));
            logAddress("UObjects address:", reinterpret_cast<uintptr_t>(Runtime::uobject::game_pool::ptr()));
            logAddress("UObjects offset:", memory::getOffset(Runtime::uobject::game_pool::ptr()));

        } else if (uObjectsMethod == "pattern") {
            const uintptr_t gObjectAddress = memory::findPattern(ConfigManager::instance().getUObjectsPattern(), ConfigManager::instance().getUObjectsMask());
            const uintptr_t uObjectsAddress = baseAddress + ConfigManager::instance().getUObjectsOffset();
            if (!memory::isReadable(uObjectsAddress)) {
                Logger::instance().log("[ERROR] Could not read memory at address.");
                return false;
            }
            logAddress("UObjects address:", reinterpret_cast<uintptr_t>(Runtime::uobject::game_pool::ptr()));
            Runtime::uobject::game_pool::set(reinterpret_cast<TArray<UObject*>*>(gObjectAddress));
            logAddress("UObjects offset:", memory::getOffset(Runtime::uobject::game_pool::ptr()));
        }
    }

    { // validate
        if (!isProbablyValidTArray(*Runtime::fname::game_pool::ptr())) {
            Logger::instance().log("FNameEntries is probably not a valid TArray.");
            return false;
        }
        if (!Runtime::fname::game_pool::isValid()) {
            Logger::instance().log("FNameEntries are invalid.");
        }

        if (!isProbablyValidTArray(*Runtime::uobject::game_pool::ptr())) {
            Logger::instance().log("UObjects is probably not a valid TArray.");
            return false;
        }
        if (!Runtime::uobject::game_pool::hasUObjects()) {
            Logger::instance().log("UObjects are invalid.");
            return false;
        }
    }

    { // cache
        auto& globalObjects = *Runtime::uobject::game_pool::ptr();
        Runtime::uobject::cache::ref().clear();
        Runtime::uobject::cache::ref().reserve(globalObjects.size());

        size_t count = 0;
        for (UObject* obj : globalObjects) {
            Runtime::uobject::cache::ref().push_back(obj);
            ++count;
        }
    }

    return true;
}

void RuntimeGen::dumpUObjects() {
    auto file = fopen(ConfigManager::instance().getObjectDumpFilepath().string().c_str(), "w"); // NOLINT
    if (!file) return;

    for (int32_t i = 0; i < Runtime::uobject::game_pool::ref().size() - 1; ++i) {
        if (UObject* uObject = Runtime::uobject::game_pool::ref().at(i)) {
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

    for (const auto nameEntry : Runtime::fname::game_pool::ref()) {
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
