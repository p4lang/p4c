#include "validateParsedProgram.h"

namespace P4 {

void ValidateParsedProgram::postorder(const IR::Constant* c) {
    if (c->type != nullptr &&
        !c->type->is<IR::Type_Unknown>() &&
        !c->type->is<IR::Type_Bits>() &&
        !c->type->is<IR::Type_InfInt>())
        BUG("Invalid type %1% for constant %2%", c->type, c);
}

void ValidateParsedProgram::postorder(const IR::Method* m) {
    if (m->name.isDontCare())
        ::error("%1%: Illegal method/function name", m->name);
}

void ValidateParsedProgram::postorder(const IR::StructField* f) {
    if (f->name.isDontCare())
        ::error("%1%: Illegal field name", f->name);
}

void ValidateParsedProgram::postorder(const IR::Type_Union* type) {
    if (type->fields.size() == 0)
        ::error("%1%: empty union", type);
}

void ValidateParsedProgram::postorder(const IR::Type_Bits* type) {
    if (type->size <= 0)
        ::error("%1%: Illegal bit size", type);
    if (type->size == 1 && type->isSigned)
        ::error("%1%: Signed types cannot be 1-bit wide", type);
}

void ValidateParsedProgram::postorder(const IR::ParserState* s) {
    if (s->name == IR::ParserState::accept ||
        s->name == IR::ParserState::reject)
        ::error("%1%: parser state should not be implemented, it is built-in", s->name);
}

void ValidateParsedProgram::postorder(const IR::P4Table* t) {
    auto ac = t->properties->getProperty(IR::TableProperties::actionsPropertyName);
    if (ac == nullptr)
        ::error("Table %1% does not have an `%2%' property",
                t->name, IR::TableProperties::actionsPropertyName);
    auto da = t->properties->getProperty(IR::TableProperties::defaultActionPropertyName);
    if (!p4v1 && da == nullptr)
        ::warning("Table %1% does not have an `%2%' property",
                t->name, IR::TableProperties::defaultActionPropertyName);
}

}  // namespace P4
