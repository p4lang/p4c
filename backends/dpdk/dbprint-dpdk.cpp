#include <ostream>
#include <string>
#include <vector>

#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "lib/cstring.h"
#include "lib/log.h"

void IR::DpdkJmpLabelStatement::dbprint(std::ostream &out) const {
    out << "jmp " << label << std::endl;
}

void IR::DpdkLabelStatement::dbprint(std::ostream &out) const { out << label << std::endl; }

void IR::DpdkJmpIfInvalidStatement::dbprint(std::ostream &out) const {
    out << "jmpnv " << label << " " << header << std::endl;
}

void IR::DpdkApplyStatement::dbprint(std::ostream &out) const {
    out << "apply " << table << std::endl;
}

void IR::DpdkLearnStatement::dbprint(std::ostream &out) const {
    out << "learn " << action << std::endl;
}

void IR::DpdkMirrorStatement::dbprint(std::ostream &out) const {
    out << "mirror " << slotId << " " << sessionId << std::endl;
}

void IR::DpdkEmitStatement::dbprint(std::ostream &out) const {
    out << "emit " << header << std::endl;
}

void IR::DpdkJmpNotEqualStatement::dbprint(std::ostream &out) const {
    out << "jmpneq " << label << " " << src1 << " " << src2 << std::endl;
}

void IR::DpdkValidateStatement::dbprint(std::ostream &out) const {
    out << "validate " << header << std::endl;
}

void IR::DpdkInvalidateStatement::dbprint(std::ostream &out) const {
    out << "invalidate " << header << std::endl;
}

void IR::DpdkDropStatement::dbprint(std::ostream &out) const { out << "drop" << std::endl; }

void IR::DpdkAsmProgram::dbprint(std::ostream &out) const {
    for (auto h : headerType) out << h;
}

void IR::DpdkListStatement::dbprint(std::ostream &out) const {
    for (auto s : statements) out << s;
}

void IR::DpdkExtractStatement::dbprint(std::ostream &out) const {
    out << "extract " << header;
    if (length)
        out << length << std::endl;
    else
        out << std::endl;
}

void IR::DpdkLookaheadStatement::dbprint(std::ostream &out) const {
    out << "lookahead " << header << std::endl;
}

void IR::DpdkJmpEqualStatement::dbprint(std::ostream &out) const {
    out << "jmpeq " << label << " " << src1 << " " << src2 << std::endl;
}

void IR::DpdkMovStatement::dbprint(std::ostream &out) const {
    out << "mov " << dst << " " << src << std::endl;
}
