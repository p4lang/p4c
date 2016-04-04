#ifndef _IR_ID_H_
#define _IR_ID_H_

#include "lib/source_file.h"
#include "lib/error.h"
#include "lib/exceptions.h"
#include "lib/cstring.h"

namespace IR {

// an identifier
struct ID : Util::IHasSourceInfo {
    Util::SourceInfo    srcInfo;
    cstring             name = nullptr;
    ID() = default;
    ID(Util::SourceInfo si, cstring n) : srcInfo(si), name(n) {
        if (n.isNullOrEmpty()) BUG("Identifier with no name"); }
    ID(const char *n) : ID(Util::SourceInfo(), n) {}  // NOLINT(runtime/explicit)
    ID(cstring n) : ID(Util::SourceInfo(), n) {}  // NOLINT(runtime/explicit)
    void dbprint(std::ostream &out) const { out << name; }
    bool operator==(const ID &a) const { return name == a.name; }
    bool operator!=(const ID &a) const { return name != a.name; }
    explicit operator bool() const { return name; }
    operator cstring() const { return name; }
    bool isDontCare() const { return name == "_"; }
    Util::SourceInfo getSourceInfo() const override { return srcInfo; }
    cstring toString() const override { return name; }
};

}  // namespace IR
#endif  // _IR_ID_H_
