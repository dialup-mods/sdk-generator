#pragma once

#include <clang-c/Index.h>

struct SourceLocation {
    CXFile file;
    unsigned offset;
};

#include "SchemaDef.h"

// ClangString name{clang_getCursorSpelling(cursor)};
// std::string cppName = name.str();
class ClangString {
public:
    explicit ClangString(CXString s) : s_(s) {}

    ~ClangString() { if (s_.data) clang_disposeString(s_); }

    std::string str() const {
        return clang_getCString(s_);
    }

    // non-copyable
    ClangString(const ClangString&) = delete;
    ClangString& operator=(const ClangString&) = delete;

    // movable
    ClangString(ClangString&& other) noexcept : s_(other.s_) {
        other.s_.data = nullptr;
        other.s_.private_flags = 0;
    }

private:
    CXString s_;
};

// usage:
// Cursor cur(rootCursor);
// if (cur.kind() == CXCursor_ClassDecl) { ... }
class Cursor {
public:
    Cursor(CXCursor c) : c_(c) {}

    CXCursor raw() const { return c_; }

    std::string spelling() const {
        return ClangString(clang_getCursorSpelling(c_)).str();
    }

    std::string typeSpelling() const {
        return ClangString(clang_getTypeSpelling(clang_getCursorType(c_))).str();
    }

    std::string returnTypeSpelling() const {
        return ClangString(clang_getTypeSpelling(clang_getCursorResultType(c_))).str();
    }

    CXCursorKind kind() const {
        return clang_getCursorKind(c_);
    }

    std::string kindSpelling() const {
        return ClangString(clang_getCursorKindSpelling(kind())).str();
    }

    int numArgs() const {
        return clang_Cursor_getNumArguments(c_);
    }

    Cursor getArg(int i) const {
        return Cursor(clang_Cursor_getArgument(c_, i));
    }

    auto getRawCommentSafe() const -> std::string {
        CXString raw = clang_Cursor_getRawCommentText(c_);
        if (!raw.data) {
            return {};
        }

        std::string result = clang_getCString(raw);
        clang_disposeString(raw);
        return result;
    }

    auto shouldSkip() const -> bool {
        const std::string comment = getRawCommentSafe();
        if (!comment.empty()) {
            if (comment.find("@generate_only") != std::string::npos) {
                //fmt::print("ignoring due to: {}\n", comment);
                return true;
            }
        }
        return false;
    }

    auto isFinal() const -> bool {
        const std::string comment = getRawCommentSafe();
        if (!comment.empty()) {
            if (comment.find("@final") != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    auto isReplace() const -> bool {
        const std::string comment = getRawCommentSafe();
        if (!comment.empty()) {
            if (comment.find("@replace") != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    auto isInjectMethodsStart() const -> bool {
        const std::string comment = getRawCommentSafe();
        if (!comment.empty()) {
            if (comment.find("@inject-methods") != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    auto isInjectMethodsEnd() const -> bool {
        const std::string comment = getRawCommentSafe();
        if (!comment.empty()) {
            if (comment.find("@end-inject") != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    auto isUpgradeRequested() const -> bool {
        const std::string comment = getRawCommentSafe();
        if (!comment.empty()) {
            if (comment.find("@upgrade") != std::string::npos) {
                return true;
            }
        }
        return false;
    }

    auto isOutOfClassFunctionDefinition() const -> bool {
        return clang_getCursorKind(c_) == CXCursor_CXXMethod
            && (!clang_equalCursors(
            clang_getCursorLexicalParent(c_),
            clang_getCursorSemanticParent(c_))
        );
    }

    auto getOwningClass() const -> std::optional<std::string> {
        CXCursor parent = clang_getCursorSemanticParent(c_);
        auto parentCursor = Cursor(parent);
        if (parentCursor.kind() == CXCursor_ClassDecl ||
            parentCursor.kind() == CXCursor_ClassTemplate ||
            parentCursor.kind() == CXCursor_StructDecl) {
            return parentCursor.spelling();
        }

        return std::nullopt;
    }

    [[nodiscard]] std::optional<SchemaMethod> parseMethod() const {
        auto methodName = spelling();
        //fmt::print("methodName: {}\n", methodName);

        auto owner = Cursor(c_).getOwningClass();
        if (!owner) {
            return std::nullopt;
        }

        //fmt::print("Method: {} belongs to class: {}\n", methodName, *owner);

        std::vector<SchemaParameter> params;
        int numArguments = numArgs();

        for (int i = 0; i < numArguments; ++i) {
            Cursor arg = getArg(i);
            params.push_back({
                .type = arg.typeSpelling(),
                .name = arg.spelling()
            });
            //fmt::print("    arg: {} [{}]\n", params.back().name, params.back().type);
        }

        std::string returnType = returnTypeSpelling();
        //fmt::print("    returns: {}\n", returnType);

        return SchemaMethod{
            .name = methodName,
            .owner = owner.value(),
            .returnType = returnType,
            .parameters = std::move(params)
        };
    }

    [[nodiscard]] auto getSourceStart() const -> SourceLocation {
        const CXSourceRange range = clang_getCursorExtent(c_);
        const CXSourceLocation loc = clang_getRangeStart(range);
        SourceLocation result;
        clang_getFileLocation(loc, &result.file, nullptr, nullptr, &result.offset);
        return result;
    }

    [[nodiscard]] auto getSourceEnd() const -> SourceLocation {
        const CXSourceRange range = clang_getCursorExtent(c_);
        const CXSourceLocation loc = clang_getRangeEnd(range);
        SourceLocation result;
        clang_getFileLocation(loc, &result.file, nullptr, nullptr, &result.offset);
        return result;
    }

    [[nodiscard]] auto getStartOffset() -> size_t {
        return getSourceStart().offset;
    }

    [[nodiscard]] auto getEndOffset() -> size_t {
        return getSourceEnd().offset;
    }

    [[nodiscard]] auto getSlice(const SourceLocation& start, const SourceLocation& end) const -> std::string {
        const CXString fileName = clang_getFileName(start.file);
        const char* filePath = clang_getCString(fileName);

        std::ifstream inFile(filePath);
        const std::string fileContents((std::istreambuf_iterator(inFile)),
                                  std::istreambuf_iterator<char>());
        clang_disposeString(fileName);

        if (start.offset < end.offset && end.offset <= fileContents.size()) {
            return fileContents.substr(start.offset, end.offset - start.offset);
        }
        return {};
    }

    [[nodiscard]] std::string getSourceForCursor() const {
        const auto start = getSourceStart();
        const auto end = getSourceEnd();

        const CXString fileName = clang_getFileName(start.file);
        const char* filePath = clang_getCString(fileName);

        std::ifstream inFile(filePath);
        const std::string fileContents((std::istreambuf_iterator(inFile)),
                                  std::istreambuf_iterator<char>());
        clang_disposeString(fileName);

        if (start.offset < end.offset && end.offset <= fileContents.size()) {
            return fileContents.substr(start.offset, end.offset - start.offset);
        }
        return {};
    }

private:
    CXCursor c_;
};

// usage:
// visitChildren(Cursor(root), [&](Cursor child) {
//     Logger::log("Visiting {}", child.spelling());
//     return CXChildVisit_Recurse;
// });
using CursorVisitorFn = std::function<CXChildVisitResult(Cursor)>;

struct VisitorData {
    CursorVisitorFn fn;
};

struct VisitorPayload {
    std::function<CXChildVisitResult(Cursor)> fn;
    void* userData;
};

extern "C" CXChildVisitResult VisitorThunk(
    CXCursor c, CXCursor parent, CXClientData client_data)
{
    auto* payload = static_cast<VisitorPayload*>(client_data);
    return payload->fn(Cursor(c));
}

inline void visitChildren(
    Cursor cursor,
    std::function<CXChildVisitResult(Cursor)> fn,
    void* userData = nullptr
) {
    VisitorPayload payload{ std::move(fn), userData };
    clang_visitChildren(cursor.raw(), VisitorThunk, &payload);
}

