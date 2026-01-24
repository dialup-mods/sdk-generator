#pragma once

#include <fstream>
#include <sstream>
#include <ranges>
#include <string>
#include <utility>
#include <filesystem>

namespace fs = std::filesystem;

#include <clang-c/Index.h>
#include "CLangWrapper.h"
#include "SchemaDef.h"
#include "Logger.h"
#include "ConfigManager.h"

struct DeleteRange {
    size_t start;
    size_t end;
    std::string name; // for debugging
};

class SchemaLoader {
    SchemaLoader() = default;
    ~SchemaLoader() = default;

public:
    static auto instance() -> SchemaLoader& {
        static SchemaLoader instance;
        return instance;
    }

    SchemaLoader(SchemaLoader&&) = delete;
    SchemaLoader(const SchemaLoader&) = delete;
    auto operator=(SchemaLoader&&) -> SchemaLoader& = delete;
    auto operator=(const SchemaLoader&) -> SchemaLoader& = delete;

    auto file() const -> std::filesystem::path {
        return ConfigManager::instance().getGameConfigSchemaFile();
    }

    // Read original schema from source directory (for processing)
    auto readFile() const -> std::string {
//        auto filepath = ConfigManager::instance().getGameConfigSchemaFile();
        const auto filepath = fs::path(ConfigManager::instance().getGameConfigDir() / "Schema.h");
        auto filepathStr = filepath.string();
        printf("\n\nfilepath_: %s\n", filepathStr.c_str());
        std::ifstream file(filepath);
        if (!file.is_open()) {
            Logger::instance().log("[ERROR] Failed to open file: {}", filepath.string());
            return "";
        }
        std::string contents((std::istreambuf_iterator<char>(file)),
                     std::istreambuf_iterator<char>());
        file.close();
        return contents;
    }

    // Write processed schema to install directory (AppData/Local/DialUp/sdk-plugin/includes)
    void writeFile(const std::string& contents) const {
        auto outFile = ConfigManager::instance().getHeaderDirAbs() / "Schema.h";
        auto outfileStr = outFile.string();
        printf("\n\noutFile: %s\n", outfileStr.c_str());
        printf("\n\noutFile: %s\n", outfileStr.c_str());
        printf("\n\noutFile: %s\n", outfileStr.c_str());
        printf("\n\noutFile: %s\n", outfileStr.c_str());
        std::ofstream file(outFile, std::ios::binary);  // binary mode = no transformations
        if (!file.is_open()) {
            Logger::instance().log("[ERROR] Failed to open file for writing: {}", outFile.string());
        }
        file.write(contents.data(), contents.size());
        if (!file.good()) {
            Logger::instance().log("[ERROR] Failed to write to file: {}", outFile.string());
        }
        file.close();
    }

    auto load() -> bool {
        auto filepath = ConfigManager::instance().getGameConfigSchemaFile();
        auto filepathStr = filepath.string();
        printf("\n\nfilepath_: %s\n", filepathStr.c_str());
        Logger::instance().log("load schema file: {}", filepathStr.c_str());
        // todo, validate

        auto schemaContents = readFile();
        printf("schema contents: %s\n", schemaContents.c_str());

        // keep libclang loaded to prevent crash on unload
        // i'm tired, boss
        // 1mb leak vs 40 hours of debugging libclang internals on windows
        // not my money, not my circus
        HMODULE hClang = LoadLibraryA("libclang.dll");
        if (hClang) {
            // increment ref count so it doesn't unload with us
            LoadLibraryA("libclang.dll");
        }

        const char* args[] = {"-x", "c++", "-std=c++20"};

        auto index = clang_createIndex(0, 0);
        if (index == nullptr) {
            printf("bad index\n\n");
            return false;
        }

        auto translationUnit = clang_parseTranslationUnit(
            index,
            filepathStr.c_str(),
            args, std::size(args),
            nullptr, 0,
            CXTranslationUnit_None
        );

        if (translationUnit == nullptr) {
            printf("bad translation unit\n\n");
            return false;
        }

        const Cursor rootCursor(clang_getTranslationUnitCursor(translationUnit));

        visitChildren(rootCursor, [this, schemaContents](Cursor c) {
            // make sure we haven't traversed beyond our schema file
            CXSourceLocation loc = clang_getCursorLocation(c.raw());
            CXFile file = nullptr;
            unsigned line = 0, column = 0, offset = 0;
            clang_getExpansionLocation(loc, &file, &line, &column, &offset);
            CXString fileName = clang_getFileName(file);
            std::string fileStr = clang_getCString(fileName);
            clang_disposeString(fileName);

            if (fileStr.find("Schema.h") == std::string::npos) {
                return CXChildVisit_Continue;
            }

            //Logger::instance().log("Cursor: {} ({})\n", c.spelling(), c.kindSpelling());

            if (c.kind() == CXCursor_StructDecl) {
                if (c.shouldSkip()) { return CXChildVisit_Continue; }
                SchemaStruct schemaStruct{ .name = c.spelling() };
                if (c.isFinal()) {
                    schemaStruct.source = c.getSourceForCursor();
                    schemaStruct.isFinal = true;
                }

                if (c.isReplace()) {
                    schemaStruct.source = c.getSourceForCursor();
                    schemaStruct.isReplace = true;
                    schemaStruct.shouldDelete = true;
                }

                // find start and end offset
                auto start = c.getStartOffset();
                auto end = c.getEndOffset();

                // include semicolon
                while (end < schemaContents.size() && std::isspace(schemaContents[end])) ++end;
                if (end < schemaContents.size() && schemaContents[end] == ';') ++end;

                schemaStruct.startOffset = start;
                schemaStruct.endOffset = end;

                this->addStruct(std::move(schemaStruct));
            }

            if (c.kind() == CXCursor_ClassDecl || c.kind() == CXCursor_ClassTemplate) {
                std::optional<SourceLocation> injectStartPosition{};
                std::optional<SourceLocation> injectEndPosition{};

                //Logger::instance().log("Class found: {}", c.spelling());

                if (c.shouldSkip()) { return CXChildVisit_Continue; }

                SchemaClass cls{ .name = c.spelling() };

                if (c.isFinal()) {
                    cls.source = c.getSourceForCursor();
                    cls.isFinal = true;
                }

                if (c.isReplace()) {
                    cls.source = c.getSourceForCursor();
                    cls.isReplace = true;
                    cls.shouldDelete = true;
                }

                // find start and end offset
                auto start = c.getStartOffset();
                auto end = c.getEndOffset();

                // include semicolon
                while (end < schemaContents.size() && std::isspace(schemaContents[end])) ++end;
                if (end < schemaContents.size() && schemaContents[end] == ';') ++end;

                cls.startOffset = start;
                cls.endOffset = end;

                // nested visitor for methods
                visitChildren(c, [this, &injectStartPosition, &injectEndPosition](Cursor m) {
                    if (m.kind() == CXCursor_CXXMethod) {
                        if (m.isInjectMethodsStart()) {
                            injectStartPosition = m.getSourceStart();
                        }
                        if (m.isInjectMethodsEnd()) {
                            injectEndPosition = m.getSourceStart();
                        }
                        if (const auto fnOpt = m.parseMethod()) {
                            const auto& fn = *fnOpt;
                            this->addFunctionToClass(fn.owner, fn);
                        }
                    }
                    return CXChildVisit_Continue;
                });

                if (injectStartPosition.has_value()) {
                    cls.setInjectedMethodsText(c.getSlice(injectStartPosition.value(), injectEndPosition.value_or(c.getSourceStart())));
                    cls.shouldDelete = true;
                }

                this->addClass(std::move(cls));

                return CXChildVisit_Continue;
            }

            // free functions
            if (c.isOutOfClassFunctionDefinition()) {
                if (c.shouldSkip()) { return CXChildVisit_Continue; }

                if (auto fnOpt = c.parseMethod()) {
                    auto fn = *fnOpt;

                    // delete all inline codeblock functions w/ an attached class

                    // find start and end offset
                    auto start = c.getStartOffset();
                    auto end = c.getEndOffset();

                    // include semicolon
                    while (end < schemaContents.size() && std::isspace(schemaContents[end])) ++end;
                    if (end < schemaContents.size() && schemaContents[end] == ';') ++end;

                    fn.startOffset = start;
                    fn.endOffset = end;

                    this->addFunctionToClass(fn.owner, fn);
                }

                return CXChildVisit_Continue;
            }

            return CXChildVisit_Recurse;
        });

        clang_disposeTranslationUnit(translationUnit);
        clang_disposeIndex(index);

        return true;
    }

    auto scanForInsertionPoint(const std::string& needle) const -> std::optional<size_t> {
        auto schemaContents = readFile();

        const char* args[] = {"-x", "c++", "-std=c++20"};
        auto index = clang_createIndex(0, 0);
        if (index == nullptr) {
            printf("bad index\n\n");
            return std::nullopt;
        }
        const auto schemaFileStr = ConfigManager::instance().getGameConfigSchemaFile().string();
        const auto tu = clang_parseTranslationUnit(
            index,
            schemaFileStr.c_str(),
            args, std::size(args),
            nullptr, 0,
            CXTranslationUnit_None
        );

        if (!tu) {
            clang_disposeIndex(index);
            return std::nullopt;
        }

        const Cursor rootCursor(clang_getTranslationUnitCursor(tu));
        std::optional<size_t> insertionPoint;

        visitChildren(rootCursor, [this, schemaContents, needle, &insertionPoint](Cursor c) {
            // make sure we haven't traversed beyond our schema file
            const CXSourceLocation loc = clang_getCursorLocation(c.raw());
            CXFile file = nullptr;
            unsigned line = 0, column = 0, offset = 0;
            clang_getExpansionLocation(loc, &file, &line, &column, &offset);
            CXString fileName = clang_getFileName(file);
            std::string fileStr = clang_getCString(fileName);
            clang_disposeString(fileName);

            if (fileStr.find("Schema.h") == std::string::npos) {
                return CXChildVisit_Continue;
            }

            //Logger::instance().log("Cursor: {} ({})\n", c.spelling(), c.kindSpelling());

            if (c.kind() == CXCursor_StructDecl || c.kind() == CXCursor_ClassDecl || c.kind() == CXCursor_ClassTemplate) {
                if (c.spelling() == needle) {
                    size_t end = c.getEndOffset();

                    // include trailing whitespace + semicolon
                    while (end < schemaContents.size() &&
                           std::isspace(static_cast<unsigned char>(schemaContents[end]))) {
                        ++end;
                    }
                    if (end < schemaContents.size() && schemaContents[end] == ';') {
                        ++end;
                    }

                    insertionPoint = end;
                    return CXChildVisit_Break;
                }
            }

            return CXChildVisit_Continue;
        });

        clang_disposeTranslationUnit(tu);
        clang_disposeIndex(index);

        return insertionPoint;
    }

    void describe() const {
        Logger::instance().log("======== SCHEMA =======");
        const auto& classes = getClasses();

        Logger::instance().log("====== classes ======");
        for (const auto& className : classes | std::views::keys) {
            Logger::instance().log("Class: {}", className.empty() ? "" : className);
        }

        for (const auto& [className, cls] : classes) {
            Logger::instance().log("Class: {}", className.empty() ? "" : className);

            for (const auto& func : cls.methods | std::views::values) {
                std::string signature = fmt::format(
                    "  {} {}("
                    , func.returnType.empty() ? "" : func.returnType
                    , func.name.empty() ? "" : func.name);

                for (size_t i = 0; i < func.parameters.size(); ++i) {
                    const auto& param = func.parameters[i];
                    signature += fmt::format("{} {}", param.type.empty() ? param.type : "", param.name.empty() ? param.name : "");
                    if (i < func.parameters.size() - 1)
                        signature += ", ";
                }

                signature += ")";
                Logger::instance().log("{}", signature);
            }

            Logger::instance().log(""); // blank line between classes
        }
    }

    void addClass(SchemaClass cls) {
        classes_[cls.name] = std::move(cls);
    }

    void addStruct(SchemaStruct s) {
        structs_[s.name] = std::move(s);
    }

    void addFunctionToClass(const std::string& className, SchemaMethod func) {
        classes_[className].methods[func.name] = std::move(func);
    }

    auto getClasses() const -> const std::unordered_map<std::string, SchemaClass>& {
        return classes_;
    }

    auto getClassPtr(const std::string& name) const -> const SchemaClass* {
        const auto it = classes_.find(name);
        return it != classes_.end() ? &it->second : nullptr;
    }

    auto getClass(const std::string& name) -> SchemaClass* {
        const auto it = classes_.find(name);
        return it != classes_.end() ? &it->second : nullptr;
    }

    auto getStruct(const std::string& name) -> SchemaStruct* {
        const auto it = structs_.find(name);
        return it != structs_.end() ? &it->second : nullptr;
    }

    void finalize() {
        ////Logger::instance().log("finalize");
        for (const auto& schemaClass : instance().getClasses() | std::views::values) {
            if (schemaClass.shouldDelete) {
                // Delete the whole class block
                rangesToDelete_.push_back({
                    .start = schemaClass.startOffset,
                    .end   = schemaClass.endOffset
                });

                // Delete inline methods associated with this class
                for (const auto& method : schemaClass.getMethods() | std::views::values) {
                    if (method.isInline) {
                        rangesToDelete_.push_back({
                            .start = method.startOffset,
                            .end   = method.endOffset
                        });
                    }
                }
            }
        }

        std::string schemaContents = readFile();

        // sort ranges in reverse order to avoid invalidating offsets
        std::ranges::sort(rangesToDelete_,
                  [](const auto& a, const auto& b) {
                      return a.start > b.start;
                  });

        for (const auto& range : rangesToDelete_) {
            if (range.start < schemaContents.size() && range.end <= schemaContents.size()) {
                schemaContents.erase(range.start, range.end - range.start);
                //Logger::instance().log("Deleting processed class: {}", range.name);
            } else {
                //Logger::instance().log("Invalid range: ", range.start, "-", range.end);
            }
        }

        writeFile(schemaContents);
    }

private:
    std::vector<DeleteRange> rangesToDelete_{};

    std::unordered_map<std::string, SchemaClass> classes_{};
    std::unordered_map<std::string, SchemaStruct> structs_{};
    std::unordered_map<std::string, SchemaField> fields_{};
};
