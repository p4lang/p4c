#include "ir.h"
#include "dbprint.h"
#include "lib/gmputil.h"

cstring IR::NamedCond::unique_name() {
    static int unique_counter = 0;
    char buf[16];
    snprintf(buf, sizeof(buf), "cond-%d", unique_counter++);
    return buf;
}

struct primitive_info_t {
    unsigned    min_operands, max_operands;
    unsigned    out_operands;  // bitset -- 1 bit per operand
    unsigned    type_match_operands;    // bitset -- 1 bit per operand
};

static const std::map<cstring, primitive_info_t> prim_info = {
    { "add",                    { 3, 3, 0x1, 0x7 } },
    { "add_header",             { 1, 1, 0x1, 0x0 } },
    { "add_to_field",           { 2, 2, 0x1, 0x3 } },
    { "bit_and",                { 3, 3, 0x1, 0x7 } },
    { "bit_andca",              { 3, 3, 0x1, 0x7 } },
    { "bit_andcb",              { 3, 3, 0x1, 0x7 } },
    { "bit_nand",               { 3, 3, 0x1, 0x7 } },
    { "bit_nor",                { 3, 3, 0x1, 0x7 } },
    { "bit_not",                { 2, 2, 0x1, 0x3 } },
    { "bit_or",                 { 3, 3, 0x1, 0x7 } },
    { "bit_orca",               { 3, 3, 0x1, 0x7 } },
    { "bit_orcb",               { 3, 3, 0x1, 0x7 } },
    { "bit_xnor",               { 3, 3, 0x1, 0x7 } },
    { "bit_xor",                { 3, 3, 0x1, 0x7 } },
    { "clone_egress_pkt_to_egress",     { 1, 2, 0x0, 0x0 } },
    { "clone_ingress_pkt_to_egress",    { 1, 2, 0x0, 0x0 } },
    { "copy_header",            { 2, 2, 0x1, 0x3 } },
    { "copy_to_cpu",            { 1, 1, 0x0, 0x0 } },
    { "count",                  { 2, 2, 0x1, 0x0 } },
    { "drop",                   { 0, 0, 0x0, 0x0 } },
    { "execute_meter",          { 3, 4, 0x1, 0x0 } },
    { "execute_stateful_alu",   { 1, 1, 0x0, 0x0 } },
    { "generate_digest",        { 2, 2, 0x0, 0x0 } },
    { "max",                    { 3, 3, 0x1, 0x7 } },
    { "min",                    { 3, 3, 0x1, 0x7 } },
    { "modify_field",           { 2, 3, 0x1, 0x7 } },
    { "modify_field_from_rng",  { 2, 3, 0x1, 0x5 } },
    { "modify_field_with_hash_based_offset",    { 4, 4, 0x1, 0x0 } },
    { "no_op",                  { 0, 0, 0x0, 0x0 } },
    { "pop",                    { 2, 2, 0x1, 0x0 } },
    { "push",                   { 2, 2, 0x1, 0x0 } },
    { "recirculate",            { 1, 1, 0x0, 0x0 } },
    { "register_read",          { 3, 3, 0x1, 0x0 } },
    { "register_write",         { 3, 3, 0x0, 0x0 } },
    { "remove_header",          { 1, 1, 0x1, 0x0 } },
    { "resubmit",               { 1, 1, 0x0, 0x0 } },
    { "shift_left",             { 3, 3, 0x1, 0x3 } },
    { "shift_right",            { 3, 3, 0x1, 0x3 } },
    { "subtract",               { 3, 3, 0x1, 0x7 } },
    { "subtract_from_field",    { 2, 2, 0x1, 0x3 } },
    { "valid",                  { 1, 1, 0x0, 0x0 } },
};

void IR::Primitive::typecheck() const {
    if (prim_info.count(name)) {
        auto &info = prim_info.at(name);
        if (operands.size() < info.min_operands)
            error("%s: not enough operands for primitive %s", srcInfo, name);
        if (operands.size() > info.max_operands)
            error("%s: too many operands for primitive %s", srcInfo, name);
    } else {
        error("%s: unknown primitive %s", srcInfo, name); }
}

bool IR::Primitive::isOutput(int operand_index) const {
    if (prim_info.count(name))
        return (prim_info.at(name).out_operands >> (operand_index-1)) & 1;
    return false;
}

unsigned IR::Primitive::inferOperandTypes() const {
    if (prim_info.count(name))
        return prim_info.at(name).type_match_operands;
    return 0;
}

void IR::ActionList::checkDuplicates() const {
    std::set<cstring> found;
    for (auto ale : *actionList) {
        if (found.count(ale->getName().name) > 0)
            ::error("Duplicate action name in table: %1%", ale);
        found.emplace(ale->getName().name);
    }
}

