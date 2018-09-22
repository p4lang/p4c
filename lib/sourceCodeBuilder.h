/*
Copyright 2013-present Barefoot Networks, Inc. 

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

#ifndef P4C_LIB_SOURCECODEBUILDER_H_
#define P4C_LIB_SOURCECODEBUILDER_H_

#include <ctype.h>
#include <sstream>

#include "lib/stringify.h"
#include "lib/cstring.h"
#include "lib/exceptions.h"

namespace Util {
class SourceCodeBuilder {
    int indentLevel;  // current indent level
    unsigned indentAmount;

    std::stringstream buffer;
    const std::string nl = "\n";
    bool endsInSpace;

 public:
    SourceCodeBuilder() :
            indentLevel(0),
            indentAmount(4),
            endsInSpace(false)
    {}

    void increaseIndent() { indentLevel += indentAmount; }
    void decreaseIndent() {
        indentLevel -= indentAmount;
        if (indentLevel < 0)
            BUG("Negative indent");
    }
    void newline() { buffer << nl; endsInSpace = true; }
    void spc() {
        if (!endsInSpace)
            buffer << " ";
        endsInSpace = true;
    }

    void append(cstring str) { append(str.c_str()); }
    void appendLine(cstring str) { append(str); newline(); }
    void append(const std::string &str) {
        if (str.size() == 0)
            return;
        endsInSpace = ::isspace(str.at(str.size() - 1));
        buffer << str;
    }
    void append(const char* str) {
        if (str == nullptr)
            BUG("Null argument to append");
        if (strlen(str) == 0)
            return;
        endsInSpace = ::isspace(str[strlen(str) - 1]);
        buffer << str;
    }
    void appendFormat(const char* format, ...) {
        va_list ap;
        va_start(ap, format);
        cstring str = Util::vprintf_format(format, ap);
        va_end(ap);
        append(str);
    }
    void append(unsigned u) { appendFormat("%d", u); }
    void append(int u) { appendFormat("%d", u); }

    void endOfStatement(bool addNl = false) {
        append(";");
        if (addNl) newline(); }

    void blockStart() {
        append("{");
        newline();
        increaseIndent();
    }

    void emitIndent() {
        buffer << std::string(indentLevel, ' ');
        if (indentLevel > 0)
            endsInSpace = true;
    }

    void blockEnd(bool nl) {
        decreaseIndent();
        emitIndent();
        append("}");
        if (nl)
            newline();
    }

    std::string toString() const { return buffer.str(); }
    void commentStart() { append("/* "); }
    void commentEnd() { append(" */"); }
    bool lastIsSpace() const { return endsInSpace; }
};
}  // namespace Util

#endif  /* P4C_LIB_SOURCECODEBUILDER_H_ */
