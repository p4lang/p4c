#include <ir/ir.h>

void IR::DpdkJmpLabelStatement::dbprint(std::ostream& out) const {
    out << "jmp " << label;
}

void IR::DpdkLabelStatement::dbprint(std::ostream& out) const {
    out << "label " << label;
}

void IR::DpdkJmpIfInvalidStatement::dbprint(std::ostream& out) const {
    out << "jmpnv " << label << " " << header;
}

void IR::DpdkApplyStatement::dbprint(std::ostream& out) const {
    out << "apply " << table;
}

void IR::DpdkEmitStatement::dbprint(std::ostream& out) const {
    out << "emit " << header;
}

void IR::DpdkJmpNotEqualStatement::dbprint(std::ostream& out) const {
    out << "jmpneq " << label << " " << src1 << " " << src2;
}
