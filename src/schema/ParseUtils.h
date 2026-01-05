#pragma once
#include <fstream>
#include <clang-c/Index.h>
#include <string>

namespace parse_utils {

    // paste the whole body
    //
    // CXSourceRange range = clang_getCursorExtent(fn.cursor);
    // CXTranslationUnit tu = clang_Cursor_getTranslationUnit(fn.cursor);
    //
    // CXToken* tokens;
    // unsigned numTokens;
    // clang_tokenize(tu, range, &tokens, &numTokens);
    //
    // std::string body;
    // for (token : tokens) {
    //     CXString spelling = clang_getTokenSpelling(tu, tokens[i]);
    //     body += clang_getCString(spelling);
    //     clang_disposeString(spelling);
    // }
    //
    // clang_disposeTokens(tu, tokens, numTokens);

//
inline std::string getFunctionBodyOnly(CXCursor cursor, CXTranslationUnit tu) {
    std::string body;
    clang_visitChildren(
        cursor,
        [](CXCursor c, CXCursor parent, CXClientData userData) {
            if (clang_getCursorKind(c) == CXCursor_CompoundStmt) {
                CXSourceRange range = clang_getCursorExtent(c);
                CXSourceLocation start = clang_getRangeStart(range);
                CXSourceLocation end = clang_getRangeEnd(range);

                CXFile file;
                unsigned startOffset, endOffset;
                clang_getFileLocation(start, &file, nullptr, nullptr, &startOffset);
                clang_getFileLocation(end, &file, nullptr, nullptr, &endOffset);

                CXString filename = clang_getFileName(file);
                std::ifstream inFile(clang_getCString(filename));
                std::string contents((std::istreambuf_iterator<char>(inFile)),
                                      std::istreambuf_iterator<char>());

                if (startOffset < endOffset && endOffset <= contents.size()) {
                    *static_cast<std::string*>(userData) = contents.substr(startOffset, endOffset - startOffset);
                }

                clang_disposeString(filename);
                return CXChildVisit_Break;
            }

            return CXChildVisit_Continue;
        },
        &body
    );

    return body;
}

std::string getFunctionSignature(CXCursor cursor) {
    CXType resultType = clang_getCursorResultType(cursor);
    CXString resultTypeSpelling = clang_getTypeSpelling(resultType);

    std::string returnType = clang_getCString(resultTypeSpelling);
    clang_disposeString(resultTypeSpelling);

    CXString cursorSpelling = clang_getCursorSpelling(cursor);
    std::string funcName = clang_getCString(cursorSpelling);
    clang_disposeString(cursorSpelling);

    std::string signature = returnType + " " + funcName + "(";

    int numArgs = clang_Cursor_getNumArguments(cursor);
    for (int i = 0; i < numArgs; ++i) {
        CXCursor arg = clang_Cursor_getArgument(cursor, i);

        CXType argType = clang_getCursorType(arg);
        CXString argTypeStr = clang_getTypeSpelling(argType);

        CXString argNameStr = clang_getCursorSpelling(arg);

        signature += clang_getCString(argTypeStr);

        std::string argName = clang_getCString(argNameStr);
        if (!argName.empty()) {
            signature += " " + argName;
        }

        clang_disposeString(argTypeStr);
        clang_disposeString(argNameStr);

        if (i < numArgs - 1)
            signature += ", ";
    }

    signature += ")";

    if (clang_CXXMethod_isConst(cursor)) {
        signature += " const";
    }

    return signature;
}

auto isTopLevelFunction(CXCursor cursor) -> bool {
    return clang_getCursorKind(cursor) == CXCursor_FunctionDecl
        && clang_getCursorSemanticParent(cursor).kind == CXCursor_TranslationUnit;
}

auto isDeclaration(CXCursor cursor) -> bool {
    CXCursorKind kind = clang_getCursorKind(cursor);
    return kind == CXCursor_ClassTemplate ||
           kind == CXCursor_ClassDecl ||
           kind == CXCursor_StructDecl ||
           kind == CXCursor_FunctionDecl ||
           kind == CXCursor_CXXMethod;
}


// fixme
// auto shouldToggleSkipMode(CXCursor cursor, bool isInSkipMode) -> bool {
//    std::string comment = getRawCommentSafe(cursor);
//    if (!comment.empty()) {
//        fmt::print("is in skip mode: {}", isInSkipMode);
//        if (comment.find("sdk_ignore_on") != std::string::npos && !isInSkipMode) {
//            fmt::print("skip toggle on\n", comment);
//            return true;
//        }
//        if (comment.find("sdk_ignore_off") != std::string::npos && isInSkipMode) {
//            fmt::print("skip toggle off\n", comment);
//            return true;
//        }
//    }
//    return false;
//}

}