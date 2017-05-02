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
    auto range = Util::findOnes(value);
    if (range.lowIndex == 0 && range.highIndex >= exp->type->width_bits() - 1U)
        return exp;
    if (value != range.value)
        return new IR::BAnd(expression->srcInfo, exp, cst);
    return new IR::Slice(exp, new IR::Constant(range.highIndex), new IR::Constant(range.lowIndex));
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
    BUG("Unexpected index %1%", ref->index_);
    return ref;
}

const IR::Node* ExpressionConverter::postorder(IR::GlobalRef *ref) {
    return new IR::PathExpression(new IR::Path(ref->srcInfo, ref->toString()));
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
        auto instanceName = get(renameMap, control->name);
        auto ctrl = new IR::PathExpression(IR::ID(instanceName));
        auto method = new IR::Member(ctrl, IR::ID(IR::IApply::applyMethodName));
        auto args = new IR::Vector<IR::Expression>();
        args->push_back(structure->conversionContext.header->clone());
        args->push_back(structure->conversionContext.userMetadata->clone());
        args->push_back(structure->conversionContext.standardMetadata->clone());
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
    if (auto type = findContext<IR::Type_StructLike>())
        if (auto len = type->getAnnotation("length"))
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

    void postorder(const IR::Metadata* md) override
    { structure->metadata.emplace(md); }
    void postorder(const IR::Header* hd) override
    { structure->headers.emplace(hd); }
    void postorder(const IR::Type_StructLike *t) override
    { structure->types.emplace(t); }
    void postorder(const IR::V1Control* control) override
    { structure->controls.emplace(control); }
    void postorder(const IR::V1Parser* parser) override
    { structure->parserStates.emplace(parser); }
    void postorder(const IR::V1Table* table) override
    { structure->tables.emplace(table); }
    void postorder(const IR::ActionFunction* action) override
    { structure->actions.emplace(action); }
    void postorder(const IR::HeaderStack* stack) override
    { structure->stacks.emplace(stack); }
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
    void postorder(const IR::ActionSelector* as) override
    { structure->action_selectors.emplace(as); }
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
        LOG3("Scanning " << apply->name);
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
        LOG3("Invoking " << tbl << " in " << parent->name);
        structure->tableMapping.emplace(tbl, parent);
        structure->tableInvocation.emplace(tbl, apply);
    }
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
header H_0 {
   bit<8> len;
}
header H_1 {
   varbit<64> data;
}

H_0 h_0;
H_1 h_1;
pkt.extract(h_0);
pkt.extract(h_1, h_0.len);
h.setValid();
h.len = h_0.len;
h.data = h_1.data;

*/
class FixExtracts final : public Transform {
    ProgramStructure* structure;

    /// Newly-introduced types for each extract.
    std::vector<const IR::Type_Header*> typeDecls;
    /// All newly-introduced types.
    // The following contains IR::Type_Header, but it is easier
    // to append if the elements are Node.
    IR::IndexedVector<IR::Node>        allTypeDecls;
    /// All newly-introduced variables, for each parser.
    IR::IndexedVector<IR::Declaration> varDecls;
    /// Map each newly created header with a varbit field
    /// to an expression that denotes its length.
    std::map<const IR::Type_Header*, const IR::Expression*> lengths;

    /// If a header type contains varbit fields split it into several
    /// header types, each of which starts with a varbit field.  The
    /// types are inserted in the typeDecls list.
    void splitHeaderType(const IR::Type_Header* type) {
        IR::IndexedVector<IR::StructField> fields;
        const IR::Expression* length = nullptr;
        for (auto f : type->fields) {
            if (f->type->is<IR::Type_Varbits>()) {
                cstring hname = structure->makeUniqueName(type->name);
                auto htype = new IR::Type_Header(IR::ID(hname), fields);
                if (length != nullptr)
                    lengths.emplace(htype, length);
                typeDecls.push_back(htype);
                fields.clear();
                auto anno = f->getAnnotation(IR::Annotation::lengthAnnotation);
                BUG_CHECK(anno != nullptr, "No length annotation on varbit field", f);
                BUG_CHECK(anno->expr.size() == 1, "Expected exactly 1 argument", anno->expr);
                length = anno->expr.at(0);
                f = new IR::StructField(f->srcInfo, f->name, f->type);  // lose the annotation
            }
            fields.push_back(f);
        }
        if (!typeDecls.empty() && !fields.empty()) {
            cstring hname = structure->makeUniqueName(type->name);
            auto htype = new IR::Type_Header(IR::ID(hname), fields);
            typeDecls.push_back(htype);
            lengths.emplace(htype, length);
            LOG3("Split header type " << type << " into " << typeDecls.size() << " parts");
        }
    }

    /**
       This pass rewrites expressions from a @length annotation expression:
       PathExpressions that refer to enclosing fields are rewritten to
       refer to the proper fields in a different structure.  In the example above, `len`
       is translated to `h_0.len`.
     */
    class RewriteLength final : public Transform {
        const std::vector<const IR::Type_Header*> &typeDecls;
        const IR::IndexedVector<IR::Declaration>  &vars;
     public:
        explicit RewriteLength(const std::vector<const IR::Type_Header*> &typeDecls,
                               const IR::IndexedVector<IR::Declaration>  &vars) :
                typeDecls(typeDecls), vars(vars) { setName("RewriteLength"); }
        const IR::Node* postorder(IR::PathExpression* expression) override {
            if (expression->path->absolute)
                return expression;
            unsigned index = 0;
            for (auto t : typeDecls) {
                for (auto f : t->fields) {
                    if (f->name == expression->path->name)
                        return new IR::Member(
                            expression->srcInfo,
                            new IR::PathExpression(vars.at(index)->name), f->name);
                }
                index++;
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
        allTypeDecls.append(program->declarations);
        program->declarations = allTypeDecls;
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
        typeDecls.clear();
        auto mce = getOriginal<IR::MethodCallStatement>()->methodCall;
        LOG3("Looking up in extracts " << dbp(mce));
        auto ht = ::get(structure->extractsSynthesized, mce);
        if (ht == nullptr)
            // not an extract
            return statement;

        // This is an extract method invocation
        BUG_CHECK(mce->arguments->size() == 1, "%1%: expected 1 argument", mce);
        auto arg = mce->arguments->at(0);

        splitHeaderType(ht);
        if (typeDecls.empty())
            return statement;

        auto result = new IR::IndexedVector<IR::StatOrDecl>();
        RewriteLength rewrite(typeDecls, varDecls);
        for (auto t : typeDecls) {
            allTypeDecls.push_back(t);
            cstring varName = structure->makeUniqueName("tmp_hdr");
            auto var = new IR::Declaration_Variable(IR::ID(varName), t->to<IR::Type>());
            auto length = ::get(lengths, t);
            varDecls.push_back(var);
            auto args = new IR::Vector<IR::Expression>();
            args->push_back(new IR::PathExpression(IR::ID(varName)));
            if (length != nullptr) {
                length = length->apply(rewrite);
                auto type = IR::Type_Bits::get(
                    P4::P4CoreLibrary::instance.packetIn.extractSecondArgSize);
                auto cast = new IR::Cast(Util::SourceInfo(), type, length);
                args->push_back(cast);
            }
            auto expression = new IR::MethodCallExpression(
                mce->srcInfo, mce->method->clone(), args);
            result->push_back(new IR::MethodCallStatement(expression));
        }

        auto setValid = new IR::Member(
            mce->srcInfo, arg, IR::Type_Header::setValid);
        result->push_back(new IR::MethodCallStatement(
            new IR::MethodCallExpression(
                mce->srcInfo, setValid, new IR::Vector<IR::Expression>())));
        unsigned index = 0;
        for (auto t : typeDecls) {
            auto var = varDecls.at(index);
            for (auto f : t->fields) {
                auto left = new IR::Member(mce->srcInfo, arg, f->name);
                auto right = new IR::Member(mce->srcInfo,
                                            new IR::PathExpression(var->name), f->name);
                auto assign = new IR::AssignmentStatement(mce->srcInfo, left, right);
                result->push_back(assign);
            }
            index++;
        }
        return result;
    }
};

}  // namespace

///////////////////////////////////////////////////////////////

Converter::Converter() {
    setStopOnError(true); setName("Converter");

    // Discover types using P4 v1.1 type-checker
    passes.emplace_back(new P4::DoConstantFolding(nullptr, nullptr));
    passes.emplace_back(new CheckHeaderTypes());
    passes.emplace_back(new TypeCheck());
    // Convert
    passes.emplace_back(new DiscoverStructure(&structure));
    passes.emplace_back(new ComputeCallGraph(&structure));
    passes.emplace_back(new Rewriter(&structure));
    passes.emplace_back(new FixExtracts(&structure));
}

Visitor::profile_t Converter::init_apply(const IR::Node* node) {
    if (!node->is<IR::V1Program>())
        BUG("Converter only accepts IR::Globals, not %1%", node);
    return PassManager::init_apply(node);
}

}  // namespace P4V1
