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

#include "frontends/common/constantFolding.h"
#include "frontends/common/options.h"
#include "frontends/p4-14/header_type.h"
#include "frontends/p4-14/typecheck.h"
#include "frontends/p4/coreLibrary.h"
#include "lib/big_int_util.h"
#include "programStructure.h"
#include "v1model.h"

namespace P4V1 {

const IR::Type *ExpressionConverter::getFieldType(const IR::Type_StructLike *ht,
                                                  cstring fieldName) {
    auto field = ht->getField(fieldName);
    if (field != nullptr) return field->type;
    BUG("Cannot find field %1% in type %2%", fieldName, ht);
}

// These can only come from the key of a table.
const IR::Node *ExpressionConverter::postorder(IR::Mask *expression) {
    if (!expression->right->is<IR::Constant>()) {
        ::error(ErrorType::ERR_INVALID, "%1%: Mask must be a constant", expression->right);
        return expression;
    }

    auto exp = expression->left;
    auto cst = expression->right->to<IR::Constant>();
    big_int value = cst->value;
    if (value == 0) {
        warn(ErrorType::WARN_INVALID, "%1%: zero mask", expression->right);
        return cst;
    }
    auto range = Util::findOnes(value);
    if (range.lowIndex == 0 && range.highIndex >= exp->type->width_bits() - 1U) return exp;
    if (value != range.value) return new IR::BAnd(expression->srcInfo, exp, cst);
    return new IR::Slice(exp, new IR::Constant(expression->srcInfo, range.highIndex),
                         new IR::Constant(expression->srcInfo, range.lowIndex));
}

const IR::Node *ExpressionConverter::postorder(IR::Constant *expression) {
    // The P4-14 front-end may have constants that overflow their declared type,
    // since the v1 type inference sets types to constants without any checks.
    // We fix this here.
    if (expression->type->is<IR::Type::Boolean>())
        return new IR::BoolLiteral(expression->srcInfo, expression->type, expression->value != 0);
    else
        return new IR::Constant(expression->srcInfo, expression->type, expression->value,
                                expression->base);
}

const IR::Node *ExpressionConverter::postorder(IR::FieldList *fl) {
    // Field lists may contain other field lists
    if (auto func = get(std::type_index(typeid(*fl)).name())) {
        return func(fl);
    }
    return new IR::ListExpression(fl->srcInfo, fl->fields);
}

const IR::Node *ExpressionConverter::postorder(IR::Member *field) {
    if (auto ht = field->expr->type->to<IR::Type_StructLike>())
        field->type = getFieldType(ht, field->member.name);
    return field;
}

const IR::Node *ExpressionConverter::postorder(IR::ActionArg *arg) {
    auto result = new IR::PathExpression(arg->name);
    result->type = arg->type;
    return result;
}

const IR::Node *ExpressionConverter::postorder(IR::Primitive *primitive) {
    if (primitive->name == "current") {
        // current(a, b) => packet.lookahead<bit<a+b>>()[b-1,0]
        BUG_CHECK(primitive->operands.size() == 2, "Expected 2 operands for %1%", primitive);
        auto a = primitive->operands.at(0);
        auto b = primitive->operands.at(1);
        if (!a->is<IR::Constant>() || !b->is<IR::Constant>()) {
            ::error(ErrorType::ERR_INVALID, "%1%: must have constant arguments", primitive);
            return primitive;
        }
        auto aval = a->to<IR::Constant>()->asInt();
        auto bval = b->to<IR::Constant>()->asInt();
        if (aval < 0 || bval <= 0) {
            ::error(ErrorType::ERR_INVALID, "%1%: negative offsets?", primitive);
            return primitive;
        }

        const IR::Expression *method =
            new IR::Member(structure->paramReference(structure->parserPacketIn),
                           P4::P4CoreLibrary::instance().packetIn.lookahead.Id());
        auto typeargs = new IR::Vector<IR::Type>();
        typeargs->push_back(IR::Type_Bits::get(aval + bval));
        auto lookahead = new IR::MethodCallExpression(method, typeargs);
        auto result = new IR::Slice(primitive->srcInfo, lookahead, new IR::Constant(bval - 1),
                                    new IR::Constant(0));
        result->type = IR::Type_Bits::get(bval);
        return result;
    } else if (primitive->name == "valid") {
        BUG_CHECK(primitive->operands.size() == 1, "Expected 1 operand for %1%", primitive);
        auto base = primitive->operands.at(0);
        auto method = new IR::Member(primitive->srcInfo, base, IR::ID(IR::Type_Header::isValid));
        auto result =
            new IR::MethodCallExpression(primitive->srcInfo, IR::Type::Boolean::get(), method);
        return result;
    } else {
        auto func = new IR::PathExpression(IR::ID(primitive->srcInfo, primitive->name));
        auto args = new IR::Vector<IR::Argument>;
        for (auto *op : primitive->operands) args->push_back(new IR::Argument(op));
        auto result = new IR::MethodCallExpression(primitive->srcInfo, func, args);
        return result;
    }
}

const IR::Node *ExpressionConverter::postorder(IR::PathExpression *ref) {
    if (ref->path->name.name == "latest") {
        if (structure->latest == nullptr) {
            ::error(ErrorType::ERR_INVALID, "%1%: latest not yet defined", ref);
            return ref;
        }
        return structure->latest;
    }
    if (ref->path->name.name == "next") {
        return ref;
    }
    if (auto fl = structure->field_lists.get(ref->path->name)) {
        ExpressionConverter conv(structure);
        return conv.convert(fl);
    }
    if (auto flc = structure->field_list_calculations.get(ref->path->name)) {
        // FIXME -- what to do with the algorithm and width from flc?
        return ExpressionConverter(structure).convert(flc->input_fields);
    }
    return ref;
}

const IR::Node *ExpressionConverter::postorder(IR::ConcreteHeaderRef *nhr) {
    const IR::Expression *ref;

    // convert types defined by the target system
    if (nhr->type->is<IR::Type_Header>()) {
        auto type = nhr->type->to<IR::Type_Header>();
        if (structure->parameterTypes.count(type->name)) {
            auto path = new IR::Path(nhr->ref->name);
            auto result = new IR::PathExpression(nhr->srcInfo, nhr->type, path);
            result->type = nhr->type;
            return result;
        } else if (structure->headerTypes.count(type->name)) {
            ref = structure->conversionContext->header->clone();
            auto result = new IR::Member(nhr->srcInfo, ref, nhr->ref->name);
            result->type = nhr->type;
            return result;
        }
    } else if (nhr->type->is<IR::Type_Struct>()) {
        auto type = nhr->type->to<IR::Type_Struct>();
        if (structure->parameterTypes.count(type->name)) {
            auto path = new IR::Path(nhr->ref->name);
            auto result = new IR::PathExpression(nhr->srcInfo, nhr->type, path);
            return result;
        } else if (structure->metadataTypes.count(type->name)) {
            ref = structure->conversionContext->header->clone();
            auto result = new IR::Member(nhr->srcInfo, ref, nhr->ref->name);
            return result;
        }
    }

    // convert types defined by user program
    if (structure->isHeader(nhr)) {
        ref = structure->conversionContext->header->clone();
    } else {
        if (nhr->ref->name == P4V1::V1Model::instance.standardMetadata.name)
            return structure->conversionContext->standardMetadata->clone();
        else
            ref = structure->conversionContext->userMetadata->clone();
    }
    auto result = new IR::Member(nhr->srcInfo, ref, nhr->ref->name);
    result->type = nhr->type;
    return result;
}

const IR::Node *ExpressionConverter::postorder(IR::HeaderStackItemRef *ref) {
    if (ref->index_->is<IR::PathExpression>()) {
        auto nr = ref->index_->to<IR::PathExpression>();
        if (nr->path->name == "last" || nr->path->name == "next") {
            cstring name = nr->path->name == "last" ? IR::Type_Stack::last : IR::Type_Stack::next;
            if (replaceNextWithLast && name == IR::Type_Stack::next) name = IR::Type_Stack::last;
            auto result = new IR::Member(ref->srcInfo, ref->base_, name);
            result->type = ref->base_->type;
            return result;
        }
    } else if (ref->index_->is<IR::Constant>()) {
        auto result = new IR::ArrayIndex(ref->srcInfo, ref->base_, ref->index_);
        result->type = ref->base_->type;
        return result;
    }

    ::error(ErrorType::ERR_UNSUPPORTED,
            "Illegal array index %1%: must be a constant, 'last', or 'next'.", ref);
    return ref;
}

const IR::Node *ExpressionConverter::postorder(IR::GlobalRef *ref) {
    // FIXME -- this is broken when the GlobalRef refers to something that the converter
    // FIXME -- has put into a different control.  In that case, ResolveReferences on this
    // FIXME -- path will later fail as the declaration is not in scope.  We should at
    // FIXME -- least detect that here and give a warning or other indication of the problem.
    return new IR::PathExpression(
        ref->srcInfo, new IR::Path(ref->srcInfo, IR::ID(ref->srcInfo, ref->toString())));
}

/// P4_16 is stricter on comparing booleans with ints
/// Therefore we convert such expressions into simply the boolean test
const IR::Node *ExpressionConverter::postorder(IR::Equ *equ) {
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
    if (boolExpr == nullptr) return equ;

    auto val = constExpr->to<IR::Constant>()->asInt();
    if (val == 1)
        return boolExpr;  // == 1 return the boolean
    else if (val == 0)
        return new IR::LNot(equ->srcInfo, boolExpr);  // return the !boolean
    else
        return new IR::BoolLiteral(equ->srcInfo, false);  // everything else is false
}

/// And the Neq
const IR::Node *ExpressionConverter::postorder(IR::Neq *neq) {
    const IR::Expression *boolExpr = nullptr;
    const IR::Expression *constExpr = nullptr;
    if (neq->left->type->is<IR::Type_Boolean>() && neq->right->is<IR::Constant>()) {
        boolExpr = neq->left;
        constExpr = neq->right;
    } else if (neq->right->type->is<IR::Type_Boolean>() && neq->left->is<IR::Constant>()) {
        boolExpr = neq->right;
        constExpr = neq->left;
    }

    if (boolExpr == nullptr) return neq;

    auto val = constExpr->to<IR::Constant>()->asInt();
    if (val == 0)
        return boolExpr;
    else if (val == 1)
        return new IR::LNot(neq->srcInfo, boolExpr);
    else
        return new IR::BoolLiteral(neq->srcInfo, true);  // everything else is true
}

std::map<cstring, ExpressionConverter::funcType> *ExpressionConverter::cvtForType = nullptr;

void ExpressionConverter::addConverter(cstring type, ExpressionConverter::funcType cvt) {
    static std::map<cstring, ExpressionConverter::funcType> tbl;
    cvtForType = &tbl;
    tbl[type] = cvt;
}

ExpressionConverter::funcType ExpressionConverter::get(cstring type) {
    if (cvtForType && cvtForType->count(type)) return cvtForType->at(type);
    return nullptr;
}

const IR::Node *StatementConverter::preorder(IR::Apply *apply) {
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
        const IR::Vector<IR::Expression> *hit = nullptr;
        const IR::Vector<IR::Expression> *miss = nullptr;
        bool otherLabels = false;
        for (auto a : apply->actions) {
            if (a.first == "hit") {
                if (hit != nullptr)
                    ::error(ErrorType::ERR_DUPLICATE, "%1%: Duplicate 'hit' label", hit);
                hit = a.second;
            } else if (a.first == "miss") {
                if (miss != nullptr)
                    ::error(ErrorType::ERR_DUPLICATE, "%1%: Duplicate 'miss' label", hit);
                miss = a.second;
            } else {
                otherLabels = true;
            }
        }

        if ((hit != nullptr || miss != nullptr) && otherLabels)
            ::error(ErrorType::ERR_INVALID, "%1%: Cannot mix 'hit'/'miss' and other labels", apply);

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
                    stat = conv.convert(a.second);
                }
                const IR::Expression *destination;
                if (a.first == "default") {
                    destination = new IR::DefaultExpression();
                } else {
                    cstring act_name = a.first;
                    auto path = apply->position.get<IR::Path>(act_name);
                    CHECK_NULL(path);
                    cstring full_name = table->name + '.' + act_name;
                    if (renameMap->count(full_name)) act_name = renameMap->at(full_name);
                    destination =
                        new IR::PathExpression(new IR::Path(path->srcInfo, IR::ID(act_name)));
                }
                auto swcase = new IR::SwitchCase(a.second->srcInfo, destination, stat);
                cases.insert(insert_at, swcase);
            }
            auto check = new IR::Member(call, IR::Type_Table::action_run);
            auto sw = new IR::SwitchStatement(apply->srcInfo, check, std::move(cases));
            prune();
            return sw;
        }
    }
}

const IR::Node *StatementConverter::preorder(IR::Primitive *primitive) {
    auto control = structure->controls.get(primitive->name);
    if (control != nullptr) {
        auto instanceName = ::get(renameMap, control->name);
        auto ctrl = new IR::PathExpression(IR::ID(instanceName));
        auto method = new IR::Member(ctrl, IR::ID(IR::IApply::applyMethodName));
        auto args = structure->createApplyArguments(control->name);
        auto call = new IR::MethodCallExpression(primitive->srcInfo, method, args);
        auto stat = new IR::MethodCallStatement(primitive->srcInfo, call);
        return stat;
    }
    // FIXME -- always a noop as ExpressionConverter has only postorder methods?
    return ExpressionConverter::preorder(primitive);
}

const IR::Node *StatementConverter::preorder(IR::If *cond) {
    StatementConverter conv(structure, renameMap);

    auto pred = apply_visitor(cond->pred)->to<IR::Expression>();
    BUG_CHECK(pred != nullptr, "Expected to get an expression when converting %1%", cond->pred);
    const IR::Statement *t, *f;
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

const IR::Statement *StatementConverter::convert(const IR::Vector<IR::Expression> *toConvert) {
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
                    error(ErrorType::ERR_UNSUPPORTED, "%s: max_length must be a constant", max);
                else
                    vbtype->size =
                        8 * max->expr[0]->to<IR::Constant>()->asInt() - type->width_bits();
            }
        }
    }
    if (vbtype->size == 0)
        error(ErrorType::ERR_NOT_FOUND, "%s: no max_length for * field size", vbtype);
    return vbtype;
}

namespace {

/// Checks that all references in a header length expression
/// are to fields prior to the varbit field.
class ValidateLenExpr : public Inspector {
    std::set<cstring> prior;
    const IR::StructField *varbitField;

 public:
    ValidateLenExpr(const IR::Type_StructLike *headerType, const IR::StructField *varbitField)
        : varbitField(varbitField) {
        for (auto f : headerType->fields) {
            if (f->name.name == varbitField->name.name) break;
            prior.emplace(f->name);
        }
    }
    void postorder(const IR::PathExpression *expression) override {
        BUG_CHECK(!expression->path->absolute, "%1%: absolute path", expression);
        cstring name = expression->path->name.name;
        if (prior.find(name) == prior.end())
            ::error(ErrorType::ERR_INVALID,
                    "%1%: header length must depend only on fields prior to the varbit field %2%",
                    expression, varbitField);
    }
};

}  // namespace

const IR::StructField *TypeConverter::postorder(IR::StructField *field) {
    auto type = findContext<IR::Type_StructLike>();
    if (type == nullptr) return field;

    // given a struct with length and max_length, the
    // varbit field size is max_length * 8 - struct_size
    if (field->type->is<IR::Type_Varbits>()) {
        if (auto len = type->getAnnotation("length")) {
            if (len->expr.size() == 1) {
                auto lenexpr = len->expr[0];
                ValidateLenExpr vle(type, field);
                vle.setCalledBy(this);
                lenexpr->apply(vle);
                auto scale = new IR::Mul(lenexpr->srcInfo, lenexpr, new IR::Constant(8));
                auto fieldlen =
                    new IR::Sub(scale->srcInfo, scale, new IR::Constant(type->width_bits()));
                field->annotations =
                    field->annotations->add(new IR::Annotation("length", {fieldlen}));
            }
        }
    }
    if (auto vec = structure->listIndexes(type->name.name, field->name.name))
        field->annotations = field->annotations->add(new IR::Annotation("field_list", *vec));
    return field;
}

const IR::Type_StructLike *TypeConverter::postorder(IR::Type_StructLike *str) {
    str->annotations = str->annotations->where([](const IR::Annotation *a) -> bool {
        return a->name != "length" && a->name != "max_length";
    });
    return str;
}

///////////////////////////////////////////////////////////////

namespace {
class FixupExtern : public Modifier {
    ProgramStructure *structure;
    cstring origname, extname;
    IR::TypeParameters *typeParams = nullptr;

    bool preorder(IR::Type_Extern *type) override {
        BUG_CHECK(!origname, "Nested extern");
        origname = type->name;
        return true;
    }
    void postorder(IR::Type_Extern *type) override {
        if (extname != type->name) {
            type->annotations = type->annotations->addAnnotationIfNew(
                IR::Annotation::nameAnnotation, new IR::StringLiteral(type->name.name), false);
            type->name = extname;
        }
        // FIXME -- should create ctors based on attributes?  For now just create a
        // FIXME -- 0-arg one if needed
        if (!type->lookupMethod(type->name, new IR::Vector<IR::Argument>())) {
            type->methods.push_back(new IR::Method(
                type->name, new IR::Type_Method(new IR::ParameterList(), type->getName())));
        }
    }
    void postorder(IR::Method *meth) override {
        if (meth->name == origname) meth->name = extname;
    }
    // Convert extern methods that take a field_list_calculation to take a type param instead
    bool preorder(IR::Type_MethodBase *mtype) override {
        BUG_CHECK(!typeParams, "recursion failure");
        typeParams = mtype->typeParameters->clone();
        return true;
    }
    bool preorder(IR::Parameter *param) override {
        BUG_CHECK(typeParams, "recursion failure");
        if (param->type->is<IR::Type_FieldListCalculation>()) {
            auto n = new IR::Type_Var(structure->makeUniqueName("FL"));
            param->type = n;
            typeParams->push_back(n);
        }
        return false;
    }
    void postorder(IR::Type_MethodBase *mtype) override {
        BUG_CHECK(typeParams, "recursion failure");
        if (*typeParams != *mtype->typeParameters) mtype->typeParameters = typeParams;
        typeParams = nullptr;
    }

 public:
    FixupExtern(ProgramStructure *s, cstring n) : structure(s), extname(n) {}
};
}  // namespace

const IR::Type_Extern *ExternConverter::convertExternType(ProgramStructure *structure,
                                                          const IR::Type_Extern *ext,
                                                          cstring name) {
    if (!ext->attributes.empty())
        warning(ErrorType::WARN_UNSUPPORTED, "%s: P4_14 extern type not fully supported", ext);
    return ext->apply(FixupExtern(structure, name))->to<IR::Type_Extern>();
}

const IR::Declaration_Instance *ExternConverter::convertExternInstance(
    ProgramStructure *structure, const IR::Declaration_Instance *ext, cstring name,
    IR::IndexedVector<IR::Declaration> *) {
    auto *rv = ext->clone();
    auto *et = rv->type->to<IR::Type_Extern>();
    BUG_CHECK(et, "Extern %s is not extern type, but %s", ext, ext->type);
    if (!ext->properties.empty())
        warning(ErrorType::WARN_UNSUPPORTED, "%s: P4_14 extern not fully supported", ext);
    if (structure->extern_remap.count(et)) et = structure->extern_remap.at(et);
    rv->name = name;
    rv->type = new IR::Type_Name(new IR::Path(structure->extern_types.get(et)));
    return rv->apply(TypeConverter(structure))->to<IR::Declaration_Instance>();
}

const IR::Statement *ExternConverter::convertExternCall(ProgramStructure *structure,
                                                        const IR::Declaration_Instance *ext,
                                                        const IR::Primitive *prim) {
    ExpressionConverter conv(structure);
    auto extref = new IR::PathExpression(structure->externs.get(ext));
    auto method = new IR::Member(prim->srcInfo, extref, prim->name);
    auto args = new IR::Vector<IR::Argument>();
    for (unsigned i = 1; i < prim->operands.size(); ++i)
        args->push_back(new IR::Argument(conv.convert(prim->operands.at(i))));
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
    if (cvtForType && cvtForType->count(type)) return cvtForType->at(type);
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
            if (auto *rv = cvt->convert(structure, primitive)) return rv;
    return nullptr;
}

safe_vector<const IR::Expression *> PrimitiveConverter::convertArgs(ProgramStructure *structure,
                                                                    const IR::Primitive *prim) {
    ExpressionConverter conv(structure);
    safe_vector<const IR::Expression *> rv;
    for (auto arg : prim->operands) rv.push_back(conv.convert(arg));
    return rv;
}

static ProgramStructure *defaultCreateProgramStructure() { return new ProgramStructure(); }

static ConversionContext *defaultCreateConversionContext() { return new ConversionContext(); }

ProgramStructure *(*Converter::createProgramStructure)() = defaultCreateProgramStructure;

ConversionContext *(*Converter::createConversionContext)() = defaultCreateConversionContext;

namespace {
class RemoveLengthAnnotations : public Transform {
    const IR::Node *postorder(IR::Annotation *annotation) override {
        if (annotation->name == "length") return nullptr;
        return annotation;
    }
};
}  // namespace

Converter::Converter() {
    setStopOnError(true);
    setName("Converter");
    structure = createProgramStructure();
    structure->conversionContext = createConversionContext();
    structure->conversionContext->clear();
    structure->populateOutputNames();

    // Discover types using P4-14 type-checker
    passes.emplace_back(new DetectDuplicates());
    passes.emplace_back(new P4::DoConstantFolding(nullptr, nullptr));
    passes.emplace_back(new CheckHeaderTypes());
    passes.emplace_back(new AdjustLengths());
    passes.emplace_back(new TypeCheck());
    // Convert
    passes.emplace_back(new DiscoverStructure(structure));
    passes.emplace_back(new FindRecirculated(structure));
    passes.emplace_back(new ComputeCallGraph(structure));
    passes.emplace_back(new ComputeTableCallGraph(structure));
    passes.emplace_back(new Rewriter(structure));
    passes.emplace_back(new FixExtracts(structure));
    passes.emplace_back(new FixMultiEntryPoint(structure));
    passes.emplace_back(new MoveIntrinsicMetadata(structure));
    passes.emplace_back(new RemoveLengthAnnotations());
}

Visitor::profile_t Converter::init_apply(const IR::Node *node) {
    if (!node->is<IR::V1Program>()) BUG("Converter only accepts IR::Globals, not %1%", node);
    return PassManager::init_apply(node);
}

}  // namespace P4V1
