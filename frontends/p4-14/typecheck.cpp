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
    const IR::Node *preorder(IR::V1Program *glob) override { global = glob; return glob; }
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
            ref->type = hst->elementType;
        else
            error("%s: %s is not a header", ref->base()->srcInfo, ref->base()->toString());
        return ref; }
    const IR::Node *postorder(IR::Member *ref) override {
        if (ref->member == "$valid") {
            ref->type = IR::Type::Boolean::get();
        } else if (auto ht = ref->expr->type->to<IR::Type_StructLike>()) {
            auto f = ht->getField(ref->member);
            if (f != nullptr) {
                ref->type = f->type;
                return ref; }
            error("%s: No field named %s in %s", ref->srcInfo, ref->member, ht->name); }
        return ref; }
};

static const IR::Type *combine(const Util::SourceInfo &loc, const IR::Type *a, const IR::Type *b) {
    if (!a || a == IR::Type::Unknown::get()) return b;
    if (!b || b == IR::Type::Unknown::get()) return a;
    if (a->is<IR::Type_InfInt>()) return b;
    if (b->is<IR::Type_InfInt>()) return a;
    if (*a == *b) return a;
    error("%s: Incompatible types %s and %s", loc, a->toString(), b->toString());
    return a;
}

// bottom up type inferencing -- set the types of expression nodes based on operands,
// - also records types of operands to action function calls for Pass3 to use
class TypeCheck::Pass2 : public Modifier {
    TypeCheck           &self;
    const IR::V1Program *global = nullptr;
    profile_t init_apply(const IR::Node *root) override {
        global = root->to<IR::V1Program>();
        self.actionArgUseTypes.clear();
        self.iterCounter++;
        return Modifier::init_apply(root); }
    void postorder(IR::Member *) override {}
    void postorder(IR::Operation_Unary *op) override {
        op->type = op->expr->type; }
    void postorder(IR::Operation_Binary *op) override {
        if (op->left->type->is<IR::Type_InfInt>())
            op->type = op->right->type;
        else if (op->right->type->is<IR::Type_InfInt>())
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
    profile_t init_apply(const IR::Node *root) override {
        global = root->to<IR::V1Program>();
        return Modifier::init_apply(root); }
    const IR::Type *ctxtType() {
        const IR::Type *rv = IR::Type::Unknown::get();
        const Context *ctxt = getContext();
        if (auto parent = ctxt->node->to<IR::Expression>()) {
            if (auto p = parent->to<IR::Operation_Relation>()) {
                if (ctxt->child_index == 0)
                    rv = p->right->type;
                else if (ctxt->child_index == 1)
                    rv = p->left->type;
                else
                    BUG("Unepxected child index");
            } else if (parent->is<IR::Operation::Binary>()) {
                rv = parent->type;
            } else if (!global) {
            } else if (auto prim = parent->to<IR::Primitive>()) {
                if (auto af = global->get<IR::ActionFunction>(prim->name)) {
                    if (size_t(ctxt->child_index) < af->args.size())
                        rv = af->args[ctxt->child_index]->type;
                } else if (auto infer = prim->inferOperandTypes()) {
                    if ((infer >> (ctxt->child_index)) & 1) {
                        for (auto o : prim->operands) {
                            if ((infer & 1) && o->type != rv) {
                                rv = o->type;
                                break; }
                            infer >>= 1; } } } } }
        return rv; }
    bool preorder(IR::Expression *op) override {
        if (op->type == IR::Type::Unknown::get() || op->type->is<IR::Type_InfInt>()) {
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
                if (size_t(ctxt->child_index) < af->args.size()) {
                    if (af->args[ctxt->child_index]->write) arg->write = true;
                    if (af->args[ctxt->child_index]->read) arg->read = true; }
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
    (new PassRepeated({
        new Pass2(*this),
        new Pass3(*this),
    }))->setRepeats(100)   // avoid infinite loop if there's a bug
}) { setStopOnError(true); }

const IR::Node *TypeCheck::apply_visitor(const IR::Node *n, const char *name) {
    LOG5("Before Typecheck:\n" << dumpToString(n));
    auto *rv = PassManager::apply_visitor(n, name);
    LOG5("After Typecheck:\n" << dumpToString(rv));
    return rv;
}
