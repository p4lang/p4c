#include "ir.h"
#include "dbprint.h"
#include "lib/gmputil.h"
#include "frontends/common/resolveReferences/referenceMap.h"

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

/***********************************************************************\
* v1.2->v1 lowering code.  These routines transform v1.2 IR for tables  *
* and actions to the simpler v1 IR, so that backends written with v1    *
* in mind can continue to use the v1 IR and still work with v1.2 input  *
\***********************************************************************/

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

class ActionBodySetup : public Inspector {
    IR::ActionFunction      *af;
    ActionArgSetup          &setup;
    bool preorder(const IR::Vector<IR::StatOrDecl> *) override { return true; }
    bool preorder(const IR::BlockStatement *) override { return true; }
    bool preorder(const IR::AssignmentStatement *assign) override {
        cstring pname = "modify_field";
        if (assign->left->type->is<IR::Type_Header>())
            pname = "copy_header";
        auto prim = new IR::Primitive(assign->srcInfo, pname, assign->left, assign->right);
        af->action.push_back(prim->apply(setup));
        return false; }
    bool preorder(const IR::MethodCallStatement *mc) override {
        ERROR("extern method call " << mc << " not yet implemented");
        return false; }
    bool preorder(const IR::Declaration *) override {
        // FIXME -- for now, ignoring local variables?  Need copy prop + dead code elim
        return false; }
    bool preorder(const IR::Node *n) override {
        BUG("un-handled node %1% in action", n);
        return false; }

 public:
    ActionBodySetup(IR::ActionFunction *af, ActionArgSetup &setup) : af(af), setup(setup) {}
};
}  // anonymous namespace

IR::ActionFunction::ActionFunction(const P4Action *ac, const Vector<Expression> *args) {
    srcInfo = ac->srcInfo;
    name = ac->externName();
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
    ac->body->apply(ActionBodySetup(this, setup));
}

static void setIntProperty(cstring name, int *val, const IR::PropertyValue *pval) {
    if (auto *ev = pval->to<IR::ExpressionValue>()) {
        if (auto *cv = ev->expression->to<IR::Constant>()) {
            *val = cv->asInt();
            return; } }
    error("%s: %s property must be a constant", pval->srcInfo, name);
}

IR::V1Table::V1Table(const P4Table *tc, const P4::ReferenceMap *refMap) {
    srcInfo = tc->srcInfo;
    name = tc->externName();
    for (auto prop : *tc->properties->getEnumerator()) {
        if (prop->name == "key") {
            auto reads = new IR::Vector<IR::Expression>();
            for (auto el : *prop->value->to<Key>()->keyElements) {
                reads->push_back(el->expression);
                reads_types.push_back(el->matchType->path->name); }
            this->reads = reads;
        } else if (prop->name == "actions") {
            for (auto el : *prop->value->to<ActionList>()->actionList)
                this->actions.push_back(refMap->getDeclaration(el->name->path)->externName());
        } else if (prop->name == "default_action") {
            auto v = prop->value->to<ExpressionValue>();
            if (!v) {
            } else if (auto pe = v->expression->to<PathExpression>()) {
                default_action = refMap->getDeclaration(pe->path)->externName();
                default_action.srcInfo = pe->srcInfo;
                continue;
            } else if (auto mc = v->expression->to<MethodCallExpression>()) {
                if (auto pe = mc->method->to<PathExpression>()) {
                    default_action = refMap->getDeclaration(pe->path)->externName();
                    default_action.srcInfo = mc->srcInfo;
                    default_action_args = mc->arguments;
                    continue; } }
            BUG("default action %1% is not an action or call", prop->value);
        } else if (prop->name == "size") {
            setIntProperty(prop->name, &size, prop->value);
        } else if (prop->name == "min_size") {
            setIntProperty(prop->name, &min_size, prop->value);
        } else if (prop->name == "max_size") {
            setIntProperty(prop->name, &max_size, prop->value); } }
}
