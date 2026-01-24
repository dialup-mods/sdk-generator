#pragma once
#include <map>
#include <string>
#include <vector>
#include "fmt/format.h"

struct SchemaParameter {
    std::string type;
    std::string name;
};

struct SchemaField {
    std::string type;
    std::string name;
};

struct SchemaMethod {
    std::string name;
    std::string owner;
    std::string returnType;
    std::vector<SchemaParameter> parameters;
    bool isInline;
    size_t startOffset;
    size_t endOffset;

    void emit(FILE* file) const {
        std::string signature = fmt::format(
            "  {} {}("
            , returnType.empty() ? "" : returnType
            , name.empty() ? "" : name);

        for (size_t i = 0; i < parameters.size(); ++i) {
            const auto& param = parameters[i];
            signature += fmt::format("{} {}", param.type.empty() ? param.type : "", param.name.empty() ? param.name : "");
            if (i < parameters.size() - 1)
                signature += ", ";
        }

        signature += ")";

        Logger::log("[ERROR] UNIMPLEMENTED whoops");
        //Logger::log("Emitting from Schema.h {}", signature);
    }
};

struct SchemaStruct {
    std::string name;
    std::map<std::string, SchemaMethod> methods;
    bool isFinal;
    bool isReplace;
    bool shouldDelete;
    std::string source;
    bool hasProcessed = false;
    size_t startOffset;
    size_t endOffset;
};

struct SchemaClass {
    std::string name;
    std::map<std::string, SchemaMethod> methods;
    bool isFinal;
    bool isReplace;
    bool shouldDelete;
    std::string source;
    bool hasProcessed = false;
    size_t startOffset;
    size_t endOffset;
    std::string injectMethodsText;

    void addMethod(const std::string& className, SchemaMethod func) {
        methods[func.name] = std::move(func);
    }

    void setShouldDelete(const bool bDelete) {
        shouldDelete = bDelete;
    }

    void setInjectedMethodsText(std::string text) {
        injectMethodsText = std::move(text);
    }

    auto getInjectedMethodsText() -> std::string {
        return injectMethodsText;
    }

    void emitInjectedMethodsText(FILE* file) {
        if (!getInjectedMethodsText().empty()) {
            fmt::print(file, "\n    {}\n", getInjectedMethodsText().c_str());
        }
    }

    [[nodiscard]] auto getMethods() const -> const std::map<std::string, SchemaMethod>& {
        return methods;
    }

    [[nodiscard]] auto getMethods() -> std::map<std::string, SchemaMethod>& {
        return methods;
    }

    void emitSource(FILE* file) const {
        fputs("\n", file);
        fputs(source.c_str(), file);
        fputs(";\n", file);
    }

    void emitAllMethods(FILE* file) {
        for (const auto& func : this->methods | std::views::values) {
            func.emit(file);
        }
    };
};