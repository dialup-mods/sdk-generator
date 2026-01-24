#pragma once

#ifndef CONFIG_DIR
#error "CONFIG_DIR must be defined at compile time"
#endif

#ifndef GAME_CONFIG_DIR
#error "GAME_CONFIG_DIR must be defined at compile time"
#endif

#ifndef GAME_NAME
#error "GAME_NAME must be defined at compile time"
#endif

#include <filesystem>
#include <functional>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>

#include "ObjectFilter.h"
#include "ObjectStore.h"
#include "BenchmarkTimes.h"
#include "ConfigManager.h"
#include "Cowsay.h"
#include "Logger.h"
#include "Runtime.h"
#include "RuntimeGen.h"
#include "TypeRules.h"
#include "WaveWorker.h"

#include "UClass.h"
#include "UConst.h"
#include "UEnum.h"
#include "UScriptStruct.h"
#include "MessageBox.h"
#include "SchemaLoader.h"

using GroupedBase = std::unordered_map<std::string, std::vector<ObjectEntry*>>;
namespace fs = std::filesystem;

class SDKGenerator {
public:
    explicit SDKGenerator() {};
    ~SDKGenerator() = default;

    auto ensureDirectories() -> bool {
    //    const std::vector<std::pair<std::string, fs::path>> dirs = {
    //        //{ "combined include output", ConfigManager::instance().getCombinedIncludeDirAbs() },
    //        { "sdk output",              ConfigManager::instance().getSDKOutputDir() },
    //        { "headers",                 ConfigManager::instance().getHeaderDirAbs() },
    //        { "implementations",         ConfigManager::instance().getImplementationDirAbs() },
    //        { "meta",                    ConfigManager::instance().getMetaDirAbs() },
    //    };

    //    for (const auto& [name, path] : dirs) {
    //        Logger::instance().log("  Output directories:");
    //        Logger::instance().log("    {:<24}{}", name, path.string());
    //        Logger::instance().log("");
    //    }

    //    for (const auto& [name, path] : dirs) {
    //        std::error_code ec;
    //        fs::create_directories(path, ec);  // safe: creates parents too

    //        if (ec) {
    //            Logger::instance().log("[ERROR] Failed to create {} directory '{}': {}", name, path.string(), ec.message());
    //            return false;
    //        }
    //    }
        return true;
    }

    void run() {
        WaveWorker wave(std::chrono::milliseconds(50));
        wave.start();
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        wave.stop();
        Logger::instance().log("\n\n{}", cowsay::say("Dial-Up Unreal Engine SDK Generator\n\n                     by FreeAOL"));
        Logger::instance().log("\nUsing settings:\n");
        Logger::instance().log("    {:<24}{}", "Game config dir:", GAME_CONFIG_DIR);
        Logger::instance().log("    {:<24}{}\n", "Using configuration:", GAME_NAME);
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        Logger::instance().log("");

        Logger::instance().log("Loading config..");
        if (!ConfigManager::instance().load()) { return; }
        Logger::instance().log("after config");

        if (!ensureDirectories()) { return; }

        Logger::instance().log("Opening log file handle...");

        // open the log file for writing. before this point, logger just prints to console
        Logger::instance().open();

        Logger::instance().log("Creating object store...");
        {
            TypeRules::instance().initialize();
            if (generateSDK()) {
                //printSummary();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(900));
            Logger::instance().log("\nGoodbye.\n");
        }

        auto lockFile = ConfigManager::instance().getLockFileDir() / ".lock-sdkgen";
        fs::remove(lockFile);
    }

    auto isHeaderFile(const std::string& filename) -> bool {
        return filename.size() >= 2 &&
               (filename.ends_with(".h")); // or use regex if needed
    }

    auto labelFromSuffix(const std::string& suffix) -> std::string {
        const size_t dot = suffix.find('.');
        if (dot == std::string::npos) return suffix;  // no extension? whatever
        return suffix.substr(0, dot);
    }

    auto prettifyLabel(const std::string& suffix) -> std::string {
        std::string base = labelFromSuffix(suffix);
        std::ranges::replace(base, '_', ' ');
        return base;
    }

    void triggerEpicSpySequence() const {
        std::mt19937 rng(std::chrono::high_resolution_clock::now().time_since_epoch().count());
        std::uniform_int_distribution distribution(0, 100);
        if (const int randomNumber = distribution(rng); randomNumber == 69) {
            std::thread([log = &Logger::instance()] {
                log->log("[EAC] Scanning replay files...");
                std::this_thread::sleep_for(std::chrono::milliseconds(700));
                log->log("");
                log->log("[EAC] Analyzing for macro use...");
                std::this_thread::sleep_for(std::chrono::milliseconds(1700));
                log->log("");
                log->log("[EAC] Analyzing for bot use...");
                std::this_thread::sleep_for(std::chrono::milliseconds(2200));
                log->log("");
                log->log("[EAC] Detected input anomalies in Replay_043.dem");
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                log->log("");
                log->log("[OnlineSubsystem] Establishing secure connection to Epic servers...");
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
                log->log("");
                log->log("[EAC] Reporting HWID for suspected cheating...");
                log->log("[OnlineSubsystem] Uploading replay data...");
                std::this_thread::sleep_for(std::chrono::milliseconds(650));
                log->log("");
                log->log("[EAC] Report sent. Awaiting confirmation...");
                std::this_thread::sleep_for(std::chrono::milliseconds(180));
                log->log("");
                log->log("[EAC] Confirmed. Your account will be reviewed.");
            }).detach();
        }
    }

    void emitForwardDeclarations(FILE* file, std::vector<UScriptStructEntry*> entries) {
        // todo:
        // maybe instead of pushing around two lists
        // just diff the lists...
        // whatever isn't in sorted we fwd declare
        // OH
        // nvm.
        // sorted has to have everything, even the circular deps
        // so we probably are going to need 2 lists
    }

    template <typename T>
    void emitGroupedEntries(
        const std::string& suffix,
        std::vector<std::string>& headers,
        const std::function<GroupedBase()>& getEntriesGroupedByPackageFn,
        const std::function<GroupedBase()>& getForwardDeclarationEntriesGroupedByPackageFn,
        const std::function<void(FILE*, T*, std::string)>& emitEntryFn
    ) {
        WaveWorker wave(std::chrono::milliseconds(50));

        for (auto& [package, entries] : getEntriesGroupedByPackageFn()) {
            std::string filename = package + "_" + suffix;
            if (isHeaderFile(suffix)) headers.emplace_back(filename);

            //Logger::instance().logNoNewline("\n Emitting {}", prettifyLabel(filename));
            //wave.startSimple();
            wave.start();

            FILE* file = nullptr;
            if (isHeaderFile(suffix)) {
                 file = fopen((ConfigManager::instance().getHeaderDirAbs() / filename).string().c_str(), "w");
            } else {
                file = fopen((ConfigManager::instance().getImplementationDirAbs() / filename).string().c_str(), "w");
            }
            if (!file) { wave.stop(); continue; }

            if (isHeaderFile(suffix)) {
                fmt::print(file, "#pragma once\n");
                fmt::print(file, "#include \"{}\"\n", ConfigManager::instance().getCombinedIncludeFilename().string());
                if (package == "Core") {
                    fmt::print(file, "#include \"Schema.h\"\n");
                }
                fmt::print(file, "\n#pragma pack(push, {})\n\n", static_cast<int>(ConfigManager::instance().getFinalAlignment()));
                if (filename.find("parameters") != std::string::npos) {
                    fmt::print(file, "namespace {} {{\n\n", package);
                }
                // todo
                // this needs to be output after the includes
                for (auto& [packageStr, fwdDeclStructs] : getForwardDeclarationEntriesGroupedByPackageFn()) {
                    if (packageStr == package) {
                        if (!fwdDeclStructs.empty()) {
                            for (auto& s : fwdDeclStructs) {
                                s->emitForwardDeclaration(file);
                            }
                            fputs("\n", file);
                        }
                    }
                }

            } else {
                fmt::print(file, "#include \"{}\"\n\n", ConfigManager::instance().getCombinedIncludeFilename().string());
                fmt::print(file, "using namespace {};\n", package);
		fmt::print(file, "using r = Runtime;\n\n");
                fmt::print(file, "#pragma pack(push, {})\n\n", static_cast<int>(ConfigManager::instance().getFinalAlignment()));
            }

            //std::unordered_set<UFunctionEntry*> emittedEntries;
            for (auto* baseEntry : entries) {
                auto* entry = dynamic_cast<T*>(baseEntry);
                if (entry) {
                    if (entry->isBlacklisted()) {
                        continue;
                    }

                    //if (filename.find("parameters") != std::string::npos && std::is_same_v<T, UFunctionEntry>) {
                    //    if (emittedEntries.contains(entry)) {
                    //        Logger::instance().log("skipping duplicate entry {}", entry->getParamKey());
                    //        continue;
                    //    }

                    //    emittedEntries.insert(entry);
                    //    //Logger::instance().log("param key: {}", entry->getParamKey());
                    //    //if (emittedParams.contains(entry->getParamKey())) {
                    //    //    Logger::instance().log("skipping duplicate params");
                    //    //    continue;
                    //    //}
                    //    emittedParams.insert(entry->getParamKey());
                    //}

                    emitEntryFn(file, entry, package);
                }
            }

            if (isHeaderFile(suffix)) {
                if (filename.find("parameters") != std::string::npos) {
                    fmt::print(file, "}}\n");
                }
            }
            fputs("\n#pragma pack(pop)", file);

            fclose(file);
            wave.stop();
        }
    }

    auto generateSDK() -> bool {
        printf("generatesdk\n");
        const auto schemaFile = fs::path(ConfigManager::instance().getGameConfigDir() / "Schema.h");
        printf("after schema file\n");

        times().tStart = BenchmarkTimes::mark();

        Logger::instance().log("Parsing Schema file: {}", schemaFile.string().c_str());
        SchemaLoader::instance().load();
        printf("after schema load\n");

        for (const auto key : SchemaLoader::instance().getClasses() | std::views::keys) {
            printf("\n\n schema class: %s\n", key.c_str());
        }

        times().tSchema = BenchmarkTimes::mark();

        Logger::instance().log("Creating runtime...");
        Runtime::create();

        Logger::instance().log("Populating Runtime...");
        if (!RuntimeGen::populate()) {
            Logger::instance().log("[ERROR] Could not initialize Runtime. Check patterns and offsets and such.");
            return false;
        }

        Logger::instance().log("Initializing ObjectStore...");
        ObjectStore::instance().initialize();

        times().tCache = BenchmarkTimes::mark();

        std::vector<std::string> headers;
        triggerEpicSpySequence();

        auto [sortedStructs, forwardDecls] = object_filter::getFilteredStructs();

        emitGroupedEntries<UScriptStructEntry>(
            "structs.h"
            , headers
            , [sortedStructs]() { return sortedStructs; }
            , [forwardDecls]() { return forwardDecls;}
            , [](FILE* f, UScriptStructEntry* s, const std::string& package) {
                static const auto typeRules = TypeRules::instance();
                if (typeRules.isBlacklisted(s->getNameCPP())) {
                    //log->logNoNewline("\n\r - skipping: {} (blacklisted)", s->getNameCPP());
                    return;
                }
                const auto schemaStruct = SchemaLoader::instance().getStruct(s->getNameCPP());
                if (schemaStruct && schemaStruct->isFinal) {
                    Logger::instance().log("skipping: {} (final)", s->getNameCPP());
                    return;
                }
                s->emit(f, package);
            }
        );
        times().tStructs = BenchmarkTimes::mark();

        emitGroupedEntries<ClassEntry>(
            "classes.h"
            , headers
            , []() { return object_filter::getSortedClassEntriesGroupedByPackage(); }
            , []() { return GroupedBase{}; }
            , [](FILE* file, const ClassEntry* c, const std::string& package) {
                static const auto typeRules = TypeRules::instance();
                if (typeRules.isBlacklisted(c->getNameCPP())) {
                    return;
                }

                // fixme, this only checks type of property is blacklisted. maybe need to check name?
                for (auto prop : c->getSortedProperties()) {
                    if (typeRules.isBlacklisted(prop->getCanonicalType())) { return; }
                }

                for (auto method : c->getSortedMethods()) {
                    for (auto param : method->getAllParams()) {
                        if (typeRules.isBlacklisted(param->getCanonicalType())) { return; }
                        if (typeRules.isBlacklisted(param->getNameCPP())) { return; }
                    }

                    for (auto param : method->getArguments()) {
                        if (typeRules.isBlacklisted(param->getCanonicalType())) { return; }
                        if (typeRules.isBlacklisted(param->getNameCPP())) { return; }
                    }

                    if (typeRules.isBlacklisted(method->getReturnType())) { return; }
                }

                //const auto schemaClass = SchemaLoader::instance().getClass(c->getNameCPP());
                //if (schemaClass && schemaClass->isFinal) {
        	//    Logger::instance().log("skipping: {} (final)", c->getNameCPP());
                //    return;
                //}

                //if (schemaClass && schemaClass->isReplace) {
                //    Logger::instance().log("replacing: {}", c->getNameCPP());
                //    schemaClass->emitSource(file);
                //    return;
                //}

                c->emitClassSignature(file);
                c->emitProperties(file);
                c->emitStaticClasses(file);
                c->emitFindFunctionDef(file);

                //if (c->getFullName().find("Core.Object") != std::string::npos ) {
                //    SchemaLoader::instance().describe();
                //}
                //if (schemaClass) {
                //    printf("emitting injected methods");
                //    schemaClass->emitInjectedMethodsText(file);
                //    schemaClass->hasProcessed = true;
                //}

                c->emitMethods(file);
                c->emitClose(file);
            }
        );

        times().tHeaders = BenchmarkTimes::mark();

        emitGroupedEntries<UFunctionEntry>(
            "classes.cpp"
            , headers // not added to headers in this case
            , []() { return object_filter::getBestFunctionEntriesByPackage(); }
            , []() { return GroupedBase{}; }
            , [](FILE* f, UFunctionEntry* fn, const std::string& package) {
                static const auto typeRules = TypeRules::instance();

                for (auto param : fn->getAllParams()) {
                    if (typeRules.isBlacklisted(param->getCanonicalType())) { return; }
                }

                for (auto param : fn->getArguments()) {
                    if (typeRules.isBlacklisted(param->getCanonicalType())) { return; }
                }

                if (typeRules.isBlacklisted(fn->getReturnType())) { return; }

                if (typeRules.isBlacklisted(fn->getFullName())) {
                    return;
                }

                fn->emitImplementation(f);
            }
        );

        //        times().tFunctions = BenchmarkTimes::mark();
        //
        //        emitGroupedEntries<UFunctionEntry>(
        //            "parameters.h"
        //            , headers
        //            , []() { return object_filter::getBestFunctionEntriesByPackage(); }
        //            , []() { return GroupedBase{}; }
        //            , [](FILE* f, const UFunctionEntry* fn, const std::string& package) {
        //                static const auto typeRules = TypeRules::instance();
        //                for (auto param : fn->getAllParams()) {
        //                    if (typeRules.isBlacklisted(param->getCanonicalType())) { return; }
        //                }
        //
        //                for (auto param : fn->getArguments()) {
        //                    if (typeRules.isBlacklisted(param->getCanonicalType())) { return; }
        //                }
        //
        //                if (typeRules.isBlacklisted(fn->getReturnType())) { return; }
        //
        //                if (typeRules.isBlacklisted(fn->getNameCPP())) {
        //                    return;
        //                }
        //
        //                fn->emitFunctionParamsStruct(f);
        //            }
        //        );
        //        times().tParameters = BenchmarkTimes::mark();
        //
        //        emitGroupedEntries<ConstEntry>(
        //            "consts.h"
        //            , headers
        //            , []() { return object_filter::getConstEntriesByPackage(); }
        //            , []() { return GroupedBase{}; }
        //            , [](FILE* f, ConstEntry* c, const std::string& package) {
        //                static const auto typeRules = TypeRules::instance();
        //                if (typeRules.isBlacklisted(c->getNameCPP())) {
        //                    return;
        //                }
        //                c->emit(f, package);
        //            }
        //        );
        //        times().tConsts = BenchmarkTimes::mark();
        //
        //        emitGroupedEntries<EnumEntry>(
        //            "enums.h"
        //            , headers
        //            , []() { return object_filter::getEnumsGroupedByPackage(); }
        //            , []() { return GroupedBase{}; }
        //            , [](FILE* f, EnumEntry* e, const std::string& package) {
        //                static const auto typeRules = TypeRules::instance();
        //                if (typeRules.isBlacklisted(e->getNameCPP())) {
        //                    return;
        //                }
        //                e->emit(f, package);
        //            }
        //        );
        //        times().tEnums = BenchmarkTimes::mark();
        //
        //        {   // combined include file
        //            auto key = headerSortKey();
        //            std::ranges::sort(headers, [&](const std::string& a, const std::string& b) {
        //                return key(a) < key(b);
        //            });
        //
        //            FILE* includesFile = fopen(ConfigManager::instance().getCombinedIncludeFilenameAbs().string().c_str(), "w"); // NOLINT
        //	    Logger::instance().log( "includes file {}", ConfigManager::instance().getCombinedIncludeFilenameAbs().string().c_str());
        //            fputs(cowsay::say("Fixed your 11-year inheritance bug").c_str(), includesFile);
        //            fputs("\n#pragma once\n\n", includesFile);
        //            fputs(ConfigManager::instance().getCombinedIncludeForwardDeclarations().c_str(), includesFile);
        //            fputs("#include \"Flags.h\"\n", includesFile);
        //            fputs("#include \"Schema.h\"\n\n", includesFile);
        //            fputs("#include \"Runtime.h\"\n\n", includesFile);
        //            for (const auto& h : headers) {
        //                //fmt::print(includesFile, "#include \"{}/{}\"\n", ConfigManager::instance().getHeaderPathRel().string(), h.c_str());
        //                fmt::print(includesFile, "#include \"{}\"\n", h.c_str());
        //            }
        //            fclose(includesFile);
        //        }
        //
        //        if (!copyFiles()) {
        //            Logger::instance().log("[ERROR] Failed to copy files");
        //            return false;
        //        }
        //
        //        SchemaLoader::instance().finalize();
        //
        //        times().tIncludes = BenchmarkTimes::mark();
        //
        //        Logger::instance().log(" Exporting GNames...");
        //        RuntimeGen::dumpFNames();
        //        Logger::instance().log(" Exporting GObjects...");
        //        RuntimeGen::dumpUObjects();
        //
        //        times().tExportMaps = BenchmarkTimes::mark();
        //
        //        times().tEnd = BenchmarkTimes::mark();
        //
        //        return true;
        //
        //    }

        //    auto copyFiles() const -> bool {
        //        std::unordered_map<fs::path, fs::path> filesToMove;
        //        filesToMove[ConfigManager::instance().getGameConfigDir() / "Flags.h"]     = ConfigManager::instance().getHeaderDirAbs();
        //        filesToMove[ConfigManager::instance().getGameConfigDir() / "Schema.h"]    = ConfigManager::instance().getHeaderDirAbs();
        //        filesToMove[ConfigManager::instance().getGameConfigDir() / "Schema.cpp"]  = ConfigManager::instance().getImplementationDirAbs();
        //        filesToMove[ConfigManager::instance().getConfigEngineDir() / "Runtime.h"]   = ConfigManager::instance().getHeaderDirAbs();
        //        filesToMove[ConfigManager::instance().getConfigEngineDir() / "Runtime.cpp"] = ConfigManager::instance().getImplementationDirAbs();
        //
        //        for (const auto& [fromFileAbs, toDir] : filesToMove) {
        //            std::error_code ec;
        //            auto toFileAbs = fs::path(toDir / fromFileAbs.filename());
        //
        //            std::ifstream inFileStream(fromFileAbs);
        //            if (!inFileStream.is_open()) {
        //                Logger::instance().log("[ERROR] Failed to open source file: {}", fromFileAbs.string());
        //                return false;
        //            }
        //            std::string content((std::istreambuf_iterator(inFileStream)),
        //                                std::istreambuf_iterator<char>());
        //            inFileStream.close();
        //
        //            std::ofstream outFileStream(toFileAbs);
        //            if (!outFileStream.is_open()) {
        //                Logger::instance().log("[ERROR] Failed to create destination file: {}", toFileAbs.string());
        //                return false;
        //            }
        //            fmt::print(outFileStream,
        //                "// ============================================\n"
        //                "// Auto-generated by DialUp SDK Generator\n"
        //                "// DO NOT MODIFY - Changes will be overwritten\n"
        //                "// ============================================\n\n"
        //                "{}", content);
        //            outFileStream.close();
        //        }
        //
        //        return true;
        //    }

        //    void printSummary() const {
        //        const auto totalSeen = ObjectStore::instance().getTotalSeenCount();
        //        const auto totalGObjObjectCount = ObjectStore::instance().getTotalGObjObjectsCount();
        //        const auto invalidCount = ObjectStore::instance().getInvalidCount();
        //        const double percent = static_cast<double>(invalidCount) / totalSeen * 100.0; // NOLINT(*-narrowing-conversions)
        //
        //        Logger::instance().log("\nFile's done!\n");
        //        Logger::instance().log("Summary:\n");
        //        Logger::instance().log("  GObjObjects:       {}", totalGObjObjectCount);
        //        Logger::instance().log("  Unique addresses:  {}", totalSeen);
        //        Logger::instance().log("  Invalid UObjects:  {} ({:.2f}%)", invalidCount, percent);
        //
        //        if (invalidCount) {
        //            Logger::instance().log("Invalid UObject Summary:");
        //
        //            std::unordered_map<InvalidUObjectReason, size_t> reasonCounts;
        //            for (const auto& [reason, count] : reasonCounts) {
        //                Logger::instance().log("  • {:<28} {:>5}", toString(reason), count);
        //            }
        //        }
        //
        //        Logger::instance().log("");
        //        auto logTime = [&](const char* label, double seconds) {
        //            Logger::instance().log("  {:<15}{:>7.3f}s", label, seconds);
        //        };
        //
        //        logTime("Schema:",       BenchmarkTimes::delta(times().tStart,      times().tSchema));
        //        logTime("Cache:",        BenchmarkTimes::delta(times().tSchema,      times().tCache));
        //        logTime("Structs:",      BenchmarkTimes::delta(times().tCache,      times().tStructs));
        //        logTime("Headers:",      BenchmarkTimes::delta(times().tStructs,    times().tHeaders));
        //        logTime("Functions:",    BenchmarkTimes::delta(times().tHeaders,    times().tFunctions));
        //        logTime("Parameters:",   BenchmarkTimes::delta(times().tFunctions,  times().tParameters));
        //        logTime("Constants:",    BenchmarkTimes::delta(times().tParameters, times().tConsts));
        //        logTime("Enums:",        BenchmarkTimes::delta(times().tConsts,     times().tEnums));
        //        logTime("Includes:",     BenchmarkTimes::delta(times().tEnums,      times().tIncludes));
        //        logTime("Obj/Name Dump:",BenchmarkTimes::delta(times().tIncludes,   times().tExportMaps));
        //        Logger::instance().log("  =======================\r\b");
        //        logTime("️Total:",    BenchmarkTimes::delta(times().tStart,      times().tEnd));
        //    }
        return true;
    }

  private:
    auto times() const -> BenchmarkTimes& {
        static BenchmarkTimes t;
        return t;
    }

    // sort header files - core first, engine second, then sorted by `_type`
    auto headerSortKey() {
        return [](const std::string& name)
            -> std::tuple<int, std::string, int>
        {
            std::string base = name;
            if (base.ends_with(".h"))
                base = base.substr(0, base.size() - 2);

            int pkgPriority = 2;
            if (base.starts_with("Core_")) pkgPriority = 0;
            else if (base.starts_with("Engine_")) pkgPriority = 1;

            std::string pkgName = base.substr(0, base.find('_'));

            int typePriority = 99;
            if (base.ends_with("_consts")) typePriority = 0;
            else if (base.ends_with("_enums")) typePriority = 1;
            else if (base.ends_with("_structs")) typePriority = 2;
            else if (base.ends_with("_classes")) typePriority = 3;
            else if (base.ends_with("_parameters")) typePriority = 4;

            return {pkgPriority, pkgName, typePriority};
        };
    }
};
