#include "Runtime.h"
#include "SDK.h"
#include "Schema.h"

TArray<FNameEntry*>* Runtime::fNameEntries_{nullptr};
TArray<UObject*>* Runtime::uObjects_{nullptr};
Runtime* Runtime::instance_{nullptr};
std::map<std::string, UClass*> Runtime::classCache_{};
std::map<std::string, UFunction*> Runtime::functionCache_{};
std::vector<UObject*> Runtime::uObjectsCache_{};

void SDK_API Runtime::create() {
    if (!instance_) instance_ = new Runtime();
}

auto SDK_API Runtime::instance() -> Runtime& {
    return *instance_;
}

void SDK_API Runtime::yeet() {
    delete instance_;
    instance_ = nullptr;
}

auto SDK_API Runtime::getFNameEntries() -> TArray<FNameEntry*>& { return *fNameEntries_; }

auto SDK_API Runtime::getFNameEntry(const int32_t index) -> FNameEntry* {
    if (index < 0 || index >= getFNameEntries().size()) {
        return nullptr;
    }
    return getFNameEntries()[index];
}

auto SDK_API Runtime::getFNameEntryName(const int32_t index) -> std::string {
    const FNameEntry* entry = getFNameEntry(index);
    return entry ? entry->ToString() : "";
}

auto SDK_API Runtime::findClass(const std::string& classFullName) -> UClass* {
    if (classCache_.empty()) {
        for (int32_t i = 0; i < uObjects_->size() - 1; i++) {
            if (UObject* uObject = uObjects_->at(i)) {
                if (std::string objectFullName = uObject->GetFullName(); objectFullName.starts_with("Class")) {
                    classCache_[objectFullName] = reinterpret_cast<UClass*>(uObject);
                }
            }
        }
    }

    if (classCache_.contains(classFullName)) {
        return classCache_[classFullName];
    }

    return nullptr;
}

auto SDK_API Runtime::findFunction(const std::string& functionFullName) -> UFunction* {
    if (functionCache_.empty()) {
        for (int32_t i = 0; i < uObjects_->size() - 1; i++) {
            if (UObject* uObject = uObjects_->at(i)) {
                if (std::string objectFullName = uObject->GetFullName(); objectFullName.find("Function") == 0) {
                    functionCache_[objectFullName] = reinterpret_cast<UFunction*>(uObject);
                }
            }
        }
    }

    if (functionCache_.contains(functionFullName)) {
        return functionCache_[functionFullName];
    }

    return nullptr;
}

auto SDK_API Runtime::findPackages() -> std::vector<UObject*> {
    static std::vector<UObject*> packages;
    if (packages.empty()) {
        for (int i = 0; i < 10; ++i) {
            if (UObject* obj = getUObjects().at(i); obj && !obj->GetName().empty()) {
                packages.emplace_back(obj);
            }
        }
    }
    return packages;
}

auto SDK_API Runtime::areFNameEntriesValid() -> bool {
    if (getFNameEntries().empty()) {
    	// fixme, return error string
        return false;
    }

    if (getFNameEntries().at(0)->ToString() != "None") {
    	// fixme, return error string
        return false;
    }

    if (getFNameEntries().size() < 1000) {
    	// fixme, return error string
        return false;
    }

    return true;
}

auto SDK_API Runtime::areUObjectsPopulated() -> bool {
    if (getUObjectsPtr()->empty()) {
        //ILogger::instance().log("UObjects not populated");
        return false;
    }
    return true;
}
