/*
Copyright 2020 Intel Corp.

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

#ifndef BACKENDS_DPDK_DPDKARCH_H_
#define BACKENDS_DPDK_DPDKARCH_H_

#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeMap.h"
#include "frontends/p4/sideEffects.h"
#include "midend/removeLeftSlices.h"
#include "lib/error.h"
#include "lib/ordered_map.h"
#include "dpdkProgramStructure.h"

namespace DPDK {

cstring TypeStruct2Name(const cstring *s);
bool isSimpleExpression(const IR::Expression *e);
bool isNonConstantSimpleExpression(const IR::Expression *e);
void expressionUnrollSanityCheck(const IR::Expression *e);

using UserMeta = std::set<cstring>;

class CollectMetadataHeaderInfo;

/* According to the implementation of DPDK backend, for a control block, there
 * are only two parameters: header and metadata. Therefore, first we need to
 * rewrite the declaration of PSA architecture included in psa.p4 in order to
 * pass the type checking. In addition, this pass changes the definition of
 * P4Control and P4Parser(parameter list) in the P4 program provided by the
 * user.
 *
 * This pass also modifies all metadata references and header reference. For
 * metadata, struct_name.field_name -> m.struct_name_field_name. For header
 * headers.header_name.field_name -> h.header_name.field_name The parameter
 * named for header and metadata are also updated to "h" and "m" respectively.
 */
class ConvertToDpdkArch : public Transform {
    P4::ReferenceMap *refMap;
    DpdkProgramStructure *structure;

    const IR::Type_Control *rewriteControlType(const IR::Type_Control *, cstring);
    const IR::Type_Parser *rewriteParserType(const IR::Type_Parser *, cstring);
    const IR::Type_Control *rewriteDeparserType(const IR::Type_Control *, cstring);
    const IR::Node *postorder(IR::Type_Control *c) override;
    const IR::Node *postorder(IR::Type_Parser *p) override;
    const IR::Node *preorder(IR::Member *m) override;
    const IR::Node *preorder(IR::PathExpression *pe) override;

 public:
    ConvertToDpdkArch(P4::ReferenceMap *refMap, DpdkProgramStructure *structure) :
        refMap(refMap), structure(structure) {
        CHECK_NULL(structure);
    }
};

/**
 * DPDK target supports lookahead instruction only with header operand.
 * This pass transforms usage of lookahead method with type parameters
 * of other than header type (IR::Type_Header) to lookahead with header
 * type using the following transformation:
 *
 * This parser code:
 *
 * T var_name;
 *
 * state state_name {
 *   var_name = pkt.lookahead<T>();
 * }
 *
 * is transformed to this:
 *
 * - new header definition:
 *
 * header lookahead_tmp_hdr {
 *   T f;
 * }
 *
 * - modification in parser code:
 *
 * T var_name;
 * lookahead_tmp_hdr lookahead_tmp;
 *
 * state state_name {
 *   lookahead_tmp = pkt.lookahead<lookahead_tmp_hdr>();
 *   var_name = lookahead_tmp;
 * }
 */
struct ConvertLookahead : public PassManager {
    class ReplacementMap {
        std::unordered_map<const IR::P4Program *, IR::IndexedVector<IR::Node>> newHeaderMap;
        std::unordered_map<const IR::P4Parser *,
                IR::IndexedVector<IR::Declaration>> newLocalVarMap;
        std::unordered_map<const IR::AssignmentStatement *,
                IR::IndexedVector<IR::StatOrDecl>> newStatMap;

     public:
        void insertHeader(const IR::P4Program *p, const IR::Type_Header *h) {
            if (newHeaderMap.count(p)) {
                newHeaderMap.at(p).push_back(h);
            } else {
                newHeaderMap.emplace(p, IR::IndexedVector<IR::Node>(h));
            }
            LOG5("Program: " << dbp(p));
            LOG2("Adding new header:" << std::endl << " " << h);
        }
        IR::IndexedVector<IR::Node> *getHeaders(const IR::P4Program *p) {
            if (newHeaderMap.count(p)) {
                return new IR::IndexedVector<IR::Node>(newHeaderMap.at(p));
            }
            return nullptr;
        }
        void insertVar(const IR::P4Parser *p, const IR::Declaration_Variable *v) {
            if (newLocalVarMap.count(p)) {
                newLocalVarMap.at(p).push_back(v);
            } else {
                newLocalVarMap.emplace(p, IR::IndexedVector<IR::Declaration>(v));
            }
            LOG5("Parser: " << dbp(p));
            LOG2("Adding new local variable:" << std::endl << " " << v);
        }
        IR::IndexedVector<IR::Declaration> *getVars(const IR::P4Parser *p) {
            if (newLocalVarMap.count(p)) {
                return new IR::IndexedVector<IR::Declaration>(newLocalVarMap.at(p));
            }
            return nullptr;
        }
        void insertStatements(const IR::AssignmentStatement *as,
                IR::IndexedVector<IR::StatOrDecl> *vec) {
            BUG_CHECK(newStatMap.count(as) == 0,
                    "Unexpectedly converting statement %1% multiple times!", as);
            newStatMap.emplace(as, *vec);
            LOG5("AssignmentStatement: " << dbp(as));
            LOG2("Adding new statements:");
            for (auto s : *vec) {
                LOG2(" " << s);
            }
        }
        IR::IndexedVector<IR::StatOrDecl> *getStatements(const IR::AssignmentStatement *as) {
            if (newStatMap.count(as)) {
                return new IR::IndexedVector<IR::StatOrDecl>(newStatMap.at(as));
            }
            return nullptr;
        }
    };

    ReplacementMap repl;

    class Collect : public Inspector {
        P4::ReferenceMap *refMap;
        P4::TypeMap *typeMap;
        ReplacementMap *repl;

     public:
        Collect(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, ReplacementMap *repl) :
                refMap(refMap), typeMap(typeMap), repl(repl) {}
        void postorder(const IR::AssignmentStatement *statement) override;
    };

    class Replace : public Transform {
        DpdkProgramStructure *structure;
        ReplacementMap *repl;

     public:
        Replace(DpdkProgramStructure *structure, ReplacementMap *repl) :
                structure(structure), repl(repl) {}
        const IR::Node *postorder(IR::AssignmentStatement *as) override;
        const IR::Node *postorder(IR::Type_Struct *s) override;
        const IR::Node *postorder(IR::P4Parser *parser) override;
    };

    ConvertLookahead(P4::ReferenceMap *refMap, P4::TypeMap *typeMap, DpdkProgramStructure *s) {
        passes.push_back(new P4::TypeChecking(refMap, typeMap));
        passes.push_back(new Collect(refMap, typeMap, &repl));
        passes.push_back(new Replace(s, &repl));
        passes.push_back(new P4::ClearTypeMap(typeMap));
    }
};

// This Pass collects infomation about the name of all metadata and header
// And it collects every field of metadata and renames all fields with a prefix
// according to the metadata struct name. Eventually, the reference of a fields
// will become m.$(struct_name)_$(field_name).
class CollectMetadataHeaderInfo : public Inspector {
    DpdkProgramStructure *structure;

    void pushMetadata(const IR::Parameter *p);
    void pushMetadata(const IR::ParameterList*, std::list<int> indices);

 public:
    explicit CollectMetadataHeaderInfo(DpdkProgramStructure *structure)
        : structure(structure) {}
    bool preorder(const IR::P4Program *p) override;
    bool preorder(const IR::Type_Struct *s) override;
};

// Previously, we have collected the information about how the single metadata
// struct looks like in CollectMetadataHeaderInfo. This pass finds a suitable
// place to inject this struct.
class InjectJumboStruct : public Transform {
    DpdkProgramStructure *structure;

 public:
    explicit InjectJumboStruct(DpdkProgramStructure *structure) : structure(structure) {}
    const IR::Node *preorder(IR::Type_Struct *s) override;
};

// This pass injects metadata field which is used as port for 'tx' instruction
// into the single metadata struct.
// This pass has to be applied after CollectMetadataHeaderInfo fills
// local_metadata_type field in DpdkProgramStructure which is passed to the constructor.
class InjectOutputPortMetadataField : public Transform {
    DpdkProgramStructure *structure;

 public:
    explicit InjectOutputPortMetadataField(DpdkProgramStructure *structure) :
        structure(structure) {}
    const IR::Node *preorder(IR::Type_Struct *s) override;
};

class AlignHdrMetaField : public Transform {
    P4::TypeMap* typeMap;
    P4::ReferenceMap *refMap;
    DpdkProgramStructure *structure;

    ordered_map<cstring, struct fieldInfo> field_name_list;

 public:
    AlignHdrMetaField(P4::TypeMap* typeMap,
                            P4::ReferenceMap *refMap,
                            DpdkProgramStructure* structure)
        : typeMap(typeMap), refMap(refMap), structure(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node *preorder(IR::Type_StructLike *st) override;
    const IR::Node *preorder(IR::Member *m) override;
};

struct ByteAlignment : public PassManager {
    P4::TypeMap* typeMap;
    P4::ReferenceMap *refMap;
    DpdkProgramStructure *structure;
 public:
    ByteAlignment(P4::TypeMap* typeMap,
                        P4::ReferenceMap *refMap,
                        DpdkProgramStructure* structure)
    : typeMap(typeMap), refMap(refMap), structure(structure) {
        CHECK_NULL(structure);
        passes.push_back(new AlignHdrMetaField(typeMap, refMap, structure));
        passes.push_back(new P4::ClearTypeMap(typeMap));
        passes.push_back(new P4::TypeChecking(refMap, typeMap, true));
        /* DoRemoveLeftSlices pass converts the slice Member (LHS in assn stm)
           resulting from above Pass into shift operation */
        passes.push_back(new P4::DoRemoveLeftSlices(typeMap));
        passes.push_back(new P4::ClearTypeMap(typeMap));
        passes.push_back(new P4::TypeChecking(refMap, typeMap, true));
    }
};

class ReplaceHdrMetaField : public Transform {
    P4::TypeMap* typeMap;
    P4::ReferenceMap *refMap;
    DpdkProgramStructure *structure;
 public:
    ReplaceHdrMetaField(P4::TypeMap* typeMap,
                            P4::ReferenceMap *refMap,
                            DpdkProgramStructure* structure)
        : typeMap(typeMap), refMap(refMap), structure(structure) {
        CHECK_NULL(structure);
    }
    const IR::Node* postorder(IR::Type_Struct *st) override;
};

struct fieldInfo {
    unsigned fieldWidth;
    fieldInfo() {
        fieldWidth = 0;
    }
};

// This class is helpful for StatementUnroll and IfStatementUnroll. Since dpdk
// asm is not able to process complex expression, e.g., a = b + c * d. We need
// break it down. Therefore, we need some temporary variables to hold the
// intermediate values. And this class is helpful to inject the declarations of
// temporary value into P4Control and P4Parser.
class DeclarationInjector {
    std::map<const IR::Node *, IR::IndexedVector<IR::Declaration> *> decl_map;

 public:
    // push the declaration to the right code block.
    void collect(const IR::P4Control *control, const IR::P4Parser *parser,
                 const IR::Declaration *decl) {
        IR::IndexedVector<IR::Declaration> *decls = nullptr;
        if (parser) {
            auto res = decl_map.find(parser);
            if (res != decl_map.end()) {
                decls = res->second;
            } else {
                decls = new IR::IndexedVector<IR::Declaration>;
                decl_map.emplace(parser, decls);
            }
        } else if (control) {
            auto res = decl_map.find(control);
            if (res != decl_map.end()) {
                decls = res->second;
            } else {
                decls = new IR::IndexedVector<IR::Declaration>;
                decl_map.emplace(control, decls);
            }
        }
        BUG_CHECK(decls != nullptr, "decls cannot be null");
        decls->push_back(decl);
    }
    IR::Node *inject_control(const IR::Node *orig, IR::P4Control *control) {
        auto res = decl_map.find(orig);
        if (res == decl_map.end()) {
            return control;
        }
        control->controlLocals.prepend(*res->second);
        return control;
    }
    IR::Node *inject_parser(const IR::Node *orig, IR::P4Parser *parser) {
        auto res = decl_map.find(orig);
        if (res == decl_map.end()) {
            return parser;
        }
        parser->parserLocals.prepend(*res->second);
        return parser;
    }
};

/* This pass breaks complex expressions down, since dpdk asm cannot describe
 * complex expression. This pass is not complete. MethodCallStatement should be
 * unrolled as well. Note that IfStatement should not be unrolled here, as we
 * have a separate pass for it, because IfStatement does not want to unroll
 * logical expression(dpdk asm has conditional jmp for these cases)
 */
class StatementUnroll : public Transform {
 private:
    P4::ReferenceMap *refMap;
    DpdkProgramStructure *structure;
    DeclarationInjector injector;

 public:
    StatementUnroll(P4::ReferenceMap *refMap, DpdkProgramStructure *structure) :
        refMap(refMap), structure(structure) {}
    const IR::Node *preorder(IR::AssignmentStatement *a) override;
    const IR::Node *postorder(IR::P4Control *a) override;
    const IR::Node *postorder(IR::P4Parser *a) override;
};

/* This pass helps StatementUnroll to unroll expressions inside statements.
 * For example, if an AssignmentStatement looks like this: a = b + c * d
 * StatementUnroll's AssignmentStatement preorder will call ExpressionUnroll
 * twice for BinaryExpression's left(b) and right(c * d). For left, since it is
 * a simple expression, ExpressionUnroll will set root to PathExpression(b) and
 * the decl and stmt is empty. For right, ExpressionUnroll will set root to
 * PathExpression(tmp), decl contains tmp's declaration and stmt contains:
 * tmp = c * d, which will be injected in front of the AssignmentStatement.
 */
class ExpressionUnroll : public Inspector {
    P4::ReferenceMap *refMap;

 public:
    IR::IndexedVector<IR::StatOrDecl> stmt;
    IR::IndexedVector<IR::Declaration> decl;
    IR::PathExpression *root;
    ExpressionUnroll(P4::ReferenceMap *refMap, DpdkProgramStructure *) :
        refMap(refMap) {
        setName("ExpressionUnroll");
    }
    bool preorder(const IR::Operation_Unary *a) override;
    bool preorder(const IR::Operation_Binary *a) override;
    bool preorder(const IR::MethodCallExpression *a) override;
    bool preorder(const IR::Member *a) override;
    bool preorder(const IR::PathExpression *a) override;
    bool preorder(const IR::Constant *a) override;
    bool preorder(const IR::BoolLiteral *a) override;
};

// This pass is similiar to StatementUnroll pass, the difference is that this
// pass will call LogicalExpressionUnroll to unroll the expression, which treat
// logical expression differently.
class IfStatementUnroll : public Transform {
 private:
    P4::ReferenceMap *refMap;
    DpdkProgramStructure *structure;
    DeclarationInjector injector;

 public:
    IfStatementUnroll(P4::ReferenceMap* refMap, DpdkProgramStructure *structure) :
        refMap(refMap), structure(structure) {
        setName("IfStatementUnroll");
    }
    const IR::Node *postorder(IR::SwitchStatement *a) override;
    const IR::Node *postorder(IR::IfStatement *a) override;
    const IR::Node *postorder(IR::P4Control *a) override;
};

/* Assume one logical expression looks like this: a && (b + c > d), this pass
 * will unroll the expression to {tmp = b + c; if(a && (tmp > d))}. Logical
 * calculation will be unroll in a dedicated pass.
 */
class LogicalExpressionUnroll : public Inspector {
    P4::ReferenceMap* refMap;
    DpdkProgramStructure *structure;

 public:
    IR::IndexedVector<IR::StatOrDecl> stmt;
    IR::IndexedVector<IR::Declaration> decl;
    IR::Expression *root;
    static bool is_logical(const IR::Operation_Binary *bin) {
        if (bin->is<IR::LAnd>() || bin->is<IR::LOr>() || bin->is<IR::Leq>() ||
            bin->is<IR::Equ>() || bin->is<IR::Neq>() || bin->is<IR::Grt>() ||
            bin->is<IR::Lss>() || bin->is<IR::Geq>() || bin->is<IR::Leq>())
            return true;
          else
            return false;
    }

    LogicalExpressionUnroll(P4::ReferenceMap* refMap, DpdkProgramStructure *structure)
        : refMap(refMap), structure(structure) {visitDagOnce = false;}
    bool preorder(const IR::Operation_Unary *a) override;
    bool preorder(const IR::Operation_Binary *a) override;
    bool preorder(const IR::MethodCallExpression *a) override;
    bool preorder(const IR::Member *a) override;
    bool preorder(const IR::PathExpression *a) override;
    bool preorder(const IR::Constant *a) override;
    bool preorder(const IR::BoolLiteral *a) override;
};

// According to dpdk spec, Binary Operation will only have two parameters, which
// looks like: a = a + b. Therefore, this pass transform all AssignStatement
// that has Binary_Operation to become two-parameter form.
class ConvertBinaryOperationTo2Params : public Transform {
    DeclarationInjector injector;

 public:
    ConvertBinaryOperationTo2Params() {}
    const IR::Node *postorder(IR::AssignmentStatement *a) override;
    const IR::Node *postorder(IR::P4Control *a) override;
    const IR::Node *postorder(IR::P4Parser *a) override;
};

// Since in dpdk asm, there is no local variable declaration, we need to collect
// all local variables and inject them into the metadata struct.
// Local variables which are of header types are injected into headers struct
// instead of metadata struct, so that they can be instantiated as headers in the
// resulting dpdk asm file.
class CollectLocalVariables : public Transform {
    ordered_map<const IR::Declaration_Variable *, const cstring> localsMap;
    P4::ReferenceMap *refMap;
    P4::TypeMap* typeMap;
    DpdkProgramStructure *structure;

    void insert(const cstring prefix, const IR::IndexedVector<IR::Declaration> *locals) {
        for (auto d : *locals) {
            if (auto dv = d->to<IR::Declaration_Variable>()) {
                const cstring name = refMap->newName(prefix + "_" + dv->name.name);
                localsMap.emplace(dv, name);
            } else if (!d->is<IR::P4Action>() && !d->is<IR::P4Table>() &&
                       !d->is<IR::Declaration_Instance>()) {
                BUG("%1%: Unhandled declaration type", d);
            }
        }
    }

 public:
    CollectLocalVariables(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                                   DpdkProgramStructure *structure)
        : refMap(refMap), typeMap(typeMap), structure(structure) {}
    const IR::Node *preorder(IR::P4Program *p) override;
    const IR::Node *postorder(IR::Type_Struct *s) override;
    const IR::Node *postorder(IR::Member *m) override;
    const IR::Node *postorder(IR::PathExpression *path) override;
    const IR::Node *postorder(IR::P4Control *c) override;
    const IR::Node *postorder(IR::P4Parser *p) override;
};

// According to dpdk spec, action parameters should prepend a p. In order to
// respect this, we need at first make all action parameter lists into separate
// structs and declare that struct in the P4 program. Then we modify the action
// parameter list. Eventually, it will only contain one parameter `t`, which is a
// struct containing all parameters previously defined. Next, we prepend t. in
// front of action parameters. Please note that it is possible that the user
// defines a struct paremeter himself or define multiple struct parameters in
// action parameterlist. Current implementation does not support this.
class PrependPDotToActionArgs : public Transform {
    P4::TypeMap* typeMap;
    P4::ReferenceMap *refMap;
    DpdkProgramStructure* structure;

 public:
    PrependPDotToActionArgs(P4::TypeMap* typeMap,
                            P4::ReferenceMap *refMap,
                            DpdkProgramStructure* structure)
        : typeMap(typeMap), refMap(refMap), structure(structure) {}
    const IR::Node *postorder(IR::P4Action *a) override;
    const IR::Node *postorder(IR::P4Program *s) override;
    const IR::Node *preorder(IR::PathExpression *path) override;
    const IR::Node *preorder(IR::MethodCallExpression*) override;
};

// dpdk does not support ternary operator so we need to translate ternary operator
// to corresponding if else statement
// Taken from frontend pass DoSimplifyExpressions in sideEffects.h
class DismantleMuxExpressions : public Transform {
    P4::TypeMap* typeMap;
    P4::ReferenceMap *refMap;
    IR::IndexedVector<IR::Declaration> toInsert;  // temporaries
    IR::IndexedVector<IR::StatOrDecl> statements;

    cstring createTemporary(const IR::Type* type);
    const IR::Expression* addAssignment(Util::SourceInfo srcInfo, cstring varName,
                                        const IR::Expression* expression);

 public:
    DismantleMuxExpressions(P4::TypeMap* typeMap,
                            P4::ReferenceMap *refMap)
        : typeMap(typeMap), refMap(refMap) {}
    const IR::Node* preorder(IR::Mux* expression) override;
    const IR::Node* postorder(IR::P4Parser* parser) override;
    const IR::Node* postorder(IR::Function* function) override;
    const IR::Node* postorder(IR::P4Control* control) override;
    const IR::Node* postorder(IR::P4Action* action) override;
    const IR::Node* postorder(IR::AssignmentStatement* statement) override;
};

// For dpdk asm, there is not object-oriented. Therefore, we cannot define a
// checksum in dpdk asm. And dpdk asm only provides ckadd(checksum add) and
// cksub(checksum sub). So we need to define a explicit state for each checksum
// declaration. Essentially, this state will be declared in header struct and
// initilized to 0. And for cksum.add(x), it will be translated to ckadd state
// x. For dst = cksum.get(), it will be translated to mov dst state. This pass
// collects checksum instances and index them.
class CollectInternetChecksumInstance : public Inspector {
    P4::TypeMap* typeMap;
    DpdkProgramStructure *structure;
    int index = 0;

 public:
    CollectInternetChecksumInstance(
            P4::TypeMap *typeMap,
            DpdkProgramStructure *structure)
        : typeMap(typeMap), structure(structure) {}
    bool preorder(const IR::Declaration_Instance *d) override {
        auto type = typeMap->getType(d, true);
        if (auto extn = type->to<IR::Type_Extern>()) {
            if (extn->name == "InternetChecksum") {
                std::ostringstream s;
                s << "state_" << index++;
                structure->csum_map.emplace(d, s.str());
            }
        }
        return false;
    }
};

// This pass will inject checksum states into header. The reason why we inject
// state into header instead of metadata is due to the implementation of dpdk
// side(a question related to endianness)
class InjectInternetChecksumIntermediateValue : public Transform {
    DpdkProgramStructure *structure;

 public:
    explicit InjectInternetChecksumIntermediateValue(DpdkProgramStructure *structure) :
        structure(structure) {}

    const IR::Node *postorder(IR::P4Program *p) override {
        auto new_objs = new IR::Vector<IR::Node>;
        bool inserted = false;
        for (auto obj : p->objects) {
            if (obj->to<IR::Type_Header>() && !inserted) {
                inserted = true;
                if (structure->csum_map.size() > 0) {
                    auto fields = new IR::IndexedVector<IR::StructField>;
                    for (auto kv : structure->csum_map) {
                        fields->push_back(new IR::StructField(
                            IR::ID(kv.second), new IR::Type_Bits(16, false)));
                    }
                    new_objs->push_back(
                        new IR::Type_Header(IR::ID("cksum_state_t"), *fields));
                }
            }
            new_objs->push_back(obj);
        }
        p->objects = *new_objs;
        return p;
    }

    const IR::Node *postorder(IR::Type_Struct *s) override {
        if (s->name.name == structure->header_type) {
            if (structure->csum_map.size() > 0)
                s->fields.push_back(new IR::StructField(
                    IR::ID("cksum_state"),
                    new IR::Type_Name(IR::ID("cksum_state_t"))));
        }
        return s;
    }
};

class ConvertInternetChecksum : public PassManager {
 public:
    ConvertInternetChecksum(P4::TypeMap *typeMap, DpdkProgramStructure *structure) {
        passes.push_back(new CollectInternetChecksumInstance(typeMap, structure));
        passes.push_back(new InjectInternetChecksumIntermediateValue(structure));
    }
};

/* This pass collects PSA extern meter, counter and register declaration instances and
   push them to a vector for emitting to the .spec file later */
class CollectExternDeclaration : public Inspector {
    DpdkProgramStructure *structure;

 public:
    explicit CollectExternDeclaration(DpdkProgramStructure *structure) :
        structure(structure) {}
    bool preorder(const IR::Declaration_Instance *d) override {
        if (auto type = d->type->to<IR::Type_Specialized>()) {
            auto externTypeName = type->baseType->path->name.name;
            if (externTypeName == "Meter") {
                if (d->arguments->size() != 2) {
                    ::error("%1%: expected number of meters and type of meter as arguments", d);
                } else {
                    /* Check if the meter is of PACKETS (0) type */
                    if (d->arguments->at(1)->expression->to<IR::Constant>()->asUnsigned() == 0)
                        warn(ErrorType::WARN_UNSUPPORTED,
                             "%1%: Packet metering is not supported."
                             " Falling back to byte metering.", d);
                }
            } else if (externTypeName == "Counter") {
                if (d->arguments->size() != 2) {
                    ::error("%1%: expected number of_counters and type of counter as arguments", d);
                }
            } else if (externTypeName == "Register") {
                if (d->arguments->size() != 1 && d->arguments->size() != 2) {
                    ::error("%1%: expected size and optionally init_val as arguments", d);
                }
            } else {
                // unsupported extern type
                return false;
            }
            structure->externDecls.push_back(d);
         }
         return false;
    }
};

// This pass is preparing logical expression for following branching statement
// optimization. This pass breaks parenthesis looks liks this: (a && b) && c.
// After this pass, the expression looks like this: a && b && c. (The AST is
// different).
class BreakLogicalExpressionParenthesis : public Transform {
 public:
    const IR::Node *postorder(IR::LAnd *land) {
        if (auto land2 = land->left->to<IR::LAnd>()) {
            auto sub = new IR::LAnd(land2->right, land->right);
            return new IR::LAnd(land2->left, sub);
        } else if (!land->left->is<IR::LOr>() &&
                   !land->left->is<IR::Equ>() &&
                   !land->left->is<IR::Neq>() &&
                   !land->left->is<IR::Leq>() &&
                   !land->left->is<IR::Geq>() &&
                   !land->left->is<IR::Lss>() &&
                   !land->left->is<IR::Grt>() &&
                   !land->left->is<IR::MethodCallExpression>() &&
                   !land->left->is<IR::PathExpression>() &&
                   !land->left->is<IR::Member>()) {
            BUG("Logical Expression Unroll pass failed");
        }
        return land;
    }
    const IR::Node *postorder(IR::LOr *lor) {
        if (auto lor2 = lor->left->to<IR::LOr>()) {
            auto sub = new IR::LOr(lor2->right, lor->right);
            return new IR::LOr(lor2->left, sub);
        } else if (!lor->left->is<IR::LOr>() &&
                   !lor->left->is<IR::Equ>() &&
                   !lor->left->is<IR::Neq>() &&
                   !lor->left->is<IR::Lss>() &&
                   !lor->left->is<IR::Grt>() &&
                   !lor->left->is<IR::MethodCallExpression>() &&
                   !lor->left->is<IR::PathExpression>() &&
                   !lor->left->is<IR::Member>()) {
            BUG("Logical Expression Unroll pass failed");
        }
        return lor;
    }
};

// This pass will swap the simple expression to the front of an logical
// expression. Note that even for a subexpression of a logical expression, we
// will swap it as well. For example, a && ((b && c) || d), will become
// a && (d || (b && c))
class SwapSimpleExpressionToFrontOfLogicalExpression : public Transform {
    bool is_simple(const IR::Node *n) {
        if (n->is<IR::Equ>() || n->is<IR::Neq>() || n->is<IR::Lss>() ||
            n->is<IR::Grt>() || n->is<IR::Geq>() || n->is<IR::Leq>() ||
            n->is<IR::MethodCallExpression>() || n->is<IR::PathExpression>() ||
            n->is<IR::Member>()) {
            return true;
        } else if (!n->is<IR::LAnd>() && !n->is<IR::LOr>()) {
            BUG("Logical Expression Unroll pass failed");
        } else {
            return false;
        }
    }

 public:
    const IR::Node *postorder(IR::LAnd *land) {
        if (!is_simple(land->left) && is_simple(land->right)) {
            return new IR::LAnd(land->right, land->left);
        } else if (!is_simple(land->left)) {
            if (auto land2 = land->right->to<IR::LAnd>()) {
                if (is_simple(land2->left)) {
                    auto sub = new IR::LAnd(land->left, land2->right);
                    return new IR::LAnd(land2->left, sub);
                }
            }
        }
        return land;
    }
    const IR::Node *postorder(IR::LOr *lor) {
        if (!is_simple(lor->left) && is_simple(lor->right)) {
            return new IR::LOr(lor->right, lor->left);
        } else if (!is_simple(lor->left)) {
            if (auto lor2 = lor->right->to<IR::LOr>()) {
                if (is_simple(lor2->left)) {
                    auto sub = new IR::LOr(lor->left, lor2->right);
                    return new IR::LOr(lor2->left, sub);
                }
            }
        }
        return lor;
    }
};

// This passmanager togethor transform logical expression into a form that
// the simple expression will go to the front of the expression. And for
// expression at the same level(the same level is that expressions that are
// connected directly by && or ||) should be traversed from left to right
// (a && b) && c is not a valid expression here.
class ConvertLogicalExpression : public PassManager {
 public:
    ConvertLogicalExpression() {
        auto r = new PassRepeated{new BreakLogicalExpressionParenthesis};
        passes.push_back(r);
        r = new PassRepeated{
            new SwapSimpleExpressionToFrontOfLogicalExpression};
        passes.push_back(r);
    }
};

// This Pass collects infomation about the table keys for each table. This information
// is later used for generating the context JSON output for use by the control plane
// software.
class CollectTableInfo : public Inspector {
    DpdkProgramStructure *structure;

 public:
    explicit CollectTableInfo(DpdkProgramStructure *structure)
        : structure(structure) {setName("CollectTableInfo");}
    bool preorder(const IR::Key *key) override;
};

// This pass transforms the tables such that all the Match keys are part of the same
// header/metadata struct. If the match keys are from different headers, this pass creates
// mirror copies of the struct field into the metadata struct and updates the table to use
// the metadata copy.
// This pass must be called right before CollectLocalVariables pass as the temporary
// variables created for holding copy of the table keys are inserted to Metadata by
// CollectLocalVariables pass.
class CopyMatchKeysToSingleStruct : public P4::KeySideEffect {
    IR::IndexedVector<IR::Declaration> decls;
    DpdkProgramStructure *structure;
 public:
    CopyMatchKeysToSingleStruct(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
             std::set<const IR::P4Table*>* invokedInKey, DpdkProgramStructure *structure)
             : P4::KeySideEffect(refMap, typeMap, invokedInKey), structure(structure)
    { setName("CopyMatchKeysToSingleStruct"); }

    const IR::Node* preorder(IR::Key* key) override;
    const IR::Node* postorder(IR::KeyElement* element) override;
    const IR::Node* doStatement(const IR::Statement* statement,
                                const IR::Expression* expression) override;
};

/**
 * Common code between SplitActionSelectorTable and SplitActionProfileTable
 */
class SplitP4TableCommon : public Transform {
 public:
    enum class TableImplementation { DEFAULT, ACTION_PROFILE, ACTION_SELECTOR };
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    DpdkProgramStructure *structure;
    TableImplementation implementation;
    std::set<cstring> match_tables;
    std::map<cstring, cstring> group_tables;
    std::map<cstring, cstring> member_tables;
    std::map<cstring, cstring> member_ids;
    std::map<cstring, cstring> group_ids;

    SplitP4TableCommon(P4::ReferenceMap *refMap, P4::TypeMap* typeMap,
        DpdkProgramStructure *structure) : refMap(refMap), typeMap(typeMap), structure(structure) {
        implementation = TableImplementation::DEFAULT;
    }

    const IR::Node* postorder(IR::MethodCallStatement*) override;
    const IR::Node* postorder(IR::IfStatement*) override;
    const IR::Node* postorder(IR::SwitchStatement*) override;

    std::tuple<const IR::P4Table*, cstring> create_match_table(const IR::P4Table* /* tbl */);
    const IR::P4Action* create_action(cstring /* actionName */, cstring /* id */, cstring);
    const IR::P4Table* create_member_table(const IR::P4Table*, cstring, cstring);
    const IR::P4Table* create_group_table(const IR::P4Table*, cstring, cstring, cstring, int, int);
};

/**
 * Split ActionSelector into three tables:
 *   match table that matches on exact/ternary key and generates a group id
 *   group table that matches on group id and generates a member id
 *   member table that runs an action based on member id.
 */
class SplitActionSelectorTable : public SplitP4TableCommon {
 public:
    SplitActionSelectorTable(P4::ReferenceMap *refMap, P4::TypeMap* typeMap,
        DpdkProgramStructure *structure) : SplitP4TableCommon(refMap, typeMap, structure) {
        implementation = TableImplementation::ACTION_SELECTOR; }
    const IR::Node* postorder(IR::P4Table* tbl) override;
};

/**
 * Split ActionProfile into two tables:
 *   match table that matches on exact/ternary key and generates a member id
 *   member table that runs an action based on member id.
 */
class SplitActionProfileTable : public SplitP4TableCommon {
 public:
    SplitActionProfileTable(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
        DpdkProgramStructure *structure) : SplitP4TableCommon(refMap, typeMap, structure) {
        implementation = TableImplementation::ACTION_PROFILE; }
    const IR::Node* postorder(IR::P4Table* tbl) override;
};

/**
 * Handle ActionSelector and ActionProfile extern in PSA
 */
class ConvertActionSelectorAndProfile : public PassManager {
    DpdkProgramStructure *structure;
 public:
    ConvertActionSelectorAndProfile(P4::ReferenceMap *refMap, P4::TypeMap* typeMap,
                                    DpdkProgramStructure *structure) {
        passes.emplace_back(new P4::TypeChecking(refMap, typeMap));
        passes.emplace_back(new SplitActionSelectorTable(refMap, typeMap, structure));
        passes.push_back(new P4::ClearTypeMap(typeMap));
        passes.emplace_back(new P4::TypeChecking(refMap, typeMap, true));
        passes.emplace_back(new SplitActionProfileTable(refMap, typeMap, structure));
        passes.push_back(new P4::ClearTypeMap(typeMap));
        passes.emplace_back(new P4::TypeChecking(refMap, typeMap, true));
    }
};

class CollectAddOnMissTable : public Inspector {
    P4::ReferenceMap* refMap;
    P4::TypeMap* typeMap;
    DpdkProgramStructure* structure;

 public:
    CollectAddOnMissTable(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
            DpdkProgramStructure* structure) :
    refMap(refMap), typeMap(typeMap), structure(structure) {}

    void postorder(const IR::P4Table* t) override;
    void postorder(const IR::MethodCallStatement*) override;
};

class CollectErrors : public Inspector {
    DpdkProgramStructure *structure;

 public:
    explicit CollectErrors(DpdkProgramStructure* structure) :
    structure(structure) { CHECK_NULL(structure); }
    void postorder(const IR::Type_Error* error) override {
        int id = 0;
        for (auto err : error->members) {
            if (structure->error_map.count(err->name.name) == 0) {
                structure->error_map.emplace(err->name.name, id++);
            }
        }
    }
};

class DpdkArchFirst : public PassManager {
 public:
    DpdkArchFirst() { setName("DpdkArchFirst"); }
};

class DpdkArchLast : public PassManager {
 public:
    DpdkArchLast() { setName("DpdkArchLast"); }
};

class CollectProgramStructure : public PassManager {
 public:
    CollectProgramStructure(P4::ReferenceMap * refMap, P4::TypeMap* typeMap,
                            DpdkProgramStructure* structure) {
        auto *evaluator = new P4::EvaluatorPass(refMap, typeMap);
        auto *parseDpdk = new ParseDpdkArchitecture(structure);
        passes.push_back(evaluator);
        passes.push_back(new VisitFunctor([evaluator, parseDpdk]() {
            auto toplevel = evaluator->getToplevelBlock();
            auto main = toplevel->getMain();
            if (main == nullptr) {
                ::error(ErrorType::ERR_NOT_FOUND,
                    "Could not locate top-level block; is there a %1% module?",
                    IR::P4Program::main);
                return; }
            main->apply(*parseDpdk);
        }));
     }
};

};  // namespace DPDK
#endif  /* BACKENDS_DPDK_DPDKARCH_H_ */
