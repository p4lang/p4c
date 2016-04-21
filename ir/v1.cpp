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

namespace {
class ActionArgSetup : public Transform {
    /* FIXME -- use ParameterSubstitution for this somehow? */
    std::map<cstring, const IR::Expression *>    args;
    const IR::Node *preorder(IR::PathExpression *pe) override {
        if (args.count(pe->path->name))
            return args.at(pe->path->name);
        return pe; }

 public:
    void add_arg(const IR::ActionArg *a) { args[a->name] = a; }
    void add_arg(cstring name, const IR::Expression *e) { args[name] = e; }
};
}

IR::ActionFunction::ActionFunction(const P4Action *ac, const Vector<Expression> *args) {
    srcInfo = ac->srcInfo;
    name = ac->name;
    ActionArgSetup setup;
    size_t arg_idx = 0;
    for (auto param : *ac->parameters->getEnumerator()) {
        if (param->direction == Direction::None) {
            auto arg = new IR::ActionArg(param->srcInfo, param->type, param->name);
            setup.add_arg(arg);
            this->args.push_back(arg);
        } else {
            if (!args || arg_idx >= args->size())
                error("%s: Not enough args for %s", args->srcInfo, ac);
            else
                setup.add_arg(param->name, args->at(arg_idx++)); } }
    if (arg_idx != (args ? args->size(): 0))
        error("%s: Too many args for %s", args->srcInfo, ac);
    for (auto stmt : *ac->body)
        if (auto assign = stmt->to<IR::AssignmentStatement>()) {
            auto prim = new IR::Primitive("modify_field", assign->left, assign->right);
            action.push_back(prim->apply(setup));
        } else if (auto mc = stmt->to<IR::MethodCallStatement>()) {
            BUG("extern method call %1% not yet implemented", mc);
        } else {
            BUG("un-handled statment %1% in action", stmt); }
}

IR::V1Table::V1Table(const P4Table *tc) {
    srcInfo = tc->srcInfo;
    name = tc->name;
    for (auto prop : *tc->properties->getEnumerator()) {
        if (auto key = prop->value->to<Key>()) {
            auto reads = new IR::Vector<IR::Expression>();
            for (auto el : *key->keyElements) {
                reads->push_back(el->expression);
                reads_types.push_back(el->matchType->path->name); }
            this->reads = reads;
        } else if (auto actions = prop->value->to<ActionList>()) {
            for (auto el : *actions->actionList)
                this->actions.push_back(el->name->path->name); } }
}
