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
    void append(std::string str) {
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
    void append(unsigned u) {
        appendFormat("%d", u);
    }
    void append(int u) {
        appendFormat("%d", u);
    }

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

    std::string toString() const {
        return buffer.str();
    }
};
}  // namespace Util

#endif  /* P4C_LIB_SOURCECODEBUILDER_H_ */
