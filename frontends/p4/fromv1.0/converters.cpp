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
    while (value != 0) {
        auto range = Util::findOnes(value);
        value = value - range.value;
        exp = new IR::Slice(Util::SourceInfo(), exp,
                            new IR::Constant(range.highIndex), new IR::Constant(range.lowIndex));
    }
    return exp;
}

const IR::Node* ExpressionConverter::postorder(IR::Constant* expression) {
    // The P4-14 front-end may have constants that overflow their declared type,
    // since the v1 type inference sets types to constants without any checks.
    // We fix this here.
    return new IR::Constant(expression->srcInfo, expression->type,
                            expression->value, expression->base);
}

const IR::Node* ExpressionConverter::postorder(IR::FieldList* fl) {
    // Field lists may contain other field lists
    return new IR::ListExpression(fl->srcInfo, &fl->fields);
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
        // current(a, b) => packet.lookahead<bit<a+b>>()[a+b-1,a]
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
            Util::SourceInfo(),
            structure->paramReference(structure->parserPacketIn),
            P4::P4CoreLibrary::instance.packetIn.lookahead.Id());
        auto typeargs = new IR::Vector<IR::Type>();
        typeargs->push_back(IR::Type_Bits::get(aval + bval));
        auto lookahead = new IR::MethodCallExpression(
            Util::SourceInfo(), method, typeargs, new IR::Vector<IR::Expression>());
        auto result = new IR::Slice(primitive->srcInfo, lookahead,
                                    new IR::Constant(aval + bval - 1),
                                    new IR::Constant(aval));
        result->type = IR::Type_Bits::get(bval);
        return result;
    } else if (primitive->name == "valid") {
        BUG_CHECK(primitive->operands.size() == 1, "Expected 1 operand for %1%", primitive);
        auto base = primitive->operands.at(0);
        auto method = new IR::Member(primitive->srcInfo, base, IR::ID(IR::Type_Header::isValid));
        auto result = new IR::MethodCallExpression(
            primitive->srcInfo, method, new IR::Vector<IR::Type>(),
            new IR::Vector<IR::Expression>());
        return result;
    }
    BUG("Unexpected primitive %1%", primitive);
}

const IR::Node* ExpressionConverter::postorder(IR::NamedRef* ref) {
    if (ref->name.name == "latest") {
        return structure->latest;
    }
    if (ref->name.name == "next") {
        return ref;
    }
    auto fl = structure->field_lists.get(ref->name);
    if (fl != nullptr) {
        ExpressionConverter conv(structure);
        return conv.convert(fl);
    }
    return new IR::PathExpression(ref->name);
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
    auto result = new IR::Member(Util::SourceInfo(), ref, nhr->ref->name);
    result->type = nhr->type;
    return result;
}

const IR::Node* ExpressionConverter::postorder(IR::HeaderStackItemRef* ref) {
    if (ref->index_->is<IR::NamedRef>()) {
        auto nr = ref->index_->to<IR::NamedRef>();
        if (nr->name == "last" || nr->name == "next") {
            cstring name = nr->name == "last" ? IR::Type_Stack::last : IR::Type_Stack::next;
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
    BUG("Unexpected index %1%", ref->index_);
    return ref;
}

const IR::Node* StatementConverter::preorder(IR::Apply* apply) {
    auto table = structure->tables.get(apply->name);
    auto newname = structure->tables.get(table);
    auto tbl = new IR::PathExpression(newname);
    auto method = new IR::Member(Util::SourceInfo(), tbl,
                                 IR::ID(IR::IApply::applyMethodName));
    auto call = new IR::MethodCallExpression(apply->srcInfo, method,
                                             structure->emptyTypeArguments,
                                             new IR::Vector<IR::Expression>());
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
            auto hitcase = hit ? conv.convert(hit) : new IR::EmptyStatement(Util::SourceInfo());
            auto misscase = miss ? conv.convert(miss) : nullptr;
            auto check = new IR::Member(Util::SourceInfo(), call, IR::Type_Table::hit);
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
                if (a.first == "default")
                    destination = new IR::DefaultExpression(Util::SourceInfo());
                else
                    destination = new IR::PathExpression(IR::ID(a.first));
                auto swcase = new IR::SwitchCase(a.second->srcInfo, destination, stat);
                cases.insert(insert_at, swcase); }
            auto check = new IR::Member(Util::SourceInfo(), call, IR::Type_Table::action_run);
            auto sw = new IR::SwitchStatement(apply->srcInfo, check, std::move(cases));
            prune();
            return sw;
        }
    }
}

const IR::Node* StatementConverter::preorder(IR::Primitive* primitive) {
    auto control = structure->controls.get(primitive->name);
    if (control != nullptr) {
        auto instanceName = get(renameMap, control->name);
        auto ctrl = new IR::PathExpression(IR::ID(instanceName));
        auto method = new IR::Member(Util::SourceInfo(), ctrl, IR::ID(IR::IApply::applyMethodName));
        auto args = new IR::Vector<IR::Expression>();
        args->push_back(structure->conversionContext.header->clone());
        args->push_back(structure->conversionContext.userMetadata->clone());
        args->push_back(structure->conversionContext.standardMetadata->clone());
        auto call = new IR::MethodCallExpression(primitive->srcInfo, method,
                                                 structure->emptyTypeArguments, args);
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
        t = new IR::EmptyStatement(Util::SourceInfo());
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
    auto result = new IR::BlockStatement(toConvert->srcInfo, IR::Annotations::empty, stats);
    return result;
}

const IR::Type_Varbits *TypeConverter::postorder(IR::Type_Varbits *vbtype) {
    if (vbtype->size == 0) {
        if (auto type = findContext<IR::Type_StructLike>()) {
            if (auto max = type->annotations->getSingle("max_length")) {
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
    if (auto type = findContext<IR::Type_StructLike>())
        if (auto len = type->annotations->getSingle("length"))
            field->annotations = field->annotations->add(len);
    return field;
}

const IR::Type_StructLike *TypeConverter::postorder(IR::Type_StructLike *str) {
    str->annotations = str->annotations->where([](const IR::Annotation *a) -> bool {
        return a->name != "length" && a->name != "max_length"; });
    return str;
}

///////////////////////////////////////////////////////////////

namespace {
class DiscoverStructure : public Inspector {
    ProgramStructure* structure;

 public:
    explicit DiscoverStructure(ProgramStructure* structure) : structure(structure)
    { CHECK_NULL(structure); setName("DiscoverStructure"); }

    void postorder(const IR::Metadata* md) override {
        structure->metadata.emplace(md);
        structure->types.emplace(md->type); }
    void postorder(const IR::Header* hd) override {
        structure->headers.emplace(hd);
        structure->types.emplace(hd->type); }
    void postorder(const IR::V1Control* control) override
    { structure->controls.emplace(control); }
    void postorder(const IR::V1Parser* parser) override
    { structure->parserStates.emplace(parser); }
    void postorder(const IR::V1Table* table) override
    { structure->tables.emplace(table); }
    void postorder(const IR::ActionFunction* action) override
    { structure->actions.emplace(action); }
    void postorder(const IR::HeaderStack* stack) override {
        structure->stacks.emplace(stack);
        structure->types.emplace(stack->type); }
    void postorder(const IR::Counter* count) override
    { structure->counters.emplace(count); }
    void postorder(const IR::Register* reg) override
    { structure->registers.emplace(reg); }
    void postorder(const IR::ActionProfile* ap) override
    { structure->action_profiles.emplace(ap); }
    void postorder(const IR::FieldList* fl) override
    { structure->field_lists.emplace(fl); }
    void postorder(const IR::FieldListCalculation* flc) override
    { structure->field_list_calculations.emplace(flc); }
    void postorder(const IR::CalculatedField* cf) override
    { structure->calculated_fields.push_back(cf); }
    void postorder(const IR::Meter* m) override
    { structure->meters.emplace(m); }
    void postorder(const IR::ActionSelector* as) override {
        structure->action_selectors.emplace(as);
        if (!as->type.name.isNullOrEmpty())
            ::warning("%1%: Action selector attribute ignored", as->type);
        if (!as->mode.name.isNullOrEmpty())
            ::warning("%1%: Action selector attribute ignored", as->mode);
    }
    void postorder(const IR::Type_Extern *ext) override
    { structure->extern_types.emplace(ext); }
    void postorder(const IR::Declaration_Instance *ext) override
    { structure->externs.emplace(ext); }
};

class ComputeCallGraph : public Inspector {
    ProgramStructure* structure;

 public:
    explicit ComputeCallGraph(ProgramStructure* structure) : structure(structure)
    { CHECK_NULL(structure); setName("ComputeCallGraph"); }
    void postorder(const IR::Apply* apply) override {
        LOG1("Scanning " << apply->name);
        auto tbl = structure->tables.get(apply->name.name);
        if (tbl == nullptr)
            ::error("Could not find table %1%", apply->name);
        auto parent = findContext<IR::V1Control>();
        BUG_CHECK(parent != nullptr, "%1%: Apply not within a control block?", apply);
        auto ctrl = get(structure->tableMapping, tbl);
        if (ctrl != nullptr && ctrl != parent) {
            auto previous = get(structure->tableInvocation, tbl);
            ::error("Table %1% invoked from two different controls: %2% and %3%",
                    tbl, apply, previous);
        }
        LOG1("Invoking " << tbl << " in " << parent->name);
        structure->tableMapping.emplace(tbl, parent);
        structure->tableInvocation.emplace(tbl, apply);
    }
    void postorder(const IR::V1Parser* parser) override {
        LOG1("Scanning parser " << parser->name);
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
                    LOG1("Parser " << parser->name << " extracts into " << dest);
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
            else if (auto nr = ctrref->to<IR::NamedRef>())
                ctr = structure->counters.get(nr->name);
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
            else if (auto nr = mtrref->to<IR::NamedRef>())
                mtr = structure->meters.get(nr->name);
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
            else if (auto nr = regref->to<IR::NamedRef>())
                reg = structure->registers.get(nr->name);
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
};

class Rewriter : public Transform {
    ProgramStructure* structure;
 public:
    explicit Rewriter(ProgramStructure* structure) : structure(structure)
    { CHECK_NULL(structure); setName("Rewriter"); }

    const IR::Node* preorder(IR::V1Program* global) override {
        if (Log::verbose())
            dump(global);
        prune();
        return structure->create(global->srcInfo);
    }
};
}  // namespace

///////////////////////////////////////////////////////////////

Converter::Converter() {
    setStopOnError(true);

    // Discover types using P4 v1.1 type-checker
    passes.emplace_back(new P4::DoConstantFolding(nullptr, nullptr));
    passes.emplace_back(new CheckHeaderTypes);
    passes.emplace_back(new TypeCheck);
    // Convert
    passes.emplace_back(new DiscoverStructure(&structure));
    passes.emplace_back(new ComputeCallGraph(&structure));
    passes.emplace_back(new Rewriter(&structure));
}

Visitor::profile_t Converter::init_apply(const IR::Node* node) {
    if (!node->is<IR::V1Program>())
        BUG("Converter only accepts IR::Globals, not %1%", node);
    return PassManager::init_apply(node);
}

}  // namespace P4V1
