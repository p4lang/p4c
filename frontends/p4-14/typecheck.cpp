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

/// P4-14 (v1.0 and v1.1) type checking algorithm
/// Initial type setting based on immediate context:
/// - replace named reference to ActionParams in the bodies of ActionFunctions with the
///   actual ActionParam
/// - link header/metadata instances to the Type
/// - replace named references to global header or metadata instances with ConcreteHeaderRef
///   expressions that link directly to them.
/// - set type for Member and HeaderStackItemRefs
class TypeCheck::AssignInitialTypes : public Transform {
 public:
    AssignInitialTypes() { setName("AssignInitialTypes"); }

 private:
    const IR::V1Program   *global = nullptr;

    template <typename NodeType, typename TypeType>
    void setType(NodeType* currentNode, const TypeType* type) {
        BUG_CHECK(currentNode == getCurrentNode<NodeType>(),
                  "Expected to be called on the visitor's current node");
        currentNode->type = type;
        if (type != getOriginal<NodeType>()->type)
            LOG3("Set initial type " << type << " for expression " << currentNode); }
    const IR::Node *preorder(IR::V1Program *glob) override { global = glob; return glob; }

    const IR::Node *preorder(IR::PathExpression *ref) override {
        if (auto af = findContext<IR::ActionFunction>())
            if (auto arg = af->arg(ref->path->name))
                return arg;
        const Visitor::Context *prop_ctxt = nullptr;
        if (auto prop = findContext<IR::Property>(prop_ctxt)) {
            if (auto bbox = prop_ctxt->parent->node->to<IR::Declaration_Instance>()) {
                if (auto bbox_type = bbox->type->to<IR::Type_Extern>()) {
                    auto attr = bbox_type->attributes.get<IR::Attribute>(prop->name);
                    if (attr && attr->locals && attr->locals->locals.count(ref->path->name)) {
                        return attr->locals->locals.at(ref->path->name); } } } }
        if (auto bbox = findContext<IR::Declaration_Instance>()) {
            if (auto bbox_type = bbox->type->to<IR::Type_Extern>()) {
                if (auto attr = bbox_type->attributes.get<IR::Attribute>(ref->path->name))
                    return new IR::AttributeRef(ref->srcInfo, attr->type,
                                                bbox->name, bbox_type, attr);
            } else if (global && !bbox->type->is<IR::Type_Name>()) {
                BUG("extern type is not extern_type?"); } }
        return ref; }

    const IR::Node *preorder(IR::Metadata *m) override {
        if (!global) return m;
        if (auto ht = global->get<IR::v1HeaderType>(m->type_name))
            setType(m, ht->as_metadata);
        else
            error("%s: No header type %s defined", m->srcInfo, m->type_name);
        return m; }

    const IR::Node *preorder(IR::BoolLiteral* b) override {
        setType(b, IR::Type_Boolean::get());
        return b; }

    const IR::Node *preorder(IR::HeaderOrMetadata *hm) override {
        if (!global) return hm;
        if (auto ht = global->get<IR::v1HeaderType>(hm->type_name))
            setType(hm, ht->as_header);
        else
            error("%s: No header type %s defined", hm->srcInfo, hm->type_name);
        return hm; }

    const IR::Node *preorder(IR::ActionSelector *sel) override {
        if (!global) return sel;
        if (sel->key_fields != nullptr) return sel;
        const IR::FieldListCalculation *kf = nullptr;
        if (sel->key.name && (kf = global->get<IR::FieldListCalculation>(sel->key)))
            sel->key_fields = kf;
        else
            error("%s: Key must coordinate to a field_list_calculation", sel->srcInfo);
        return sel; }

    const IR::Node *preorder(IR::FieldListCalculation *flc) override {
        if (!global) return flc;
        if (flc->input_fields != nullptr) return flc;
        if (!flc->input || flc->input->names.empty()) {
            error("%s: no input in field_list_calculation", flc->srcInfo);
            return flc; }
        const IR::FieldList *in_f = nullptr;
        if (flc->input->names.size() == 1 &&
            (in_f = global->get<IR::FieldList>(flc->input->names[0]))) {
            flc->input_fields = in_f;
        } else {
            auto fl = new IR::FieldList();
            for (auto &name : flc->input->names) {
                fl->fields.push_back(new IR::PathExpression(name));
                fl->srcInfo += name.srcInfo; }
            flc->input_fields = fl; }
        return flc; }

    template<typename T> void prop_update(IR::Property *prop, const char *tname) {
        auto ev = prop->value->to<IR::ExpressionValue>();
        auto pe = ev ? ev->expression->to<IR::PathExpression>() : nullptr;
        if (auto t = pe ? global->get<T>(pe->path->name) : nullptr) {
            prop->value = new IR::ExpressionValue(ev->srcInfo,
                            new IR::GlobalRef(pe->srcInfo, t));
        } else {
            error("property %s must be a %s", prop, tname); } }
    const IR::Node *preorder(IR::Property *prop) override {
        if (auto di = findContext<IR::Declaration_Instance>()) {
            auto ext = di->type->to<IR::Type_Extern>();
            if (!ext && !global) return prop;
            if (!ext && di->type->is<IR::Type_Name>()) return prop;
            BUG_CHECK(ext, "%s is not an extern", di);
            if (auto attr = ext->attributes[prop->name]) {
                if (attr->type->is<IR::Type::String>())
                    prune();
                else if (attr->type->is<IR::Type_AnyTable>())
                    prop_update<IR::V1Table>(prop, "table");
                else if (attr->type->is<IR::Type_Counter>())
                    prop_update<IR::Counter>(prop, "counter");
                else if (attr->type->is<IR::Type_Meter>())
                    prop_update<IR::Meter>(prop, "meter");
                else if (attr->type->is<IR::Type_Register>())
                    prop_update<IR::Register>(prop, "register");
            } else {
                error("No property name %s in extern %s", prop->name, ext->name); } }
        return prop; }

    const IR::Node *postorder(IR::PathExpression *ref) override {
        if (!global) return ref;
        IR::Node *new_node = ref;
        // There might be multiple different objects in the program with this name.
        // If there are, we need to infer which to use based on the context.
        auto hdr = global->get<IR::HeaderOrMetadata>(ref->path->name);
        auto obj = global->get<IR::IInstance>(ref->path->name);
        auto ext = global->get<IR::Declaration_Instance>(ref->path->name);
        bool preferHdr = getParent<IR::Member>()     // context is a member reference
                      || getParent<IR::HeaderStackItemRef>();  // or header stack
        auto prim = getParent<IR::Primitive>();         // context is a primitive call
        static const std::set<cstring> header_prims = {
            // primitives that operate on headers or header stacks
            "add_header", "copy_header", "emit", "extract", "push", "pop",
            "remove_header", "valid" };
        if (hdr && (!obj || preferHdr || (prim && header_prims.count(prim->name)))) {
            // prefer header to object only if in a simple member reference or in a
            // header stack reference or a primitive that is valid on headers.
            visit(hdr);
            new_node = new IR::ConcreteHeaderRef(ref->srcInfo, hdr);
        } else if (obj) {
            if (prim && ext && ext != obj && getContext()->child_index == 0) {
                // prefer a blackbox if it has the method in question
                if (auto et = ext->type->to<IR::Type_Extern>()) {
                    for (auto m : et->methods) {
                        if (m->name == prim->name) {
                            obj = ext;
                            break; } } } }
            const IR::Node *tmp = obj->getNode();  // FIXME -- can't visit an interface directly
            visit(tmp);
            obj = tmp->to<IR::IInstance>();
            new_node = new IR::GlobalRef(ref->srcInfo, obj->getType(), tmp);
        } else if (/*auto obj = */global->get<IR::Node>(ref->path->name)) {
            /* FIXME -- is something, should probably be typechecked */
        } else if (getParent<IR::Member>()) {
            if (ref->path->name != "latest")
                error("%s: No header or metadata named %s", ref->srcInfo, ref->path->name);
        } else {
            if (getParent<IR::HeaderStackItemRef>()) {
                if (ref->path->name == "next" || ref->path->name == "last")
                    return ref; }
            if (auto hdr = findContext<IR::Type_StructLike>()) {
                if (auto field = hdr->getField(ref->path->name)) {
                    /* FIXME --  Should this be converted to something else? */
                    setType(ref, field->type);
                    visit(ref->type);
                    return ref; } }
            error("%s: No defintion for %s", ref->srcInfo, ref->path->name); }
        return new_node; }

    const IR::Node *postorder(IR::Type_Name *ref) override {
        if (!global) return ref;
        if (auto t = global->get<IR::Type>(ref->path->name)) {
            visit(t);
            return t;
        } else {
            error("%s: No defintion for %s", ref->srcInfo, ref->path->name); }
        return ref; }

    const IR::Node *postorder(IR::HeaderStackItemRef *ref) override {
        if (auto ht = ref->base()->type->to<IR::Type_StructLike>())
            setType(ref, ht);
        else if (auto hst = ref->base()->type->to<IR::Type_Stack>())
            setType(ref, hst->elementType);
        else
            error("%s: %s is not a header", ref->base()->srcInfo, ref->base()->toString());
        visit(ref->type);
        return ref; }

    const IR::Node *postorder(IR::Member *ref) override {
        if (ref->member.toString()[0] == '$') {
            if (ref->member == "$valid")
                setType(ref, IR::Type::Boolean::get());
        } else if (auto ht = ref->expr->type->to<IR::Type_StructLike>()) {
            auto f = ht->getField(ref->member);
            if (f != nullptr) {
                setType(ref, f->type);
                visit(ref->type);
                return ref; }
            error("%s: No field named %s in %s", ref->srcInfo, ref->member, ht->name); }
        return ref; }

    const IR::Node *postorder(IR::Expression *e) override {
        visit(e->type);
        return e; }
};

static const IR::Type*
combineTypes(const Util::SourceInfo &loc, const IR::Type *a, const IR::Type *b) {
    if (!a || a == IR::Type::Unknown::get()) return b;
    if (!b || b == IR::Type::Unknown::get()) return a;
    if (a->is<IR::Type_InfInt>()) return b;
    if (b->is<IR::Type_InfInt>()) return a;
    if (*a == *b) return a;
    if (a->is<IR::Type_Bits>() && b->is<IR::Type_Bits>()) {
        auto aBits = a->to<IR::Type_Bits>();
        auto bBits = b->to<IR::Type_Bits>();
        if (aBits->isSigned != bBits->isSigned) {
            error("%s: Types %s and %s differ in signedness",
                  loc, a->toString(), b->toString());
            return a;
        }
        return IR::Type_Bits::get(std::max(aBits->width_bits(), bBits->width_bits()),
                                  aBits->isSigned);
    }
    error("%s: Incompatible types %s and %s", loc, a->toString(), b->toString());
    return a;
}

static IR::Cast *makeP14Cast(const IR::Type *type, const IR::Expression *exp) {
    auto t1 = type->to<IR::Type::Bits>(), t2 = exp->type->to<IR::Type::Bits>();
    if (t1 && t2 && t1->size != t2->size && t1->isSigned != t2->isSigned) {
        // P4_16 does not allow bit cast to different size AND signedness in one step,
        // while P4_14 does, so we insert an extra intermediate cast
        exp = new IR::Cast(IR::Type::Bits::get(t1->size, t2->isSigned), exp); }
    return new IR::Cast(type, exp);
}

/// Bottom up type inferencing -- set the types of expression nodes based on operands.
class TypeCheck::InferExpressionsBottomUp : public Modifier {
 public:
    InferExpressionsBottomUp() { setName("InferExpressionsBottomUp"); }

 private:
    void setType(IR::Expression* currentNode, const IR::Type* type) {
        BUG_CHECK(currentNode == getCurrentNode<IR::Expression>(),
                  "Expected to be called on the visitor's current node");
        currentNode->type = type;
        if (type != getOriginal<IR::Expression>()->type)
            LOG3("Inferred (up) type " << type << " for expression " << currentNode <<
                 " [" << currentNode->id <<"]");
    }
    bool checkBits(const IR::Node* node, const IR::Type* type) const {
        if (type->is<IR::Type::Unknown>() || type->is<IR::Type::Bits>() ||
            type->is<IR::Type_InfInt>())
            return true;
        ::error("%1%: not defined on operands of type %2%", node, type);
        return false;
    }

    void postorder(IR::Operation_Binary *op) override {
        checkBits(op, op->left->type) && checkBits(op, op->right->type);
        if (op->left->type->is<IR::Type_InfInt>()) {
            setType(op, op->right->type);
        } else if (op->right->type->is<IR::Type_InfInt>()) {
            setType(op, op->left->type);
        } else if (op->left->type == op->right->type) {
            setType(op, op->left->type);
        } else {
            auto *lt = op->left->type->to<IR::Type::Bits>();
            auto *rt = op->right->type->to<IR::Type::Bits>();
            if (lt && rt) {
                if (lt->size < rt->size) {
                    setType(op, rt);
                    op->left = makeP14Cast(rt, op->left);
                } else if (rt->size < lt->size) {
                    setType(op, lt);
                    op->right = makeP14Cast(lt, op->right); }
                LOG3("Inserted cast in " << op);
            }
        }
    }
    void logic_operand(const IR::Expression *&op) {
        if (auto *bit = op->type->to<IR::Type::Bits>()) {
            LOG3("Inserted bool conversion for " << op);
            op = new IR::Neq(IR::Type::Boolean::get(), op, new IR::Constant(bit, 0));
        } }
    void postorder(IR::LAnd *op) override {
        logic_operand(op->left);
        logic_operand(op->right);
        setType(op, IR::Type::Boolean::get()); }
    void postorder(IR::LOr *op) override {
        logic_operand(op->left);
        logic_operand(op->right);
        setType(op, IR::Type::Boolean::get()); }
    void postorder(IR::LNot *op) override {
        logic_operand(op->expr);
        setType(op, IR::Type::Boolean::get()); }
    void postorder(IR::Operation_Relation *op) override {
        setType(op, IR::Type::Boolean::get()); }
};

static const IR::Type*
inferTypeFromContext(const Visitor::Context* ctxt, const IR::V1Program* global) {
    const IR::Type *rv = IR::Type::Unknown::get();
    if (!ctxt) return rv;
    if (auto parent = ctxt->node->to<IR::Expression>()) {
        if (auto p = parent->to<IR::Operation_Relation>()) {
            const IR::Type::Bits *t, *ct = nullptr;
            if (ctxt->child_index == 0) {
                rv = p->right->type;
                ct = p->left->type->to<IR::Type::Bits>();
            } else if (ctxt->child_index == 1) {
                rv = p->left->type;
                ct = p->right->type->to<IR::Type::Bits>();
            } else {
                BUG("Unepxected child index"); }
            if (ct && (t = rv->to<IR::Type::Bits>()) && ct->size > t->size)
                // if both children have bit types, use the larger
                rv = ct;
        } else if (parent->is<IR::Operation::Binary>()) {
            if ((parent->is<IR::Shl>() || parent->is<IR::Shr>()) && ctxt->child_index == 1) {
                // don't propagate into shift count -- maybe should infer log2 bits?
            } else {
                rv = parent->type; }
        } else if (parent->is<IR::Neg>() || parent->is<IR::Cmpl>()) {
            rv = parent->type;
        } else if (!global) {
        } else if (auto prim = parent->to<IR::Primitive>()) {
            if (auto af = global->get<IR::ActionFunction>(prim->name)) {
                if (size_t(ctxt->child_index) < af->args.size()) {
                    rv = af->args[ctxt->child_index]->type;
                }
            } else if (auto infer = prim->inferOperandType(ctxt->child_index)) {
                rv = infer; } } }
    return rv;
}

/// Top down type inferencing -- set the type of expression nodes based on their uses.
class TypeCheck::InferExpressionsTopDown : public Modifier {
 public:
    InferExpressionsTopDown() { setName("InferExpressionsTopDown"); }
 private:
    const IR::V1Program *global = nullptr;
    profile_t init_apply(const IR::Node *root) override {
        global = root->to<IR::V1Program>();
        return Modifier::init_apply(root); }
    bool preorder(IR::ActionArg *) override { return false; }  // don't infer these yet
    bool preorder(IR::Expression *op) override {
        if (op->type == IR::Type::Unknown::get() || op->type->is<IR::Type_InfInt>()) {
            auto *type = inferTypeFromContext(getContext(), global);
            if (type != IR::Type::Unknown::get() &&
                type != getOriginal<IR::Expression>()->type) {
                op->type = type;
                LOG3("Inferred (down) type " << type << " for expression " << op <<
                     " [" << op->id <<"]");
            }
        }
        return true; }
};

class TypeCheck::InferActionArgsBottomUp : public Inspector {
    TypeCheck           &self;
    const IR::V1Program *global = nullptr;
    profile_t init_apply(const IR::Node *root) override {
        global = root->to<IR::V1Program>();
        self.actionArgUseTypes.clear();
        self.iterCounter++;
        return Inspector::init_apply(root); }
    void postorder(const IR::Primitive *prim) override {
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
                        combineTypes(prim->srcInfo, self.actionArgUseTypes[*arg], op->type);
                ++arg; }
            if (arg != af->args.end() && self.iterCounter == 1)
                error("%s: not enough arguments to action %s", prim->srcInfo, prim);
        } else if (self.iterCounter == 1) {
            prim->typecheck(); } }

 public:
    explicit InferActionArgsBottomUp(TypeCheck &s) : self(s) { setName("InferActionArgsBottomUp"); }
};

class TypeCheck::InferActionArgsTopDown : public Inspector {
    TypeCheck           &self;
    const IR::V1Program *global = nullptr;
    profile_t init_apply(const IR::Node *root) override {
        global = root->to<IR::V1Program>();
        return Inspector::init_apply(root); }
    bool preorder(const IR::ActionArg *arg) override {
        // Start with any type we may have already assigned to this argument in
        // a previous run of AssignActionArgTypes.
        auto type = arg->type;

        // Combine that with any type we computed in the bottom-up pass we just
        // completed, or in a previous visit to this node within this pass.
        auto bottomUpType = self.actionArgUseTypes.count(arg)
                          ? self.actionArgUseTypes[arg]
                          : IR::Type::Unknown::get();
        type = combineTypes(arg->srcInfo, type, bottomUpType);

        // Combine that with whatever type information we can infer from the
        // context (e.g., if this argument is being used as an operand to a
        // primitive, we know the type we expect for that operand).
        auto contextType = inferTypeFromContext(getContext(), global);
        type = combineTypes(arg->srcInfo, type, contextType);

        self.actionArgUseTypes[arg] = type;
        return true;
    }

 public:
    explicit InferActionArgsTopDown(TypeCheck &s) : self(s) {
        // In the AssignInitialTypes pass, we replaced all nodes which refer to
        // action arguments with the action argument they refer to. This means
        // that the same action argument node may appear in the IR tree multiple
        // times if it's referenced more than once within the action body.
        // Because we may infer different types from different uses of the
        // action argument, we need to visit the action argument nodes every
        // time they appear in the IR tree.
        visitDagOnce = false;
        setName("InferActionArgsTopDown");
    }
};

class TypeCheck::AssignActionArgTypes : public Modifier {
    TypeCheck           &self;
    const IR::V1Program *global = nullptr;
    profile_t init_apply(const IR::Node *root) override {
        global = root->to<IR::V1Program>();
        return Modifier::init_apply(root); }
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

        // Assign the type we computed in the previous passes.
        BUG_CHECK(self.actionArgUseTypes.count(getOriginal()) > 0,
                  "Didn't compute a type for action arg %1%", getOriginal());
        auto type = self.actionArgUseTypes[getOriginal()];
        if (type != getOriginal<IR::ActionArg>()->type) {
            arg->type = type;
            LOG3("Inferred type " << arg->type << " for action argument " << arg <<
                 " [" << arg->id <<"]");
        }

        return true;
    }

 public:
    explicit AssignActionArgTypes(TypeCheck &s) : self(s) { setName("AssignActionArgTypes"); }
};

class TypeCheck::MakeImplicitCastsExplicit : public Transform, P4WriteContext {
    const IR::V1Program *global = nullptr;
    profile_t init_apply(const IR::Node *root) override {
        global = root->to<IR::V1Program>();
        return Transform::init_apply(root); }
    IR::Annotation *preorder(IR::Annotation *a) override { prune(); return a; }
    IR::Expression *postorder(IR::Expression *op) override {
        visitAgain();
        if (isWrite()) return op;  // don't cast lvalues
        auto *type = inferTypeFromContext(getContext(), global);
        if (type != IR::Type::Unknown::get() && !type->is<IR::Type_InfInt>() && type != op->type) {
                LOG3("Need cast " << op->type << " -> " << type << " on " << op);
                const IR::Expression *e = getOriginal<IR::Expression>();
                if (*op != *e) e = op;  // undo Transform clone if it wasn't needed
                return makeP14Cast(type, e); }
        return op; }

 public:
    MakeImplicitCastsExplicit() { setName("MakeImplicitCastsExplicit"); }
};

TypeCheck::TypeCheck() : PassManager({
    new AssignInitialTypes,
    (new PassRepeated({
        new InferExpressionsBottomUp,
        new InferExpressionsTopDown,
        new InferActionArgsBottomUp(*this),
        new InferActionArgsTopDown(*this),
        new AssignActionArgTypes(*this)
    }))->setRepeats(100),  // avoid infinite loop if there's a bug
    new MakeImplicitCastsExplicit
}) { setStopOnError(true); setName("TypeCheck"); }

const IR::Node *TypeCheck::apply_visitor(const IR::Node *n, const char *name) {
    LOG5("Before Typecheck:\n" << dumpToString(n));
    auto *rv = PassManager::apply_visitor(n, name);
    LOG5("After Typecheck:\n" << dumpToString(rv));
    return rv;
}
