#include "typecheck.h"

// P4 v1.0 and v1.1 type checking algorithm
// Initial type setting based on immediate context:
// - replace named reference to ActionParams in the bodies of ActionFunctions with the
//   actual ActionParam
// - link header/metadata instances to the Type
// - replace named references to global header or metadata instances with ConcreteHeaderRef
//   expressions that link directly to them.
// - set type for Member and HeaderStackItemRefs
class TypeCheck::Pass1 : public Transform {
    const IR::V1Program   *global = nullptr;
    const IR::Node *preorder(IR::V1Program *glob) { global = glob; return glob; }
    const IR::Node *preorder(IR::NamedRef *ref) override {
        if (auto af = findContext<IR::ActionFunction>())
            if (auto arg = af->arg(ref->name))
                return arg;
        return ref; }
    const IR::Node *preorder(IR::Metadata *m) override {
        if (!global) return m;
        if (auto ht = global->get<IR::v1HeaderType>(m->type_name))
            m->type = ht->as_metadata;
        else
            error("%s: No header type %s defined", m->srcInfo, m->type_name);
        return m; }
    const IR::Node *preorder(IR::BoolLiteral* b) override {
        b->type = IR::Type_Boolean::get();
        return b; }
    const IR::Node *preorder(IR::HeaderOrMetadata *hm) override {
        if (!global) return hm;
        if (auto ht = global->get<IR::v1HeaderType>(hm->type_name))
            hm->type = ht->as_header;
        else
            error("%s: No header type %s defined", hm->srcInfo, hm->type_name);
        return hm; }
    const IR::Node *postorder(IR::NamedRef *ref) override {
        if (!global) return ref;
        IR::Node *new_node = ref;
        if (auto hdr = global->get<IR::HeaderOrMetadata>(ref->name))
            new_node = new IR::ConcreteHeaderRef(ref->srcInfo, hdr);
        else if (ref->name != "latest" && findContext<IR::Member>())
            error("%s: No header or metadata named %s", ref->srcInfo, ref->name);
        return new_node; }
    const IR::Node *postorder(IR::HeaderStackItemRef *ref) override {
        if (auto ht = ref->base()->type->to<IR::Type_StructLike>())
            ref->type = ht;
        else if (auto hst = ref->base()->type->to<IR::Type_Stack>())
            ref->type = hst->baseType;
        else
            error("%s: %s is not a header", ref->base()->srcInfo, ref->base()->toString());
        return ref; }
    const IR::Node *postorder(IR::Member *ref) override {
        if (ref->member.name[0] == '$') {
        } else if (auto ht = ref->expr->type->to<IR::Type_StructLike>()) {
            auto f = ht->getField(ref->member);
            if (f != nullptr) {
                ref->type = f->type;
                return ref; }
            error("%s: No field named %s in %s", ref->srcInfo, ref->member, ht->name);
        } else if (ref->expr->toString() != "latest") {
            error("%s: %s is not a header", ref->expr->srcInfo, ref->expr->toString()); }
        return ref; }
};

static const IR::Type *combine(const Util::SourceInfo &loc, const IR::Type *a, const IR::Type *b) {
    if (!a || a == IR::Type::Unknown::get()) return b;
    if (!b || b == IR::Type::Unknown::get()) return a;
    if (a == IR::Type_InfInt::get()) return b;
    if (b == IR::Type_InfInt::get()) return a;
    if (*a == *b) return a;
    error("%s: Incompatible types %s and %s", loc, a->toString(), b->toString());
    return a;
}

// bottom up type inferencing -- set the types of expression nodes based on operands,
// - also records types of operands to action function calls for Pass3 to use
class TypeCheck::Pass2 : public Modifier {
    TypeCheck           &self;
    const IR::V1Program *global = nullptr;
    profile_t init_apply(const IR::Node *root) {
        global = root->to<IR::V1Program>();
        self.actionArgUseTypes.clear();
        self.iterCounter++;
        return Modifier::init_apply(root); }
    void postorder(IR::Member *) override {}
    void postorder(IR::Operation_Unary *op) override {
        op->type = op->expr->type; }
    void postorder(IR::Operation_Binary *op) override {
        if (op->left->type == IR::Type_InfInt::get())
            op->type = op->right->type;
        else if (op->right->type == IR::Type_InfInt::get())
            op->type = op->left->type;
        else if (op->left->type == op->right->type)
            op->type = op->left->type; }
    void postorder(IR::Operation_Relation *op) override {
        op->type = IR::Type::Boolean::get(); }
    void postorder(IR::Primitive *prim) override {
        if (!global || !findContext<IR::ActionFunction>())
            return;
        if (auto af = global->get<IR::ActionFunction>(prim->name)) {
            auto arg = af->args.begin();
            for (auto op : prim->operands) {
                if (arg == af->args.end()) {
                    if (self.iterCounter == 1)
                        error("%s: too many arguments to action %s", prim->srcInfo, prim);
                    break; }
                if (op->type != IR::Type::Unknown::get())
                    self.actionArgUseTypes[*arg] =
                        combine(prim->srcInfo, self.actionArgUseTypes[*arg], op->type);
                ++arg; }
            if (arg != af->args.end() && self.iterCounter == 1)
                error("%s: not enough arguments to action %s", prim->srcInfo, prim);
        } else if (self.iterCounter == 1) {
            prim->typecheck(); } }

 public:
    explicit Pass2(TypeCheck &s) : self(s) {}
};

// top down type inferencing -- set the type of expression nodes based on their uses.
class TypeCheck::Pass3 : public Modifier {
    TypeCheck           &self;
    const IR::V1Program *global = nullptr;
    profile_t init_apply(const IR::Node *root) {
        global = root->to<IR::V1Program>();
        return Modifier::init_apply(root); }
    const IR::Type *ctxtType() {
        const IR::Type *rv = IR::Type::Unknown::get();
        const Context *ctxt = getContext();
        if (auto parent = ctxt->node->to<IR::Expression>()) {
            if (auto p = parent->to<IR::Operation_Relation>()) {
                if (ctxt->child_index == 1)
                    rv = p->right->type;
                else if (ctxt->child_index == 2)
                    rv = p->left->type;
                else
                    BUG("Unepxected child index");
            } else if (parent->is<IR::Operation::Binary>()) {
                rv = parent->type;
            } else if (!global) {
            } else if (auto prim = parent->to<IR::Primitive>()) {
                if (auto af = global->get<IR::ActionFunction>(prim->name)) {
                    if (size_t(ctxt->child_index-1) < af->args.size())
                        rv = af->args[ctxt->child_index-1]->type;
                } else if (auto infer = prim->inferOperandTypes()) {
                    if ((infer >> (ctxt->child_index-1)) & 1) {
                        for (auto o : prim->operands) {
                            if ((infer & 1) && o->type != rv) {
                                rv = o->type;
                                break; }
                            infer >>= 1; } } } } }
        return rv; }
    bool preorder(IR::Expression *op) override {
        if (op->type == IR::Type::Unknown::get() || op->type == IR::Type_InfInt::get()) {
            auto *type = ctxtType();
            if (type != IR::Type::Unknown::get())
                op->type = type; }
        return true; }
    bool preorder(IR::ActionArg *arg) override {
        // FIXME -- this duplicates P4WriteContext::isWrite, but that is unable to deal with
        // calls of action functions (it treats all IR::Primitive as primitive calls)
        const Context *ctxt = getContext();
        if (auto *prim = ctxt->node->to<IR::Primitive>()) {
            if (auto af = global ? global->get<IR::ActionFunction>(prim->name) : nullptr) {
                if (size_t(ctxt->child_index-1) < af->args.size()) {
                    if (af->args[ctxt->child_index-1]->write) arg->write = true;
                    if (af->args[ctxt->child_index-1]->read) arg->read = true; }
            } else if (prim->isOutput(ctxt->child_index)) {
                arg->write = true;
            } else {
                arg->read = true; } }
        if (self.actionArgUseTypes.count(getOriginal()))
            arg->type = combine(arg->srcInfo, arg->type, self.actionArgUseTypes[getOriginal()]);
        arg->type = combine(arg->srcInfo, arg->type, ctxtType());
        return true; }

 public:
    explicit Pass3(TypeCheck &s) : self(s) {}
};

TypeCheck::TypeCheck() : PassManager({
    new Pass1,
    new PassRepeated({
        new Pass2(*this),
        new Pass3(*this),
    })
}) {}

const IR::Node *TypeCheck::apply_visitor(const IR::Node *n, const char *name) {
    return PassManager::apply_visitor(n, name);
}
