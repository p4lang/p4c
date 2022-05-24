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

#ifndef _FRONTENDS_P4_FROMV1_0_CONVERTERS_H_
#define _FRONTENDS_P4_FROMV1_0_CONVERTERS_H_

#include <typeindex>
#include <typeinfo>

#include "ir/ir.h"
#include "lib/safe_vector.h"
#include "frontends/p4/coreLibrary.h"
#include "programStructure.h"

namespace P4V1 {

// Converts expressions from P4-14 to P4-16
// However, the type in each expression is still a P4-14 type.
class ExpressionConverter : public Transform {
 protected:
    ProgramStructure* structure;
    P4::P4CoreLibrary &p4lib;
    using funcType = std::function<const IR::Node*(const IR::Node*)>;
    static std::map<cstring, funcType> *cvtForType;

 public:
    bool replaceNextWithLast;  // if true p[next] becomes p.last
    explicit ExpressionConverter(ProgramStructure* structure)
            : structure(structure), p4lib(P4::P4CoreLibrary::instance),
              replaceNextWithLast(false) { setName("ExpressionConverter"); }
    const IR::Type* getFieldType(const IR::Type_StructLike* ht, cstring fieldName);
    const IR::Node* postorder(IR::Constant* expression) override;
    const IR::Node* postorder(IR::Member* field) override;
    const IR::Node* postorder(IR::FieldList* fl) override;
    const IR::Node* postorder(IR::Mask* expression) override;
    const IR::Node* postorder(IR::ActionArg* arg) override;
    const IR::Node* postorder(IR::Primitive* primitive) override;
    const IR::Node* postorder(IR::PathExpression* ref) override;
    const IR::Node* postorder(IR::ConcreteHeaderRef* nhr) override;
    const IR::Node* postorder(IR::HeaderStackItemRef* ref) override;
    const IR::Node* postorder(IR::GlobalRef *gr) override;
    const IR::Node* postorder(IR::Equ *equ) override;
    const IR::Node* postorder(IR::Neq *neq) override;
    const IR::Expression* convert(const IR::Node* node) {
        auto result = node->apply(*this);
        return result->to<IR::Expression>();
    }
    static void addConverter(cstring type, funcType);
    static funcType get(cstring type);
};

class StatementConverter : public ExpressionConverter {
    std::map<cstring, cstring> *renameMap;
 public:
    StatementConverter(ProgramStructure* structure, std::map<cstring, cstring> *renameMap)
            : ExpressionConverter(structure), renameMap(renameMap) {}

    const IR::Node* preorder(IR::Apply* apply) override;
    const IR::Node* preorder(IR::Primitive* primitive) override;
    const IR::Node* preorder(IR::If* cond) override;
    const IR::Statement* convert(const IR::Vector<IR::Expression>* toConvert);

    const IR::Statement* convert(const IR::Node* node) {
        auto conv = node->apply(*this);
        auto result = conv->to<IR::Statement>();
        BUG_CHECK(result != nullptr, "Conversion of %1% did not produce a statement", node);
        return result;
    }
};

class TypeConverter : public ExpressionConverter {
    const IR::Type_Varbits *postorder(IR::Type_Varbits *) override;
    const IR::Type_StructLike *postorder(IR::Type_StructLike *) override;
    const IR::StructField *postorder(IR::StructField *) override;
 public:
    explicit TypeConverter(ProgramStructure* structure) : ExpressionConverter(structure) {}
};

class ExternConverter {
    static std::map<cstring, ExternConverter *> *cvtForType;

 public:
    virtual const IR::Type_Extern *convertExternType(ProgramStructure *,
                const IR::Type_Extern *, cstring);
    virtual const IR::Declaration_Instance *convertExternInstance(ProgramStructure *,
                const IR::Declaration_Instance *, cstring, IR::IndexedVector<IR::Declaration> *);
    virtual const IR::Statement *convertExternCall(ProgramStructure *,
                const IR::Declaration_Instance *, const IR::Primitive *);
    virtual bool convertAsGlobal(ProgramStructure *, const IR::Declaration_Instance *) {
        return false; }
    ExternConverter() {}
    /// register a converter for a p4_14 extern_type
    /// @type: extern_type that the converter works on
    static void addConverter(cstring type, ExternConverter *);
    static ExternConverter *get(cstring type);
    static ExternConverter *get(const IR::Type_Extern *type) { return get(type->name); }
    static ExternConverter *get(const IR::Declaration_Instance *ext) {
        return get(ext->type->to<IR::Type_Extern>()); }
    static const IR::Type_Extern *cvtExternType(ProgramStructure *s,
                const IR::Type_Extern *e, cstring name) {
        return get(e)->convertExternType(s, e, name); }
    static const IR::Declaration_Instance *cvtExternInstance(ProgramStructure *s,
                const IR::Declaration_Instance *di, cstring name,
                IR::IndexedVector<IR::Declaration> *scope) {
        return get(di)->convertExternInstance(s, di, name, scope); }
    static const IR::Statement *cvtExternCall(ProgramStructure *s,
                const IR::Declaration_Instance *di, const IR::Primitive *p) {
        return get(di)->convertExternCall(s, di, p); }
    static bool cvtAsGlobal(ProgramStructure *s, const IR::Declaration_Instance *di) {
        return get(di)->convertAsGlobal(s, di); }
};

class PrimitiveConverter {
    static std::map<cstring, std::vector<PrimitiveConverter *>> *all_converters;
    cstring     prim_name;
    int         priority;

 protected:
    PrimitiveConverter(cstring name, int prio);
    virtual ~PrimitiveConverter();

    // helper functions
    safe_vector<const IR::Expression *> convertArgs(ProgramStructure *, const IR::Primitive *);

 public:
    virtual const IR::Statement *convert(ProgramStructure *, const IR::Primitive *) = 0;
    static  const IR::Statement *cvtPrimitive(ProgramStructure *, const IR::Primitive *);
};

/** Macro for defining PrimitiveConverter subclass singleton instances.
 * @name must be an identifier token -- the name of the primitive
 * second (optional) argument must be an integer constant priority
 * Multiple converters for the same primitive can be defined with different priorities
 * the highest priority converter will be run first, and if it returns nullptr, the
 * next highest will be run, etc.  The macro invocation is followed by the body of the
 * converter function.
 */
#define CONVERT_PRIMITIVE(NAME, ...) \
    class PrimitiveConverter_##NAME##_##__VA_ARGS__ : public PrimitiveConverter {               \
        const IR::Statement *convert(ProgramStructure *, const IR::Primitive *) override;       \
        PrimitiveConverter_##NAME##_##__VA_ARGS__()                                             \
        : PrimitiveConverter(#NAME, __VA_ARGS__ + 0) {}                                       \
        static PrimitiveConverter_##NAME##_##__VA_ARGS__ singleton;                             \
    } PrimitiveConverter_##NAME##_##__VA_ARGS__::singleton;                                     \
    const IR::Statement *PrimitiveConverter_##NAME##_##__VA_ARGS__::convert(                    \
        ProgramStructure *structure, const IR::Primitive *primitive)

///////////////////////////////////////////////////////////////

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
            ::error(ErrorType::ERR_INVALID,
                    "%1%: invalid name; it can only be used for %2%", node, it->second);
    }
    void checkReserved(const IR::Node* node, cstring nodeName) const {
        checkReserved(node, nodeName, nullptr);
    }

 public:
    explicit DiscoverStructure(ProgramStructure* structure) : structure(structure)
    { CHECK_NULL(structure); setName("DiscoverStructure"); }

    void postorder(const IR::ParserException* ex) override {
        warn(ErrorType::WARN_UNSUPPORTED, "%1%: parser exception is not translated to P4-16",
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
                ::error(ErrorType::ERR_NOT_FOUND, "%1%: Cannot find counter", ctrref);
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
                ::error(ErrorType::ERR_NOT_FOUND, "%1%: Cannot find meter", mtrref);
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
                ::error(ErrorType::ERR_NOT_FOUND, "%1%: Cannot find register", regref);
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
            ::error(ErrorType::ERR_NOT_FOUND, "%1%: Could not find table", apply->name);
        auto parent = findContext<IR::V1Control>();
        if (!parent)
            ::error(ErrorType::ERR_UNEXPECTED,
                    "%1%: Apply not within a control block?", apply);

        auto ctrl = get(structure->tableMapping, tbl);

        // skip control block that is unused.
        if (!structure->calledControls.isCallee(parent->name) &&
            parent->name != P4V1::V1Model::instance.ingress.name &&
            parent->name != P4V1::V1Model::instance.egress.name )
            return;

        if (ctrl != nullptr && ctrl != parent) {
            auto previous = get(structure->tableInvocation, tbl);
            ::error(ErrorType::ERR_INVALID,
                    "%1%: Table invoked from two different controls: %2% and %3%",
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
                    ::error(ErrorType::ERR_INVALID,
                            "%1%: header types with multiple varbit fields are not supported",
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
        rewrite.setCalledBy(this);
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
                            ::error(ErrorType::ERR_DUPLICATE, "%1%: same name as %2%", e1, e2);
                        else
                            // This name is probably standard_metadata_t, a built-in declaration
                            ::error(ErrorType::ERR_INVALID,
                                    "%1% is invalid; name %2% is reserved", e2, key);
                    }
                }
            }
            firstWithKey = range.second;
        }
        // prune; we're done; everything is top-level
        return false;
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
    IR::Vector<IR::Node>               allTypeDecls;
    IR::IndexedVector<IR::ParserState> parserStates;
    IR::Vector<IR::SelectCase>         selCases;
    cstring newStartState;
    cstring newInstanceType;

 public:
    explicit InsertCompilerGeneratedStartState(ProgramStructure* structure) : structure(structure) {
        setName("InsertCompilerGeneratedStartState");
        structure->allNames.emplace(IR::ParserState::start);
        structure->allNames.emplace("InstanceType");
        newStartState = structure->makeUniqueName(IR::ParserState::start);
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

    // Rename any path refering to original start state
    const IR::Node* postorder(IR::Path* path) override {
        if (!structure->parserEntryPoints.size())
            return path;
        // At this point any identifier called start should have been renamed
        // to unique name (e.g. start_1) => we can safely assume that any
        // "start" refers to the parser state
        if (path->name.name != IR::ParserState::start)
            return path;
        // Just to make sure we can also check it explicitly
        auto pe = getContext()->node->to<IR::PathExpression>();
        auto sc = findContext<IR::SelectCase>();
        auto ps = findContext<IR::ParserState>();
        // Either the path is within SelectCase->state<PathExpression>->path
        if (pe &&
            ((sc && pe->equiv(*sc->state->to<IR::PathExpression>())) ||
        // Or just within ParserState->selectExpression<PathExpression>->path
             (ps && pe->equiv(*ps->selectExpression->to<IR::PathExpression>()))))
            path->name = newStartState;
        return path;
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
        instAnnos->add(new IR::Annotation(IR::Annotation::nameAnnotation, ".$InstanceType"));
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
        annos->add(new IR::Annotation(IR::Annotation::nameAnnotation, ".$start"));
        auto startState = new IR::ParserState(IR::ParserState::start, annos, selects);
        parserStates.push_back(startState);

        if (!parserStates.empty()) {
            parser->states.append(parserStates);
            parserStates.clear();
        }
        return parser;
    }
};

/// Handle @packet_entry pragma in P4-14. A P4-14 program may be extended to
/// support multiple entry points to the parser. This feature does not comply
/// with P4-14 specification, but it is useful in certain use cases.
class FixMultiEntryPoint : public PassManager {
 public:
    explicit FixMultiEntryPoint(ProgramStructure* structure) {
        setName("FixMultiEntryPoint");
        passes.emplace_back(new CheckIfMultiEntryPoint(structure));
        passes.emplace_back(new InsertCompilerGeneratedStartState(structure));
    }
};

/**
   If the user metadata structure has a fields called
   "intrinsic_metadata" or "queueing_metadata" move all their fields
   to the standard_metadata Change all references appropriately.  We
   do this because the intrinsic_metadata and queueing_metadata are
   handled specially in P4-14 programs - much more like
   standard_metadata.
*/
class MoveIntrinsicMetadata : public Transform {
    ProgramStructure* structure;
    const IR::Type_Struct* stdType = nullptr;
    const IR::Type_Struct* userType = nullptr;
    const IR::Type_Struct* intrType = nullptr;
    const IR::Type_Struct* queueType = nullptr;
    const IR::StructField* intrField = nullptr;
    const IR::StructField* queueField = nullptr;

 public:
    explicit MoveIntrinsicMetadata(ProgramStructure* structure): structure(structure)
    { CHECK_NULL(structure); setName("MoveIntrinsicMetadata"); }
    const IR::Node* preorder(IR::P4Program* program) override {
        stdType = program->getDeclsByName(
            structure->v1model.standardMetadataType.name)->single()->to<IR::Type_Struct>();
        userType = program->getDeclsByName(
            structure->v1model.metadataType.name)->single()->to<IR::Type_Struct>();
        CHECK_NULL(stdType);
        CHECK_NULL(userType);
        intrField = userType->getField(structure->v1model.intrinsicMetadata.name);
        if (intrField != nullptr) {
            auto intrTypeName = intrField->type;
            auto tn = intrTypeName->to<IR::Type_Name>();
            BUG_CHECK(tn, "%1%: expected a Type_Name", intrTypeName);
            auto nt = program->getDeclsByName(tn->path->name)->nextOrDefault();
            if (nt == nullptr || !nt->is<IR::Type_Struct>()) {
                ::error(ErrorType::ERR_INVALID, "%1%: expected a structure", tn);
                return program;
            }
            intrType = nt->to<IR::Type_Struct>();
            LOG2("Intrinsic metadata type " << intrType);
        }

        queueField = userType->getField(structure->v1model.queueingMetadata.name);
        if (queueField != nullptr) {
            auto queueTypeName = queueField->type;
            auto tn = queueTypeName->to<IR::Type_Name>();
            BUG_CHECK(tn, "%1%: expected a Type_Name", queueTypeName);
            auto nt = program->getDeclsByName(tn->path->name)->nextOrDefault();
            if (nt == nullptr || !nt->is<IR::Type_Struct>()) {
                ::error(ErrorType::ERR_INVALID, "%1%: expected a structure", tn);
                return program;
            }
            queueType = nt->to<IR::Type_Struct>();
            LOG2("Queueing metadata type " << queueType);
        }
        return program;
    }

    const IR::Node* postorder(IR::Type_Struct* type) override {
        if (getOriginal() == stdType) {
            if (intrType != nullptr) {
                for (auto f : intrType->fields) {
                    if (type->fields.getDeclaration(f->name) == nullptr) {
                        ::error(ErrorType::ERR_NOT_FOUND,
                                "%1%: no such field in standard_metadata", f->name);
                        LOG2("standard_metadata: " << type);
                    }
                }
            }
            if (queueType != nullptr) {
                for (auto f : queueType->fields) {
                    if (type->fields.getDeclaration(f->name) == nullptr) {
                        ::error(ErrorType::ERR_NOT_FOUND,
                                "%1%: no such field in standard_metadata", f->name);
                        LOG2("standard_metadata: " << type);
                    }
                }
            }
        }
        return type;
    }

    const IR::Node* postorder(IR::StructField* field) override {
        if (getOriginal() == intrField || getOriginal() == queueField)
            // delete it from its parent
            return nullptr;
        return field;
    }

    const IR::Node* postorder(IR::Member* member) override {
        // We rewrite expressions like meta.intrinsic_metadata.x as
        // standard_metadata.x.  We know that these parameter names
        // are always the same.
        if (member->member != structure->v1model.intrinsicMetadata.name &&
            member->member != structure->v1model.queueingMetadata.name)
            return member;
        auto pe = member->expr->to<IR::PathExpression>();
        if (pe == nullptr || pe->path->absolute)
            return member;
        if (pe->path->name == structure->v1model.parser.metadataParam.name) {
            LOG2("Renaming reference " << member);
            return new IR::PathExpression(
                new IR::Path(member->expr->srcInfo,
                             IR::ID(pe->path->name.srcInfo,
                                    structure->v1model.standardMetadata.name)));
        }
        return member;
    }
};

/// This visitor finds all field lists that participate in
/// recirculation, resubmission, and cloning
class FindRecirculated : public Inspector {
    ProgramStructure* structure;

    void add(const IR::Primitive* primitive, unsigned operand) {
        if (primitive->operands.size() <= operand) {
            // not enough arguments, do nothing.
            // resubmit and recirculate have optional arguments
            return;
        }
        auto expression = primitive->operands.at(operand);
        if (!expression->is<IR::PathExpression>()) {
            ::error(ErrorType::ERR_EXPECTED, "%1%: expected a field list", expression);
            return;
        }
        auto nr = expression->to<IR::PathExpression>();
        auto fl = structure->field_lists.get(nr->path->name);
        if (fl == nullptr) {
            ::error(ErrorType::ERR_EXPECTED, "%1%: Expected a field list", expression);
            return;
        }
        LOG3("Recirculated " << nr->path->name);
        structure->allFieldLists.emplace(fl);
    }

 public:
    explicit FindRecirculated(ProgramStructure* structure): structure(structure)
    { CHECK_NULL(structure); setName("FindRecirculated"); }

    void postorder(const IR::Primitive* primitive) override {
        if (primitive->name == "recirculate") {
            add(primitive, 0);
        } else if (primitive->name == "resubmit") {
            add(primitive, 0);
        } else if (primitive->name.startsWith("clone") && primitive->operands.size() == 2) {
            add(primitive, 1);
        }
    }
};

///////////////////////////////////////////////////////////////

// Is fed a P4-14 program and outputs an equivalent P4-16 program in v1model
class Converter : public PassManager {
 public:
    ProgramStructure *structure;
    static ProgramStructure *(*createProgramStructure)();
    static ConversionContext *(*createConversionContext)();
    Converter();
    void loadModel() { structure->loadModel(); }
    Visitor::profile_t init_apply(const IR::Node* node) override;
};



}  // namespace P4V1

#endif /* _FRONTENDS_P4_FROMV1_0_CONVERTERS_H_ */
