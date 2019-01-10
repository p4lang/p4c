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

#include "converters.h"
#include "programStructure.h"
#include "v1model.h"
#include "lib/gmputil.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4-14/header_type.h"
#include "frontends/p4-14/typecheck.h"

namespace P4V1 {

const IR::Type*
ExpressionConverter::getFieldType(const IR::Type_StructLike* ht, cstring fieldName) {
    auto field = ht->getField(fieldName);
    if (field != nullptr)
        return field->type;
    BUG("Cannot find field %1% in type %2%", fieldName, ht);
}

// These can only come from the key of a table.
const IR::Node* ExpressionConverter::postorder(IR::Mask* expression) {
    if (!expression->right->is<IR::Constant>()) {
        ::error("%1%: Mask must be a constant", expression->right);
        return expression;
    }

    auto exp = expression->left;
    auto cst = expression->right->to<IR::Constant>();
    mpz_class value = cst->value;
    if (value == 0) {
        ::warning(ErrorType::WARN_INVALID, "%1%: zero mask", expression->right);
        return cst;
    }
    auto range = Util::findOnes(value);
    if (range.lowIndex == 0 && range.highIndex >= exp->type->width_bits() - 1U)
        return exp;
    if (value != range.value)
        return new IR::BAnd(expression->srcInfo, exp, cst);
    return new IR::Slice(exp,
                         new IR::Constant(expression->srcInfo, range.highIndex),
                         new IR::Constant(expression->srcInfo, range.lowIndex));
}

const IR::Node* ExpressionConverter::postorder(IR::Constant* expression) {
    // The P4-14 front-end may have constants that overflow their declared type,
    // since the v1 type inference sets types to constants without any checks.
    // We fix this here.
    if (expression->type->is<IR::Type::Boolean>())
        return new IR::BoolLiteral(expression->srcInfo, expression->type, expression->value != 0);
    else
        return new IR::Constant(expression->srcInfo, expression->type,
                                expression->value, expression->base);
}

const IR::Node* ExpressionConverter::postorder(IR::FieldList* fl) {
    // Field lists may contain other field lists
    if (auto func = get(std::type_index(typeid(*fl)).name())) {
        return func(fl); }
    return new IR::ListExpression(fl->srcInfo, fl->fields);
}

const IR::Node* ExpressionConverter::postorder(IR::Member* field) {
    if (auto ht = field->expr->type->to<IR::Type_StructLike>())
        field->type = getFieldType(ht, field->member.name);
    return field;
}

const IR::Node* ExpressionConverter::postorder(IR::ActionArg* arg) {
    auto result = new IR::PathExpression(arg->name);
    result->type = arg->type;
    return result;
}

const IR::Node* ExpressionConverter::postorder(IR::Primitive* primitive) {
    if (primitive->name == "current") {
        // current(a, b) => packet.lookahead<bit<a+b>>()[b-1,0]
        BUG_CHECK(primitive->operands.size() == 2, "Expected 2 operands for %1%", primitive);
        auto a = primitive->operands.at(0);
        auto b = primitive->operands.at(1);
        if (!a->is<IR::Constant>() || !b->is<IR::Constant>()) {
            ::error("%1%: must have constant arguments", primitive);
            return primitive;
        }
        auto aval = a->to<IR::Constant>()->asInt();
        auto bval = b->to<IR::Constant>()->asInt();
        if (aval < 0 || bval <= 0) {
            ::error("%1%: negative offsets?", primitive);
            return primitive;
        }

        const IR::Expression* method = new IR::Member(
            structure->paramReference(structure->parserPacketIn),
            P4::P4CoreLibrary::instance.packetIn.lookahead.Id());
        auto typeargs = new IR::Vector<IR::Type>();
        typeargs->push_back(IR::Type_Bits::get(aval + bval));
        auto lookahead = new IR::MethodCallExpression(method, typeargs);
        auto result = new IR::Slice(primitive->srcInfo, lookahead,
                                    new IR::Constant(bval - 1),
                                    new IR::Constant(0));
        result->type = IR::Type_Bits::get(bval);
        return result;
    } else if (primitive->name == "valid") {
        BUG_CHECK(primitive->operands.size() == 1, "Expected 1 operand for %1%", primitive);
        auto base = primitive->operands.at(0);
        auto method = new IR::Member(primitive->srcInfo, base, IR::ID(IR::Type_Header::isValid));
        auto result = new IR::MethodCallExpression(primitive->srcInfo, IR::Type::Boolean::get(),
                                                   method);
        return result;
    }
    BUG("Unexpected primitive %1%", primitive);
}

const IR::Node* ExpressionConverter::postorder(IR::PathExpression *ref) {
    if (ref->path->name.name == "latest") {
        if (structure->latest == nullptr) {
            ::error("%1%: latest not yet defined", ref);
            return ref;
        }
        return structure->latest;
    }
    if (ref->path->name.name == "next") {
        return ref;
    }
    if (auto fl = structure->field_lists.get(ref->path->name)) {
        ExpressionConverter conv(structure);
        return conv.convert(fl); }
    if (auto flc = structure->field_list_calculations.get(ref->path->name)) {
        // FIXME -- what to do with the algorithm and width from flc?
        return ExpressionConverter(structure).convert(flc->input_fields);
    }
    return ref;
}

const IR::Node* ExpressionConverter::postorder(IR::ConcreteHeaderRef* nhr) {
    const IR::Expression* ref;
    if (structure->isHeader(nhr)) {
        ref = structure->conversionContext.header->clone();
    } else {
        if (nhr->ref->name == P4V1::V1Model::instance.standardMetadata.name)
            return structure->conversionContext.standardMetadata->clone();
        else
            ref = structure->conversionContext.userMetadata->clone();
    }
    auto result = new IR::Member(nhr->srcInfo, ref, nhr->ref->name);
    result->type = nhr->type;
    return result;
}

const IR::Node* ExpressionConverter::postorder(IR::HeaderStackItemRef* ref) {
    if (ref->index_->is<IR::PathExpression>()) {
        auto nr = ref->index_->to<IR::PathExpression>();
        if (nr->path->name == "last" || nr->path->name == "next") {
            cstring name = nr->path->name == "last" ? IR::Type_Stack::last : IR::Type_Stack::next;
            if (replaceNextWithLast && name == IR::Type_Stack::next)
                name = IR::Type_Stack::last;
            auto result = new IR::Member(ref->srcInfo, ref->base_, name);
            result->type = ref->base_->type;
            return result;
        }
    } else if (ref->index_->is<IR::Constant>()) {
        auto result = new IR::ArrayIndex(ref->srcInfo, ref->base_, ref->index_);
        result->type = ref->base_->type;
        return result;
    }

    ::error("Illegal array index %1%: must be a constant, 'last', or 'next'.", ref);
    return ref;
}

const IR::Node* ExpressionConverter::postorder(IR::GlobalRef *ref) {
    // FIXME -- this is broken when the GlobalRef refers to something that the converter
    // FIXME -- has put into a different control.  In that case, ResolveReferences on this
    // FIXME -- path will later fail as the declaration is not in scope.  We should at
    // FIXME -- least detect that here and give a warning or other indication of the problem.
    return new IR::PathExpression(ref->srcInfo,
            new IR::Path(ref->srcInfo, IR::ID(ref->srcInfo, ref->toString())));
}

/// P4_16 is stricter on comparing booleans with ints
/// Therefore we convert such expressions into simply the boolean test
const IR::Node* ExpressionConverter::postorder(IR::Equ *equ) {
    const IR::Expression *boolExpr = nullptr;
    const IR::Expression *constExpr = nullptr;
    if (equ->left->type->is<IR::Type_Boolean>() && equ->right->is<IR::Constant>()) {
        boolExpr = equ->left;
        constExpr = equ->right;
    } else if (equ->right->type->is<IR::Type_Boolean>() && equ->left->is<IR::Constant>()) {
        boolExpr = equ->right;
        constExpr = equ->left;
    }

    // not a case we support
    if (boolExpr == nullptr)
        return equ;

    auto val = constExpr->to<IR::Constant>()->asInt();
    if (val == 1)
        return boolExpr;  // == 1 return the boolean
    else if (val == 0)
        return new IR::LNot(equ->srcInfo, boolExpr);  // return the !boolean
    else
        return new IR::BoolLiteral(equ->srcInfo, false);  // everything else is false
}

/// And the Neq
const IR::Node* ExpressionConverter::postorder(IR::Neq *neq) {
    const IR::Expression *boolExpr = nullptr;
    const IR::Expression *constExpr = nullptr;
    if (neq->left->type->is<IR::Type_Boolean>() && neq->right->is<IR::Constant>()) {
        boolExpr = neq->left;
        constExpr = neq->right;
    } else if (neq->right->type->is<IR::Type_Boolean>() && neq->left->is<IR::Constant>()) {
        boolExpr = neq->right;
        constExpr = neq->left;
    }

    if (boolExpr == nullptr)
        return neq;

    auto val = constExpr->to<IR::Constant>()->asInt();
    if (val == 0)
        return boolExpr;
    else if (val == 1)
        return new IR::LNot(neq->srcInfo, boolExpr);
    else
        return new IR::BoolLiteral(neq->srcInfo, true);  // everything else is true
}

std::map<cstring, ExpressionConverter::funcType>* ExpressionConverter::cvtForType = nullptr;

void ExpressionConverter::addConverter(cstring type, ExpressionConverter::funcType cvt) {
    static std::map<cstring, ExpressionConverter::funcType> tbl;
    cvtForType = &tbl;
    tbl[type] = cvt;
}

ExpressionConverter::funcType ExpressionConverter::get(cstring type) {
    if (cvtForType && cvtForType->count(type))
        return cvtForType->at(type);
    return nullptr;
}

const IR::Node* StatementConverter::preorder(IR::Apply* apply) {
    auto table = structure->tables.get(apply->name);
    auto newname = structure->tables.get(table);
    auto tbl = new IR::PathExpression(newname);
    auto method = new IR::Member(apply->srcInfo, tbl, IR::ID(IR::IApply::applyMethodName));
    auto call = new IR::MethodCallExpression(apply->srcInfo, method);
    if (apply->actions.size() == 0) {
        auto stat = new IR::MethodCallStatement(apply->srcInfo, call);
        prune();
        return stat;
    } else {
        const IR::Vector<IR::Expression>* hit = nullptr;
        const IR::Vector<IR::Expression>* miss = nullptr;
        bool otherLabels = false;
        for (auto a : apply->actions) {
            if (a.first == "hit") {
                if (hit != nullptr)
                    ::error("%1%: Duplicate 'hit' label", hit);
                hit = a.second;
            } else if (a.first == "miss") {
                if (miss != nullptr)
                    ::error("%1%: Duplicate 'miss' label", hit);
                miss = a.second;
            } else {
                otherLabels = true;
            }
        }

        if ((hit != nullptr || miss != nullptr) && otherLabels)
            ::error("%1%: Cannot mix 'hit'/'miss' and other labels", apply);

        if (!otherLabels) {
            StatementConverter conv(structure, renameMap);
            auto hitcase = hit ? conv.convert(hit) : new IR::EmptyStatement();
            auto misscase = miss ? conv.convert(miss) : nullptr;
            auto check = new IR::Member(call, IR::Type_Table::hit);
            auto ifstat = new IR::IfStatement(apply->srcInfo, check, hitcase, misscase);
            prune();
            return ifstat;
        } else {
            IR::Vector<IR::SwitchCase> cases;
            std::map<const IR::Vector<IR::Expression> *, int> converted;
            for (auto a : apply->actions) {
                StatementConverter conv(structure, renameMap);
                const IR::Statement *stat = nullptr;
                auto insert_at = cases.end();
                if (converted.count(a.second)) {
                    insert_at = cases.begin() + converted.at(a.second);
                } else {
                    converted[a.second] = cases.size();
                    stat = conv.convert(a.second); }
                const IR::Expression* destination;
                if (a.first == "default") {
                    destination = new IR::DefaultExpression();
                } else {
                    cstring act_name = a.first;
                    cstring full_name = table->name + '.' + act_name;
                    if (renameMap->count(full_name))
                        act_name = renameMap->at(full_name);
                    destination = new IR::PathExpression(IR::ID(act_name)); }
                auto swcase = new IR::SwitchCase(a.second->srcInfo, destination, stat);
                cases.insert(insert_at, swcase); }
            auto check = new IR::Member(call, IR::Type_Table::action_run);
            auto sw = new IR::SwitchStatement(apply->srcInfo, check, std::move(cases));
            prune();
            return sw;
        }
    }
}

const IR::Node* StatementConverter::preorder(IR::Primitive* primitive) {
    auto control = structure->controls.get(primitive->name);
    if (control != nullptr) {
        auto instanceName = ::get(renameMap, control->name);
        auto ctrl = new IR::PathExpression(IR::ID(instanceName));
        auto method = new IR::Member(ctrl, IR::ID(IR::IApply::applyMethodName));
        auto args = new IR::Vector<IR::Argument>();
        args->push_back(new IR::Argument(
            structure->conversionContext.header->clone()));
        args->push_back(new IR::Argument(
            structure->conversionContext.userMetadata->clone()));
        args->push_back(new IR::Argument(
            structure->conversionContext.standardMetadata->clone()));
        auto call = new IR::MethodCallExpression(primitive->srcInfo, method, args);
        auto stat = new IR::MethodCallStatement(primitive->srcInfo, call);
        return stat;
    }
    // FIXME -- always a noop as ExpressionConverter has only postorder methods?
    return ExpressionConverter::preorder(primitive);
}

const IR::Node* StatementConverter::preorder(IR::If* cond) {
    StatementConverter conv(structure, renameMap);

    auto pred = apply_visitor(cond->pred)->to<IR::Expression>();
    BUG_CHECK(pred != nullptr, "Expected to get an expression when converting %1%", cond->pred);
    const IR::Statement* t, *f;
    if (cond->ifTrue == nullptr)
        t = new IR::EmptyStatement();
    else
        t = conv.convert(cond->ifTrue);

    if (cond->ifFalse == nullptr)
        f = nullptr;
    else
        f = conv.convert(cond->ifFalse);

    prune();
    auto result = new IR::IfStatement(cond->srcInfo, pred, t, f);
    return result;
}

const IR::Statement* StatementConverter::convert(const IR::Vector<IR::Expression>* toConvert) {
    auto stats = new IR::IndexedVector<IR::StatOrDecl>();
    for (auto e : *toConvert) {
        auto s = convert(e);
        stats->push_back(s);
    }
    auto result = new IR::BlockStatement(toConvert->srcInfo, *stats);
    return result;
}

const IR::Type_Varbits *TypeConverter::postorder(IR::Type_Varbits *vbtype) {
    if (vbtype->size == 0) {
        if (auto type = findContext<IR::Type_StructLike>()) {
            if (auto max = type->getAnnotation("max_length")) {
                if (max->expr.size() != 1 || !max->expr[0]->is<IR::Constant>())
                    error("%s: max_length must be a constant", max);
                else
                    vbtype->size = 8 * max->expr[0]->to<IR::Constant>()->asInt() -
                                   type->width_bits(); } } }
    if (vbtype->size == 0)
        error("%s: no max_length for * field size", vbtype);
    return vbtype;
}

const IR::StructField *TypeConverter::postorder(IR::StructField *field) {
    if (!field->type->is<IR::Type_Varbits>()) return field;
    // given a struct with length and max_length, the
    // varbit field size is max_length * 8 - struct_size
    if (auto type = findContext<IR::Type_StructLike>()) {
        if (auto len = type->getAnnotation("length")) {
            if (len->expr.size() == 1) {
                auto lenexpr = len->expr[0];
                auto scale = new IR::Mul(lenexpr->srcInfo, lenexpr, new IR::Constant(8));
                auto fieldlen = new IR::Sub(
                    scale->srcInfo, scale, new IR::Constant(type->width_bits()));
                field->annotations = field->annotations->add(
                    new IR::Annotation("length", { fieldlen }));
            }
        }
    }
    return field;
}

const IR::Type_StructLike *TypeConverter::postorder(IR::Type_StructLike *str) {
    str->annotations = str->annotations->where([](const IR::Annotation *a) -> bool {
        return a->name != "length" && a->name != "max_length"; });
    return str;
}

///////////////////////////////////////////////////////////////

namespace {
class FixupExtern : public Modifier {
    ProgramStructure            *structure;
    cstring                     origname, extname;
    IR::TypeParameters          *typeParams = nullptr;

    bool preorder(IR::Type_Extern *type) override {
        BUG_CHECK(!origname, "Nested extern");
        origname = type->name;
        return true; }
    void postorder(IR::Type_Extern *type) override {
        if (extname != type->name) {
            type->annotations = type->annotations->addAnnotationIfNew(
                IR::Annotation::nameAnnotation, new IR::StringLiteral(type->name.name));
            type->name = extname; }
        // FIXME -- should create ctors based on attributes?  For now just create a
        // FIXME -- 0-arg one if needed
        if (!type->lookupMethod(type->name, new IR::Vector<IR::Argument>())) {
            type->methods.push_back(new IR::Method(type->name, new IR::Type_Method(
                                                new IR::ParameterList()))); } }
    void postorder(IR::Method *meth) override {
        if (meth->name == origname) meth->name = extname; }
    // Convert extern methods that take a field_list_calculation to take a type param instead
    bool preorder(IR::Type_MethodBase *mtype) override {
        BUG_CHECK(!typeParams, "recursion failure");
        typeParams = mtype->typeParameters->clone();
        return true; }
    bool preorder(IR::Parameter *param) override {
        BUG_CHECK(typeParams, "recursion failure");
        if (param->type->is<IR::Type_FieldListCalculation>()) {
            auto n = new IR::Type_Var(structure->makeUniqueName("FL"));
            param->type = n;
            typeParams->push_back(n); }
        return false; }
    void postorder(IR::Type_MethodBase *mtype) override {
        BUG_CHECK(typeParams, "recursion failure");
        if (*typeParams != *mtype->typeParameters)
            mtype->typeParameters = typeParams;
        typeParams = nullptr; }

 public:
    FixupExtern(ProgramStructure *s, cstring n) : structure(s), extname(n) {}
};
}  // end anon namespace

const IR::Type_Extern *ExternConverter::convertExternType(ProgramStructure *structure,
            const IR::Type_Extern *ext, cstring name) {
    if (!ext->attributes.empty())
        warning(ErrorType::WARN_UNSUPPORTED, "%s: P4_14 extern type not fully supported", ext);
    return ext->apply(FixupExtern(structure, name))->to<IR::Type_Extern>();
}

const IR::Declaration_Instance *ExternConverter::convertExternInstance(ProgramStructure *structure,
            const IR::Declaration_Instance *ext, cstring name,
            IR::IndexedVector<IR::Declaration> *) {
    auto *rv = ext->clone();
    auto *et = rv->type->to<IR::Type_Extern>();
    BUG_CHECK(et, "Extern %s is not extern type, but %s", ext, ext->type);
    if (!ext->properties.empty())
        warning(ErrorType::WARN_UNSUPPORTED, "%s: P4_14 extern not fully supported", ext);
    if (structure->extern_remap.count(et))
        et = structure->extern_remap.at(et);
    rv->name = name;
    rv->type = new IR::Type_Name(new IR::Path(structure->extern_types.get(et)));
    return rv->apply(TypeConverter(structure))->to<IR::Declaration_Instance>();
}

const IR::Statement *ExternConverter::convertExternCall(ProgramStructure *structure,
            const IR::Declaration_Instance *ext, const IR::Primitive *prim) {
    ExpressionConverter conv(structure);
    auto extref = new IR::PathExpression(structure->externs.get(ext));
    auto method = new IR::Member(prim->srcInfo, extref, prim->name);
    auto args = new IR::Vector<IR::Argument>();
    for (unsigned i = 1; i < prim->operands.size(); ++i)
        args->push_back(new IR::Argument(
            conv.convert(prim->operands.at(i))));
    auto mc = new IR::MethodCallExpression(prim->srcInfo, method, args);
    return new IR::MethodCallStatement(prim->srcInfo, mc);
}

std::map<cstring, ExternConverter *> *ExternConverter::cvtForType = nullptr;

void ExternConverter::addConverter(cstring type, ExternConverter *cvt) {
    static std::map<cstring, ExternConverter *> tbl;
    cvtForType = &tbl;
    tbl[type] = cvt;
}

ExternConverter *ExternConverter::get(cstring type) {
    static ExternConverter default_cvt;
    if (cvtForType && cvtForType->count(type))
        return cvtForType->at(type);
    return &default_cvt;
}

///////////////////////////////////////////////////////////////

std::map<cstring, std::vector<PrimitiveConverter *>> *PrimitiveConverter::all_converters;

PrimitiveConverter::PrimitiveConverter(cstring name, int prio) : prim_name(name), priority(prio) {
    static std::map<cstring, std::vector<PrimitiveConverter *>> converters;
    all_converters = &converters;
    auto &vec = converters[name];
    auto it = vec.begin();
    while (it != vec.end() && (*it)->priority > prio) ++it;
    if (it != vec.end() && (*it)->priority == prio)
        BUG("duplicate primitive converter for %s at priority %d", name, prio);
    vec.insert(it, this);
}

PrimitiveConverter::~PrimitiveConverter() {
    auto &vec = all_converters->at(prim_name);
    vec.erase(std::find(vec.begin(), vec.end(), this));
}

const IR::Statement *PrimitiveConverter::cvtPrimitive(ProgramStructure *structure,
                                                      const IR::Primitive *primitive) {
    if (all_converters->count(primitive->name))
        for (auto cvt : all_converters->at(primitive->name))
            if (auto *rv = cvt->convert(structure, primitive))
                return rv;
    return nullptr;
}

safe_vector<const IR::Expression *>
PrimitiveConverter::convertArgs(ProgramStructure *structure, const IR::Primitive *prim) {
    ExpressionConverter conv(structure);
    safe_vector<const IR::Expression *> rv;
    for (auto arg : prim->operands)
        rv.push_back(conv.convert(arg));
    return rv;
}

///////////////////////////////////////////////////////////////

namespace {
class DiscoverStructure : public Inspector {
    ProgramStructure* structure;

    // These names can only be used for very specific purposes
    std::map<cstring, cstring> reserved_names = {
        { "standard_metadata_t", "type" },
        { "standard_metadata", "metadata" },
        { "egress", "control" }
    };

    void checkReserved(const IR::Node* node, cstring nodeName, cstring kind) const {
        auto it = reserved_names.find(nodeName);
        if (it == reserved_names.end())
            return;
        if (it->second != kind)
            ::error("%1% cannot have this name; it can only be used for %2%", node, it->second);
    }
    void checkReserved(const IR::Node* node, cstring nodeName) const {
        checkReserved(node, nodeName, nullptr);
    }

 public:
    explicit DiscoverStructure(ProgramStructure* structure) : structure(structure)
    { CHECK_NULL(structure); setName("DiscoverStructure"); }

    void postorder(const IR::ParserException* ex) override {
        ::warning(ErrorType::WARN_UNSUPPORTED, "%1%: parser exception is not translated to P4-16",
                  ex); }
    void postorder(const IR::Metadata* md) override
    { structure->metadata.emplace(md); checkReserved(md, md->name, "metadata"); }
    void postorder(const IR::Header* hd) override
    { structure->headers.emplace(hd); checkReserved(hd, hd->name); }
    void postorder(const IR::Type_StructLike *t) override
    { structure->types.emplace(t); checkReserved(t, t->name, "type"); }
    void postorder(const IR::V1Control* control) override
    { structure->controls.emplace(control); checkReserved(control, control->name, "control"); }
    void postorder(const IR::V1Parser* parser) override
    { structure->parserStates.emplace(parser); checkReserved(parser, parser->name); }
    void postorder(const IR::V1Table* table) override
    { structure->tables.emplace(table); checkReserved(table, table->name); }
    void postorder(const IR::ActionFunction* action) override
    { structure->actions.emplace(action); checkReserved(action, action->name); }
    void postorder(const IR::HeaderStack* stack) override
    { structure->stacks.emplace(stack); checkReserved(stack, stack->name); }
    void postorder(const IR::Counter* count) override
    { structure->counters.emplace(count); checkReserved(count, count->name); }
    void postorder(const IR::Register* reg) override
    { structure->registers.emplace(reg); checkReserved(reg, reg->name); }
    void postorder(const IR::ActionProfile* ap) override
    { structure->action_profiles.emplace(ap); checkReserved(ap, ap->name); }
    void postorder(const IR::FieldList* fl) override
    { structure->field_lists.emplace(fl); checkReserved(fl, fl->name); }
    void postorder(const IR::FieldListCalculation* flc) override
    { structure->field_list_calculations.emplace(flc); checkReserved(flc, flc->name); }
    void postorder(const IR::CalculatedField* cf) override
    { structure->calculated_fields.push_back(cf); }
    void postorder(const IR::Meter* m) override
    { structure->meters.emplace(m); checkReserved(m, m->name); }
    void postorder(const IR::ActionSelector* as) override
    { structure->action_selectors.emplace(as); checkReserved(as, as->name); }
    void postorder(const IR::Type_Extern *ext) override
    { structure->extern_types.emplace(ext); checkReserved(ext, ext->name); }
    void postorder(const IR::Declaration_Instance *ext) override
    { structure->externs.emplace(ext); checkReserved(ext, ext->name); }
    void postorder(const IR::ParserValueSet* pvs) override
    { structure->value_sets.emplace(pvs); checkReserved(pvs, pvs->name); }
};

class ComputeCallGraph : public Inspector {
    ProgramStructure* structure;

 public:
    explicit ComputeCallGraph(ProgramStructure* structure) : structure(structure)
    { CHECK_NULL(structure); setName("ComputeCallGraph"); }

    void postorder(const IR::V1Parser* parser) override {
        LOG3("Scanning parser " << parser->name);
        structure->parsers.add(parser->name);
        if (!parser->default_return.name.isNullOrEmpty())
            structure->parsers.calls(parser->name, parser->default_return);
        if (parser->cases != nullptr)
            for (auto ce : *parser->cases)
                structure->parsers.calls(parser->name, ce->action.name);
        for (auto expr : parser->stmts) {
            if (expr->is<IR::Primitive>()) {
                auto primitive = expr->to<IR::Primitive>();
                if (primitive->name == "extract") {
                    BUG_CHECK(primitive->operands.size() == 1,
                              "Expected 1 operand for %1%", primitive);
                    auto dest = primitive->operands.at(0);
                    LOG3("Parser " << parser->name << " extracts into " << dest);
                    structure->extracts[parser->name].push_back(dest);
                }
            }
        }
    }
    void postorder(const IR::Primitive* primitive) override {
        auto name = primitive->name;
        const IR::GlobalRef *glob = nullptr;
        const IR::Declaration_Instance *extrn = nullptr;
        if (primitive->operands.size() >= 1)
            glob = primitive->operands[0]->to<IR::GlobalRef>();
        if (glob) extrn = glob->obj->to<IR::Declaration_Instance>();

        if (extrn) {
            auto parent = findContext<IR::ActionFunction>();
            BUG_CHECK(parent != nullptr, "%1%: Extern call not within action", primitive);
            structure->calledExterns.calls(parent->name, extrn->name.name);
            return;
        } else if (primitive->name == "count") {
            // counter invocation
            auto ctrref = primitive->operands.at(0);
            const IR::Counter *ctr = nullptr;
            if (auto gr = ctrref->to<IR::GlobalRef>())
                ctr = gr->obj->to<IR::Counter>();
            else if (auto nr = ctrref->to<IR::PathExpression>())
                ctr = structure->counters.get(nr->path->name);
            if (ctr == nullptr)
                ::error("Cannot find counter %1%", ctrref);
            auto parent = findContext<IR::ActionFunction>();
            BUG_CHECK(parent != nullptr, "%1%: Counter call not within action", primitive);
            structure->calledCounters.calls(parent->name, ctr->name.name);
            return;
        } else if (primitive->name == "execute_meter") {
            auto mtrref = primitive->operands.at(0);
            const IR::Meter *mtr = nullptr;
            if (auto gr = mtrref->to<IR::GlobalRef>())
                mtr = gr->obj->to<IR::Meter>();
            else if (auto nr = mtrref->to<IR::PathExpression>())
                mtr = structure->meters.get(nr->path->name);
            if (mtr == nullptr)
                ::error("Cannot find meter %1%", mtrref);
            auto parent = findContext<IR::ActionFunction>();
            BUG_CHECK(parent != nullptr,
                      "%1%: not within action", primitive);
            structure->calledMeters.calls(parent->name, mtr->name.name);
            return;
        } else if (primitive->name == "register_read" || primitive->name == "register_write") {
            const IR::Expression* regref;
            if (primitive->name == "register_read")
                regref = primitive->operands.at(1);
            else
                regref = primitive->operands.at(0);
            const IR::Register *reg = nullptr;
            if (auto gr = regref->to<IR::GlobalRef>())
                reg = gr->obj->to<IR::Register>();
            else if (auto nr = regref->to<IR::PathExpression>())
                reg = structure->registers.get(nr->path->name);
            if (reg == nullptr)
                ::error("Cannot find register %1%", regref);
            auto parent = findContext<IR::ActionFunction>();
            BUG_CHECK(parent != nullptr,
                      "%1%: not within action", primitive);
            structure->calledRegisters.calls(parent->name, reg->name.name);
            return;
        } else if (structure->actions.contains(name)) {
            auto parent = findContext<IR::ActionFunction>();
            BUG_CHECK(parent != nullptr, "%1%: Action call not within action", primitive);
            structure->calledActions.calls(parent->name, name);
        } else if (structure->controls.contains(name)) {
            auto parent = findContext<IR::V1Control>();
            BUG_CHECK(parent != nullptr, "%1%: Control call not within control", primitive);
            structure->calledControls.calls(parent->name, name);
        }
    }
    void postorder(const IR::GlobalRef *gref) override {
        cstring caller;
        if (auto af = findContext<IR::ActionFunction>()) {
            caller = af->name;
        } else if (auto di = findContext<IR::Declaration_Instance>()) {
            caller = di->name;
        } else {
            BUG("%1%: GlobalRef not within action or extern", gref); }
        if (auto ctr = gref->obj->to<IR::Counter>())
            structure->calledCounters.calls(caller, ctr->name.name);
        else if (auto mtr = gref->obj->to<IR::Meter>())
            structure->calledMeters.calls(caller, mtr->name.name);
        else if (auto reg = gref->obj->to<IR::Register>())
            structure->calledRegisters.calls(caller, reg->name.name);
        else if (auto ext = gref->obj->to<IR::Declaration_Instance>())
            structure->calledExterns.calls(caller, ext->name.name);
    }
};

/// Table call graph should be built after the control call graph is built.
/// In the case that the program contains an unused control block, the
/// table invocation in the unused control block should not be considered.
class ComputeTableCallGraph : public Inspector {
    ProgramStructure *structure;

 public:
    explicit ComputeTableCallGraph(ProgramStructure *structure) : structure(structure) {
        CHECK_NULL(structure);
        setName("ComputeTableCallGraph");
    }

    void postorder(const IR::Apply *apply) override {
        LOG3("Scanning " << apply->name);
        auto tbl = structure->tables.get(apply->name.name);
        if (tbl == nullptr)
            ::error("Could not find table %1%", apply->name);
        auto parent = findContext<IR::V1Control>();
        ERROR_CHECK(parent != nullptr, "%1%: Apply not within a control block?", apply);

        auto ctrl = get(structure->tableMapping, tbl);

        // skip control block that is unused.
        if (!structure->calledControls.isCallee(parent->name) &&
            parent->name != P4V1::V1Model::instance.ingress.name &&
            parent->name != P4V1::V1Model::instance.egress.name )
            return;

        if (ctrl != nullptr && ctrl != parent) {
            auto previous = get(structure->tableInvocation, tbl);
            ::error("Table %1% invoked from two different controls: %2% and %3%",
                    tbl, apply, previous);
        }
        LOG3("Invoking " << tbl << " in " << parent->name);
        structure->tableMapping.emplace(tbl, parent);
        structure->tableInvocation.emplace(tbl, apply);
    }
};

class Rewriter : public Transform {
    ProgramStructure* structure;
 public:
    explicit Rewriter(ProgramStructure* structure) : structure(structure)
    { CHECK_NULL(structure); setName("Rewriter"); }

    const IR::Node* preorder(IR::V1Program* global) override {
        if (LOGGING(4)) {
            LOG4("#### Initial P4_14 program");
            dump(global); }
        prune();
        auto *rv = structure->create(global->srcInfo);
        if (LOGGING(4)) {
            LOG4("#### Generated P4_16 program");
            dump(rv); }
        return rv;
    }
};

/**
This pass uses the @length annotation set by the v1 front-end on
varbit fields and converts extracts for headers with varbit fields.
This only supports headers with a single varbit field.
(The @length annotation is inserted as a conversion from the length
header property.)  For example:

header H {
   bit<8> len;
   @length(len)
   varbit<64> data;
}
...
H h;
pkt.extract(h);

is converted to:

header H {
   bit<8> len;
   varbit<64> data;  // annotation removed
}
...
H h;

// Fixed-length size of H
header H_0 {
   bit<8> len;
}

H_0 h_0;
h_0 = pkt.lookahead<H_0>();
pkt.extract(h, h_0.len);

*/
class FixExtracts final : public Transform {
    ProgramStructure* structure;

    struct HeaderSplit {
        /// Fixed-size part of a header type (computed for each extract).
        const IR::Type_Header* fixedHeaderType;
        /// Expression computing the length of the variable-size header.
        const IR::Expression* headerLength;
    };

    /// All newly-introduced types.
    // The following vector contains only IR::Type_Header, but it is easier
    // to append if the elements are Node.
    IR::Vector<IR::Node>               allTypeDecls;
    /// All newly-introduced variables, for each parser.
    IR::IndexedVector<IR::Declaration> varDecls;
    /// Maps each original header type name to the fixed part of it.
    std::map<cstring, HeaderSplit*> fixedPart;

    /// If a header type contains a varbit field then create a new
    /// header type containing only the fields prior to the varbit.
    /// Returns nullptr otherwise.
    HeaderSplit* splitHeaderType(const IR::Type_Header* type) {
        // Maybe we have seen this type already
        auto fixed = ::get(fixedPart, type->name.name);
        if (fixed != nullptr)
            return fixed;

        const IR::Expression* headerLength = nullptr;
        // We allocate the following when we find the first varbit field.
        const IR::Type_Header* fixedHeaderType = nullptr;
        IR::IndexedVector<IR::StructField> fields;

        for (auto f : type->fields) {
            if (f->type->is<IR::Type_Varbits>()) {
                cstring hname = structure->makeUniqueName(type->name);
                if (fixedHeaderType != nullptr) {
                    ::error("%1%: header types with multiple varbit fields are not supported",
                            type);
                    return nullptr;
                }
                fixedHeaderType = new IR::Type_Header(IR::ID(hname), fields);
                // extract length from annotation
                auto anno = f->getAnnotation(IR::Annotation::lengthAnnotation);
                BUG_CHECK(anno != nullptr, "No length annotation on varbit field", f);
                BUG_CHECK(anno->expr.size() == 1, "Expected exactly 1 argument", anno->expr);
                headerLength = anno->expr.at(0);
                // We keep going through the loop just to check whether there is another
                // varbit field in the header.
            } else if (fixedHeaderType == nullptr) {
                // We only keep the fields prior to the varbit field
                fields.push_back(f);
            }
        }
        if (fixedHeaderType != nullptr) {
            LOG3("Extracted fixed-size header type from " << type << " into " << fixedHeaderType);
            fixed = new HeaderSplit;
            fixed->fixedHeaderType = fixedHeaderType;
            fixed->headerLength = headerLength;
            fixedPart.emplace(type->name.name, fixed);
            allTypeDecls.push_back(fixedHeaderType);
            return fixed;
        }
        return nullptr;
    }

    /**
       This pass rewrites expressions that appear in a @length
       annotation: PathExpressions that refer to enclosing struct
       fields are rewritten to refer to the proper fields of a
       variable `var`.  In the example above, the variable is `h_0`
       and `len` is translated to `h_0.len`.
    */
    class RewriteLength final : public Transform {
        const IR::Type_Header* header;
        const IR::Declaration* var;
     public:
        explicit RewriteLength(const IR::Type_Header* header,
                               const IR::Declaration* var) :
                header(header), var(var) { setName("RewriteLength"); }

        const IR::Node* postorder(IR::PathExpression* expression) override {
            if (expression->path->absolute)
                return expression;
            for (auto f : header->fields) {
                if (f->name == expression->path->name)
                    return new IR::Member(
                        expression->srcInfo,
                        new IR::PathExpression(var->name), f->name);
            }
            return expression;
        }
    };

 public:
    explicit FixExtracts(ProgramStructure* structure) : structure(structure)
    { CHECK_NULL(structure); setName("FixExtracts"); }

    const IR::Node* postorder(IR::P4Program* program) override {
        // P4-14 headers cannot refer to other types, so it is safe
        // to prepend them to the list of declarations.
        allTypeDecls.append(program->objects);
        program->objects = allTypeDecls;
        return program;
    }

    const IR::Node* postorder(IR::P4Parser* parser) override {
        if (!varDecls.empty()) {
            parser->parserLocals.append(varDecls);
            varDecls.clear();
        }
        return parser;
    }

    const IR::Node* postorder(IR::MethodCallStatement* statement) override {
        auto mce = getOriginal<IR::MethodCallStatement>()->methodCall;
        LOG3("Looking up in extracts " << dbp(mce));
        auto ht = ::get(structure->extractsSynthesized, mce);
        if (ht == nullptr)
            // not an extract
            return statement;

        // This is an extract method invocation
        BUG_CHECK(mce->arguments->size() == 1, "%1%: expected 1 argument", mce);
        auto arg = mce->arguments->at(0);

        auto fixed = splitHeaderType(ht);
        if (fixed == nullptr)
            return statement;
        CHECK_NULL(fixed->headerLength);
        CHECK_NULL(fixed->fixedHeaderType);

        auto result = new IR::IndexedVector<IR::StatOrDecl>();
        cstring varName = structure->makeUniqueName("tmp_hdr");
        auto var = new IR::Declaration_Variable(
            IR::ID(varName), fixed->fixedHeaderType->to<IR::Type>());
        varDecls.push_back(var);

        // Create lookahead
        auto member = mce->method->to<IR::Member>();  // should be packet_in.extract
        CHECK_NULL(member);
        auto typeArgs = new IR::Vector<IR::Type>();
        typeArgs->push_back(fixed->fixedHeaderType->getP4Type());
        auto lookaheadMethod = new IR::Member(member->expr,
                                              P4::P4CoreLibrary::instance.packetIn.lookahead.name);
        auto lookahead = new IR::MethodCallExpression(
            mce->srcInfo, lookaheadMethod, typeArgs, new IR::Vector<IR::Argument>());
        auto assign = new IR::AssignmentStatement(
            mce->srcInfo, new IR::PathExpression(varName), lookahead);
        result->push_back(assign);
        LOG3("Created lookahead " << assign);

        // Create actual extract
        RewriteLength rewrite(fixed->fixedHeaderType, var);
        auto length = fixed->headerLength->apply(rewrite);
        auto args = new IR::Vector<IR::Argument>();
        args->push_back(arg->clone());
        auto type = IR::Type_Bits::get(
            P4::P4CoreLibrary::instance.packetIn.extractSecondArgSize);
        auto cast = new IR::Cast(Util::SourceInfo(), type, length);
        args->push_back(new IR::Argument(cast));
        auto expression = new IR::MethodCallExpression(
            mce->srcInfo, mce->method->clone(), args);
        result->push_back(new IR::MethodCallStatement(expression));
        return result;
    }
};

/*
  This class is used to adjust the expressions in a @length
  annotation on a varbit field.  The P4-14 to P4-16 converter inserts
  these annotations on the unique varbit field in a header; the
  annotations are created from the header max_length and length
  fields.  The length annotation contains an expression which is used
  to compute the length of the varbit field.  The problem that we are
  solving here is that expression semantics is different in P4-14 and
  P4-16.  Consider the canonical case of an IPv4 header:

  header_type ipv4_t {
     fields {
        version : 4;
        ihl : 4;
        // lots of other fields...
        options: *;
    }
    length     : ihl*4;
    max_length : 64;
  }

  This generates the following P4-16 structure:
  struct ipv4_t {
      bit<4> version;
      bit<4> ihl;
      @length((ihl*4) * 8 - 20)  // 20 is the size of the fixed part of the header
      varbit<(64 - 20) * 8> options;
  }

  When such a header is used in an extract statement, the @length
  annotation is used to compute the second argument of the extract
  method.  The problem we are solving here is the fact that ihl is
  only represented on 4 bits, so the evaluation ihl*4 will actually
  overflow.  This is not a problem in P4-14, but it is a problem in
  P4-16.  Unfortunately there is no easy way to guess how many bits
  are required to evaluate this computation.  So what we do is to cast
  all PathExpressions to 32-bits.  This is really just a heuristic,
  but since the semantics of P4-14 expressions is unclear, we cannot
  do much better than this.
*/
class AdjustLengths : public Transform {
 public:
    AdjustLengths() { setName("AdjustLengths"); }
    const IR::Node* postorder(IR::PathExpression* expression) override {
        auto anno = findContext<IR::Annotation>();
        if (anno == nullptr)
            return expression;
        if (anno->name != "length")
            return expression;

        LOG3("Inserting cast in length annotation");
        auto type = IR::Type_Bits::get(32);
        auto cast = new IR::Cast(expression->srcInfo, type, expression);
        return cast;
    }
};

/// Detects whether there are two declarations in the P4-14 program
/// with the same name and for the same kind of object.
class DetectDuplicates: public Inspector {
 public:
    DetectDuplicates() { setName("DetectDuplicates"); }

    bool preorder(const IR::V1Program* program) override {
        auto &map = program->scope;
        auto firstWithKey = map.begin();
        while (firstWithKey != map.end()) {
            auto key = firstWithKey->first;
            auto range = map.equal_range(key);
            for (auto s = range.first; s != range.second; s++) {
                auto n = s;
                for (n++; n != range.second; n++) {
                    auto e1 = s->second;
                    auto e2 = n->second;
                    if (e1->node_type_name() == e2->node_type_name()) {
                        if (e1->srcInfo.getStart().isValid())
                            ::error("%1%: same name as %2%", e1, e2);
                        else
                            // This name is probably standard_metadata_t, a built-in declaration
                            ::error("%1%: name %2% is reserved", e2, key);
                    }
                }
            }
            firstWithKey = range.second;
        }
        // prune; we're done; everything is top-level
        return false;
    }
};

// The fields in standard_metadata in v1model.p4 should only be used if
// the source program is written in P4-16. Therefore we remove those
// fields from the translated P4-14 program.
class RemoveAnnotatedFields : public Transform {
 public:
    RemoveAnnotatedFields() { setName("RemoveAnnotatedFields"); }
    const IR::Node* postorder(IR::Type_Struct* node) override {
        if (node->name == "standard_metadata_t") {
            auto fields = new IR::IndexedVector<IR::StructField>();
            for (auto f : node->fields) {
                if (!f->getAnnotation("alias")) {
                    fields->push_back(f);
                }
            }
            return new IR::Type_Struct(node->srcInfo, node->name, node->annotations, *fields);
        }
        return node;
    }
};

// If a parser state has a pragma @packet_entry, it is treated as a new entry
// point to the parser.
class CheckIfMultiEntryPoint: public Inspector {
    ProgramStructure* structure;

 public:
    explicit CheckIfMultiEntryPoint(ProgramStructure* structure) : structure(structure) {
        setName("CheckIfMultiEntryPoint");
    }
    bool preorder(const IR::ParserState* state) {
        for (const auto* anno : state->getAnnotations()->annotations) {
            if (anno->name.name == "packet_entry") {
                structure->parserEntryPoints.emplace(state->name, state);
            }
        }
        return false;
    }
};

// Generate a new start state that selects on the meta variable,
// standard_metadata.instance_type and branches into one of the entry points.
// The backend is responsible for removing the use of the meta variable and
// eliminate the new start state.  The new start state is not added if the user
// does not use the @packet_entry pragma.
class InsertCompilerGeneratedStartState: public Transform {
    ProgramStructure* structure;
    IR::IndexedVector<IR::Node>        allTypeDecls;
    IR::IndexedVector<IR::Declaration> varDecls;
    IR::Vector<IR::SelectCase>         selCases;
    cstring newStartState;
    cstring newInstanceType;

 public:
    explicit InsertCompilerGeneratedStartState(ProgramStructure* structure) : structure(structure) {
        setName("InsertCompilerGeneratedStartState");
        structure->allNames.emplace("start");
        structure->allNames.emplace("InstanceType");
        newStartState = structure->makeUniqueName("start");
        newInstanceType = structure->makeUniqueName("InstanceType");
    }

    const IR::Node* postorder(IR::P4Program* program) override {
        allTypeDecls.append(program->objects);
        program->objects = allTypeDecls;
        return program;
    }

    // rename original start state
    const IR::Node* postorder(IR::ParserState* state) override {
        if (!structure->parserEntryPoints.size())
            return state;
        if (state->name == IR::ParserState::start) {
            state->name = newStartState;
        }
        return state;
    }

    const IR::Node* postorder(IR::P4Parser* parser) override {
        if (!structure->parserEntryPoints.size())
            return parser;
        IR::IndexedVector<IR::SerEnumMember> members;
        // transition to original start state
        members.push_back(new IR::SerEnumMember("START", new IR::Constant(0)));
        selCases.push_back(new IR::SelectCase(
            new IR::Member(new IR::TypeNameExpression(new IR::Type_Name(newInstanceType)), "START"),
            new IR::PathExpression(new IR::Path(newStartState))));

        // transition to addtional entry points
        unsigned idx = 1;
        for (auto p : structure->parserEntryPoints) {
            members.push_back(new IR::SerEnumMember(p.first, new IR::Constant(idx++)));
            selCases.push_back(new IR::SelectCase(
                new IR::Member(new IR::TypeNameExpression(new IR::Type_Name(newInstanceType)),
                               p.first),
                new IR::PathExpression(new IR::Path(p.second->name))));
        }
        auto instAnnos = new IR::Annotations();
        instAnnos->add(new IR::Annotation(IR::Annotation::nameAnnotation,
                                          {new IR::StringLiteral(IR::ID(".$InstanceType"))}));
        auto instEnum =
            new IR::Type_SerEnum(newInstanceType, instAnnos, IR::Type_Bits::get(32), members);
        allTypeDecls.push_back(instEnum);

        IR::Vector<IR::Expression> selExpr;
        selExpr.push_back(
            new IR::Cast(new IR::Type_Name(newInstanceType),
                         new IR::Member(new IR::PathExpression(new IR::Path("standard_metadata")),
                                        "instance_type")));
        auto selects = new IR::SelectExpression(new IR::ListExpression(selExpr), selCases);
        auto annos = new IR::Annotations();
        annos->add(new IR::Annotation(IR::Annotation::nameAnnotation,
                                      {new IR::StringLiteral(IR::ID(".$start"))}));
        auto startState = new IR::ParserState(IR::ParserState::start, annos, selects);
        varDecls.push_back(startState);

        if (!varDecls.empty()) {
            parser->parserLocals.append(varDecls);
            varDecls.clear();
        }
        return parser;
    }
};

// Handle @packet_entry pragma in P4-14. A P4-14 program may be extended to
// support multiple entry points to the parser. This feature does not comply
// with P4-14 specification, but it is useful in certain use cases.
class FixMultiEntryPoint : public PassManager {
 public:
    explicit FixMultiEntryPoint(ProgramStructure* structure) {
        setName("FixMultiEntryPoint");
        passes.emplace_back(new CheckIfMultiEntryPoint(structure));
        passes.emplace_back(new InsertCompilerGeneratedStartState(structure));
    }
};

}  // namespace



///////////////////////////////////////////////////////////////

static ProgramStructure *defaultCreateProgramStructure() {
    return new ProgramStructure();
}

ProgramStructure *(*Converter::createProgramStructure)() = defaultCreateProgramStructure;

Converter::Converter() {
    setStopOnError(true); setName("Converter");
    structure = createProgramStructure();
    structure->populateOutputNames();

    // Discover types using P4-14 type-checker
    passes.emplace_back(new DetectDuplicates());
    passes.emplace_back(new P4::DoConstantFolding(nullptr, nullptr));
    passes.emplace_back(new CheckHeaderTypes());
    passes.emplace_back(new AdjustLengths());
    passes.emplace_back(new TypeCheck());
    // Convert
    passes.emplace_back(new DiscoverStructure(structure));
    passes.emplace_back(new ComputeCallGraph(structure));
    passes.emplace_back(new ComputeTableCallGraph(structure));
    passes.emplace_back(new Rewriter(structure));
    passes.emplace_back(new FixExtracts(structure));
    passes.emplace_back(new RemoveAnnotatedFields);
    passes.emplace_back(new FixMultiEntryPoint(structure));
}

Visitor::profile_t Converter::init_apply(const IR::Node* node) {
    if (!node->is<IR::V1Program>())
        BUG("Converter only accepts IR::Globals, not %1%", node);
    return PassManager::init_apply(node);
}

}  // namespace P4V1
