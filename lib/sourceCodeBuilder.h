/*
 * SPDX-FileCopyrightText: 2013 Barefoot Networks, Inc.
 * Copyright 2013-present Barefoot Networks, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef LIB_SOURCECODEBUILDER_H_
#define LIB_SOURCECODEBUILDER_H_

#include <ctype.h>

#include "absl/strings/cord.h"
#include "absl/strings/str_format.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"
#include "lib/stringify.h"

namespace P4::Util {
class SourceCodeBuilder {
    int indentLevel;  // current indent level
    unsigned indentAmount;

    absl::Cord buffer;
    bool endsInSpace;
    bool supressSemi = false;

 public:
    SourceCodeBuilder() : indentLevel(0), indentAmount(4), endsInSpace(false) {}

    void increaseIndent() { indentLevel += indentAmount; }
    void decreaseIndent() {
        indentLevel -= indentAmount;
        if (indentLevel < 0) BUG("Negative indent");
    }
    void newline() {
        buffer.Append("\n");
        endsInSpace = true;
    }
    void spc() {
        if (!endsInSpace) buffer.Append(" ");
        endsInSpace = true;
    }

    void append(cstring str) { append(str.c_str()); }
    void appendLine(const char *str) {
        append(str);
        newline();
    }
    void appendLine(cstring str) {
        append(str);
        newline();
    }
    void append(const std::string &str) {
        if (str.empty()) return;
        endsInSpace = ::isspace(str.back());
        buffer.Append(str);
    }
    [[deprecated("use string / char* version instead")]]
    void append(char c) {
        std::string str(1, c);
        append(str);
    }
    void append(const char *str) {
        if (str == nullptr) BUG("Null argument to append");
        if (strlen(str) == 0) return;
        endsInSpace = ::isspace(str[strlen(str) - 1]);
        buffer.Append(str);
    }

    template <typename... Args>
    void appendFormat(const absl::FormatSpec<Args...> &format, Args &&...args) {
        // FIXME: Sink directly to cord
        append(absl::StrFormat(format, std::forward<Args>(args)...));
    }
    void append(unsigned u) { appendFormat("%d", u); }
    void append(int u) { appendFormat("%d", u); }

    void endOfStatement(bool addNl = false) {
        if (!supressSemi) append(";");
        supressSemi = false;
        if (addNl) newline();
    }
    void supressStatementSemi() { supressSemi = true; }

    void blockStart() {
        append("{");
        newline();
        increaseIndent();
    }

    void emitIndent() {
        buffer.Append(std::string(indentLevel, ' '));
        if (indentLevel > 0) endsInSpace = true;
    }

    void blockEnd(bool nl) {
        decreaseIndent();
        emitIndent();
        append("}");
        if (nl) newline();
    }

    std::string toString() const { return std::string(buffer); }
    void commentStart() { append("/* "); }
    void commentEnd() { append(" */"); }
    bool lastIsSpace() const { return endsInSpace; }
};
}  // namespace P4::Util

#endif /* LIB_SOURCECODEBUILDER_H_ */
