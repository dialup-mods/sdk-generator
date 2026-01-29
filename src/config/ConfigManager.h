#pragma once

#ifndef SDK_OUTPUT_DIR
#error "SDK_OUTPUT_DIR must be defined at compile time"
#endif

#ifndef CONFIG_DIR
#error "CONFIG_DIR must be defined at compile time"
#endif

#ifndef ENGINE_RUNTIME_DIR
#error "ENGINE_RUNTIME_DIR must be defined at compile time"
#endif

#ifndef GAME_CONFIG_DIR
#error "GAME_CONFIG_DIR must be defined at compile time"
#endif

#ifndef LOG_DIR
#error "LOG_DIR must be defined at compile time"
#endif

#ifndef GAME_NAME
#error "GAME_NAME must be defined at compile time"
#endif

#ifndef ENGINE_RUNTIME_DIR
#error "ENGINE_RUNTIME_DIR must be defined at compile time"
#endif

#ifndef UE_MODEL_DIR
#error "UE_MODEL_DIR must be defined at compile time"
#endif

#ifndef DIALUP_DIR
#error "DIALUP_DIR must be defined at compile time"
#endif

#include <filesystem>
#include <fstream>
#include <string>

#include "fmt/format.h"
#include "fkYAML/node.hpp"

#include "MessageBox.h"

class ConfigManager {
    ConfigManager() = default;
    ~ConfigManager() = default;

public:
    static auto instance() -> ConfigManager& {
        static ConfigManager instance;
        return instance;
    }

    ConfigManager(ConfigManager&&) = delete;
    ConfigManager(const ConfigManager&) = delete;
    auto operator=(ConfigManager&&) -> ConfigManager& = delete;
    auto operator=(const ConfigManager&) -> ConfigManager& = delete;

    auto load() const -> bool {
        const auto configFile = std::filesystem::path(CONFIG_DIR) / std::format("{}.yaml", GAME_NAME);
        if (!exists(configFile)) {
            messagebox::error("[ERROR] Config file does not exist at path: " + configFile.string());
            return false;
        }
        try {
            std::ifstream ifs(configFile);
            config_ = fkyaml::node::deserialize(ifs);
            return true;
        } catch (const std::exception& e) {
            messagebox::error(fmt::format("[ERROR] Failed to load config: {}", e.what()));
            config_ = {};
            return false;
        }
    }

    template<typename T>
    auto getNestedValue(const std::string& dottedKey) const -> T {
        const fkyaml::node* current = &getConfig();

        size_t start = 0;
        while (start < dottedKey.size()) {
            const size_t end = dottedKey.find('.', start);
            std::string key = dottedKey.substr(start, end - start);

            if (!current->contains(key)) {
                messagebox::error("Missing config key: " + key);
                return {};
            }
            current = &current->at(key);

            if (end == std::string::npos) { break; };
            start = end + 1;
        }

        if constexpr (std::is_same_v<T, std::string>) {
            return current->as_str();
        }

        return current->get_value<T>();
    }

    auto getNestedNode(const std::string& dottedKey) const -> const fkyaml::node* {
        const fkyaml::node* current = &getConfig();
        size_t start = 0;

        while (start < dottedKey.size()) {
            const size_t end = dottedKey.find('.', start);
            std::string key = dottedKey.substr(start, end - start);

            if (!current->contains(key)) {
                messagebox::error("Missing config key: " + key);
                return {};
            }

            current = &current->at(key);

            if (end == std::string::npos) { break; }
            start = end + 1;
        }

        return current;
    }

    auto getConfigEngineDir() const -> std::filesystem::path {
        return std::string(ENGINE_RUNTIME_DIR);
    }

    auto getModelDir() const -> std::filesystem::path {
        return std::string(UE_MODEL_DIR);
    }

    auto getConfigDir() const -> std::filesystem::path {
        return std::string(CONFIG_DIR);
    }

    auto getGameConfigDir() const -> std::filesystem::path {
        return std::string(GAME_CONFIG_DIR);
    }

    auto getDialUpDir() const -> std::filesystem::path {
        return std::string(DIALUP_DIR);
    }

    auto getSDKOutputDir() const -> std::filesystem::path {
        return std::string(SDK_OUTPUT_DIR);
    }

    auto getLockFileDir() const -> std::filesystem::path {
        return std::string(DIALUP_DIR);
    }

    auto getGameConfigSchemaFile() const -> std::filesystem::path {
        return getGameConfigDir() / "Schema.h";
    }

    auto getEngineDir() const -> std::filesystem::path {
        return std::string(ENGINE_RUNTIME_DIR);
    }

    auto getPlatform() const -> std::string {
        return getNestedValue<std::string>("game.platform");
    }

    auto getImplementationDirAbs() const -> std::filesystem::path {
        return getSDKOutputDir() / "impl";
    }

    auto getHeaderDirAbs() const -> std::filesystem::path {
        return getSDKOutputDir() / "include";
    }

    auto getHeaderDirRel() const -> std::filesystem::path {
        return std::filesystem::relative(
            std::filesystem::path(getHeaderDirAbs())
            , getCombinedIncludeDirAbs().parent_path()
        );
    }

    auto getHeaderExtension() const -> std::filesystem::path {
        return getNestedValue<std::string>("output.headerExtension");
    }

    auto getCombinedIncludeFilename() const -> std::filesystem::path {
        return getCombinedIncludeFilenameAbs().filename();
    }

    auto getCombinedIncludeDirAbs() const -> std::filesystem::path {
        return getSDKOutputDir() / "include";
    }

    auto getCombinedIncludeFilenameAbs() const -> std::filesystem::path {
        return getCombinedIncludeDirAbs() / "SDK.h";
    }

    auto getCombinedIncludeForwardDeclarations() const -> std::string {
        return getNestedValue<std::string>("output.combinedIncludeForwardDeclarations");
    }

    auto getLogFile() const -> std::filesystem::path {
        return std::filesystem::path(getDialUpDir() / "log.txt");
    }

    auto getMetaDirAbs() const -> std::filesystem::path {
        return getSDKOutputDir();
    }

    auto getObjectDumpFilepath() const -> std::filesystem::path {
        return getMetaDirAbs() / "Objects.txt";
    }

    auto getFNameEntriesDumpFilepath() const -> std::filesystem::path {
        return getMetaDirAbs() / "FNameEntries.txt";
    }

    auto getProcessEventMethod() const -> std::string {
        return getNestedValue<std::string>("patterns.processEvent.method");
    }

    auto getProcessEventIndex() const -> uint64_t {
        return getNestedValue<uint64_t>("patterns.processEvent.index");
    }

    auto getFNameEntriesMethod() const -> std::string {
        return getNestedValue<std::string>("patterns.fNameEntries.method");
    }

    auto getFNameEntriesPattern() const -> std::string {
        return getNestedValue<std::string>("patterns.fNameEntries.pattern");
    }

    auto getFNameEntriesOffset() const -> uint64_t {
        return getNestedValue<uint64_t>("patterns.fNameEntries.offset");
    }

    auto getFNameEntriesMask() const -> std::string {
        return getNestedValue<std::string>("patterns.fNameEntries.mask");
    }

    auto getUObjectsMethod() const -> std::string {
        return getNestedValue<std::string>("patterns.uObjects.method");
    }

    auto getUObjectsPattern() const -> std::string {
        return getNestedValue<std::string>("patterns.uObjects.pattern");
    }

    auto getUObjectsMask() const -> std::string {
        return getNestedValue<std::string>("patterns.uObjects.mask");
    }

    auto getUObjectsOffset() const -> uint64_t {
        return getNestedValue<uint64_t>("patterns.uObjects.offset");
    }

    // generation

    auto insertDeprecatedStaticClassFunction() const -> bool {
        return getNestedValue<bool>("generation.format.insertDeprecatedStaticClassFunction");
    }

    auto getGameAlignment() const -> uint64_t {
        return getNestedValue<uint64_t>("generation.alignment.game");
    }

    auto getFinalAlignment() const -> uint64_t {
        return getNestedValue<uint64_t>("generation.alignment.final");
    }

    auto getBlacklist() const -> std::vector<std::string> {
        return getNestedValue<std::vector<std::string>>("types.blacklist");
    }

    auto getRulesNode() const -> const fkyaml::node* {
        auto* node = getNestedNode("rules");
        if (!node || node->empty()) {
            return nullptr;
        }
        return node;
    }

    auto getOverrideNode() const -> const fkyaml::node* {
        return getNestedNode("types.override");
    }

    auto getPrefixEnumsWithClass() const -> bool {
        return getNestedValue<bool>("generation.prefixEnumsWithClass");
    }


  private:
    auto getConfig() const -> const fkyaml::node& {
        if (config_.empty()) {
            messagebox::error("[ERROR] Config not loaded");
        }
        return config_;
    }
    mutable fkyaml::node config_;
};