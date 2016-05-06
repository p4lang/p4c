#include "converters.h"
#include "programStructure.h"
#include "v1model.h"
#include "lib/gmputil.h"
#include "frontends/p4/coreLibrary.h"
#include "frontends/common/constantFolding.h"
#include "frontends/p4v1/header_type.h"
#include "frontends/p4v1/typecheck.h"

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
    // The P4v1 front-end may have constants that overflow their declared type,
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
        // current(a, b) => packet.lookahead<bit<a+b>>()[b-1::0]
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
                                    new IR::Constant(bval - 1),
                                    new IR::Constant(0));
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
    BUG("Unexpected expression %1%", ref);
}

const IR::Node* ExpressionConverter::postorder(IR::ConcreteHeaderRef* nhr) {
    const IR::Expression* ref;
    if (structure->isHeader(nhr)) {
        ref = structure->conversionContext.header;
    } else {
        if (nhr->ref->name == P4V1::V1Model::instance.standardMetadata.name)
            return structure->conversionContext.standardMetadata;
        else
            ref = structure->conversionContext.userMetadata;
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
            auto cases = new IR::Vector<IR::SwitchCase>();
            for (auto a : apply->actions) {
                StatementConverter conv(structure, renameMap);
                auto stat = conv.convert(a.second);
                const IR::Expression* destination;
                if (a.first == "default")
                    destination = new IR::DefaultExpression(Util::SourceInfo());
                else
                    destination = new IR::PathExpression(IR::ID(a.first));
                auto swcase = new IR::SwitchCase(a.second->srcInfo, destination, stat);
                cases->push_back(swcase);
            }
            auto check = new IR::Member(Util::SourceInfo(), call, IR::Type_Table::action_run);
            auto sw = new IR::SwitchStatement(apply->srcInfo, check, cases);
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
        args->push_back(structure->conversionContext.header);
        args->push_back(structure->conversionContext.userMetadata);
        args->push_back(structure->conversionContext.standardMetadata);
        auto call = new IR::MethodCallExpression(primitive->srcInfo, method,
                                                 structure->emptyTypeArguments, args);
        auto stat = new IR::MethodCallStatement(primitive->srcInfo, call);
        return stat;
    }
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
    auto result = new IR::BlockStatement(toConvert->srcInfo, stats);
    return result;
}

///////////////////////////////////////////////////////////////

namespace {
class DiscoverStructure : public Inspector {
    ProgramStructure* structure;

 public:
    explicit DiscoverStructure(ProgramStructure* structure) : structure(structure)
    { CHECK_NULL(structure); }

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
};

class ComputeCallGraph : public Inspector {
    ProgramStructure* structure;

 public:
    explicit ComputeCallGraph(ProgramStructure* structure) : structure(structure)
    { CHECK_NULL(structure); }
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
            structure->parsers.add(parser->name, parser->default_return);
        if (parser->cases != nullptr)
            for (auto ce : *parser->cases)
                structure->parsers.add(parser->name, ce->action.name);
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
        if (primitive->name == "count") {
            // counter invocation
            auto ctrref = primitive->operands.at(0);
            if (ctrref->is<IR::NamedRef>()) {
                auto nr = ctrref->to<IR::NamedRef>();
                auto ctr = structure->counters.get(nr->name);
                if (ctr == nullptr)
                    ::error("Cannot find counter %1%", ctrref);
                auto parent = findContext<IR::ActionFunction>();
                BUG_CHECK(parent != nullptr, "%1%: Counter call not within action", primitive);
                structure->calledCounters.add(parent->name, ctr->name.name);
                return;
            }
            BUG("%1%: Unexpected counter reference", ctrref);
        } else if (primitive->name == "execute_meter") {
            auto mtrref = primitive->operands.at(0);
            if (mtrref->is<IR::NamedRef>()) {
                auto nr = mtrref->to<IR::NamedRef>();
                auto mtr = structure->meters.get(nr->name);
                if (mtr == nullptr)
                    ::error("Cannot find meter %1%", mtrref);
                auto parent = findContext<IR::ActionFunction>();
                BUG_CHECK(parent != nullptr,
                          "%1%: not within action", primitive);
                structure->calledMeters.add(parent->name, mtr->name.name);
                return;
            }
            BUG("%1%: Unexpected meter reference", mtrref);
        } else if (primitive->name == "register_read" || primitive->name == "register_write") {
            const IR::Expression* regref;
            if (primitive->name == "register_read")
                regref = primitive->operands.at(1);
            else
                regref = primitive->operands.at(0);
            if (regref->is<IR::NamedRef>()) {
                auto nr = regref->to<IR::NamedRef>();
                auto reg = structure->registers.get(nr->name);
                if (reg == nullptr)
                    ::error("Cannot find register %1%", regref);
                auto parent = findContext<IR::ActionFunction>();
                BUG_CHECK(parent != nullptr,
                          "%1%: not within action", primitive);
                structure->calledRegisters.add(parent->name, reg->name.name);
                return;
            }
            BUG("%1%: Unexpected register reference", regref);
        } else if (structure->actions.contains(name)) {
            auto parent = findContext<IR::ActionFunction>();
            BUG_CHECK(parent != nullptr, "%1%: Action call not within action", primitive);
            structure->calledActions.add(parent->name, name);
        } else if (structure->controls.contains(name)) {
            auto parent = findContext<IR::V1Control>();
            BUG_CHECK(parent != nullptr, "%1%: Control call not within control", primitive);
            structure->calledControls.add(parent->name, name);
        }
    }
};

class Rewriter : public Transform {
    ProgramStructure* structure;
 public:
    explicit Rewriter(ProgramStructure* structure) : structure(structure)
    { CHECK_NULL(structure); }

    const IR::Node* preorder(IR::V1Program* global) override {
        if (verbose > 1)
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
    passes.emplace_back(new P4::ConstantFolding(nullptr, nullptr));
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
