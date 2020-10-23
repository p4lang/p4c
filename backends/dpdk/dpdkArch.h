#ifndef BACKENDS_CONVERT_TO_DPDK_ARCH_H_
#define BACKENDS_CONVERT_TO_DPDK_ARCH_H_
#include "dpdkVarCollector.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeMap.h"
#include <ir/ir.h>
namespace DPDK {

cstring TypeStruct2Name(const cstring *s);
bool isSimpleExpression(const IR::Expression *e);
bool isNonConstantSimpleExpression(const IR::Expression *e);

enum gress_t {
  INGRESS = 0,
  EGRESS = 1,
};

enum block_t {
  PARSER = 0,
  PIPELINE,
  DEPARSER,
};

struct BlockInfo {
  cstring pipe;
  gress_t gress;
  block_t block;

  BlockInfo(cstring p, gress_t g, block_t b) : pipe(p), gress(g), block(b) {}

  void dbprint(std::ostream &out) {
    out << "pipe" << pipe << " ";
    out << "gress " << gress << " ";
    out << "block" << block << std::endl;
  }
};

using BlockInfoMapping = std::map<const IR::Node *, BlockInfo>;
using UserMeta = std::set<cstring>;

class CollectMetadataHeaderInfo;

/* According to the implementation of DPDK backend, for a control block, there
 * are only two parameters: header and metadata. Therefore, first we need to
 * rewrite the declaration of PSA architecture included in psa.p4 in order to
 * pass the type checking. In addition, this pass changes the definition of
 * P4Control and P4Parser(parameter list) in the P4 program provided by the
 * user.
 */
class ConvertToDpdkArch : public Transform {
  BlockInfoMapping *block_info;
  P4::ReferenceMap *refMap;
  CollectMetadataHeaderInfo *info;

  const IR::Type_Control *rewriteControlType(const IR::Type_Control *, cstring);
  const IR::Type_Parser *rewriteParserType(const IR::Type_Parser *, cstring);
  const IR::Node *postorder(IR::P4Program *prog) override;
  const IR::Node *postorder(IR::Type_Control *c) override;
  const IR::Node *postorder(IR::Type_Parser *p) override;
  const IR::Node *postorder(IR::P4Control *c) override;
  const IR::Node *postorder(IR::P4Parser *c) override;

public:
  ConvertToDpdkArch(BlockInfoMapping *b, P4::ReferenceMap *refMap,
                    CollectMetadataHeaderInfo *info)
      : block_info(b), refMap(refMap), info(info) {}
};

/* This Pass collects information about the name of Ingress, IngressParser,
 * IngressDeparser, Egress, EgressParser and EgressDeparser.
 */
class ParsePsa : public Inspector {
  const IR::PackageBlock *mainBlock;

public:
  ParsePsa() {}

  void parseIngressPipeline(const IR::PackageBlock *block);
  void parseEgressPipeline(const IR::PackageBlock *block);
  bool preorder(const IR::PackageBlock *block) override;

public:
  BlockInfoMapping toBlockInfo;
  UserMeta userMeta;
};

// This Pass collects infomation about the name of all metadata and header
// And it collects every field of metadata and renames all fields with a prefix
// according to the metadata struct name. Eventually, the reference of a fields
// will become m.$(struct_name)_$(field_name).
class CollectMetadataHeaderInfo : public Inspector {
  BlockInfoMapping *toBlockInfo;
  IR::Vector<IR::Type> used_metadata;
  void pushMetadata(const IR::Parameter *p);

public:
  CollectMetadataHeaderInfo(BlockInfoMapping *toBlockInfo)
      : toBlockInfo(toBlockInfo) {}
  bool preorder(const IR::P4Program *p) override;
  bool preorder(const IR::Type_Struct *s) override;
  cstring local_metadata_type;
  cstring header_type;
  IR::IndexedVector<IR::StructField> fields;
};

// This pass modifies all metadata references and header reference. For
// metadata, struct_name.field_name -> m.struct_name_field_name. For header
// headers.header_name.field_name -> h.header_name.field_name
class ReplaceMetadataHeaderName : public Transform {
  P4::ReferenceMap *refMap;
  CollectMetadataHeaderInfo *info;

public:
  ReplaceMetadataHeaderName(P4::ReferenceMap *refMap,
                            CollectMetadataHeaderInfo *info)
      : refMap(refMap), info(info) {}
  const IR::Node *preorder(IR::Member *m) override;
  const IR::Node *preorder(IR::Parameter *p) override;
};

// Previously, we have collected the information about how the single metadata
// struct looks like in CollectMetadataHeaderInfo. This pass finds a suitable
// place to inject this struct.
class InjectJumboStruct : public Transform {
  CollectMetadataHeaderInfo *info;
  int cnt = 0;

public:
  InjectJumboStruct(CollectMetadataHeaderInfo *info) : info(info) {}
  const IR::Node *preorder(IR::Type_Struct *s) override;
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
    IR::IndexedVector<IR::Declaration> *decls;
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
  DpdkVariableCollector *collector;
  DeclarationInjector injector;

public:
  StatementUnroll(DpdkVariableCollector *collector) : collector(collector) {}
  const IR::Node *preorder(IR::AssignmentStatement *a) override;
  const IR::Node *postorder(IR::P4Control *a) override;
  const IR::Node *postorder(IR::P4Parser *a) override;
  const IR::Node *preorder(IR::MethodCallStatement *a) override;
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
  DpdkVariableCollector *collector;

public:
  IR::IndexedVector<IR::StatOrDecl> stmt;
  IR::IndexedVector<IR::Declaration> decl;
  IR::PathExpression *root;
  // This function is a sanity to check whether the component of a Expression
  // falls into following classes, if not, it means we haven't implemented a
  // handle for that class.
  static void sanity(const IR::Expression *e) {
    if (not e->is<IR::Operation_Unary>() and
        not e->is<IR::MethodCallExpression>() and not e->is<IR::Member>() and
        not e->is<IR::PathExpression>() and
        not e->is<IR::Operation_Binary>() and not e->is<IR::Constant>() and
        not e->is<IR::BoolLiteral>()) {
      std::cerr << e->node_type_name() << std::endl;
      BUG("Untraversed node");
    }
  }
  ExpressionUnroll(DpdkVariableCollector *collector) : collector(collector) {
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
  DpdkVariableCollector *collector;
  DeclarationInjector injector;
  P4::ReferenceMap *refMap;
  P4::TypeMap *typeMap;

public:
  IfStatementUnroll(DpdkVariableCollector *collector, P4::ReferenceMap *refMap,
                    P4::TypeMap *typeMap)
      : collector(collector), refMap(refMap), typeMap(typeMap) {
    setName("IfStatementUnroll");
  }
  const IR::Node *postorder(IR::IfStatement *a) override;
  const IR::Node *postorder(IR::P4Control *a) override;
  const IR::Node *postorder(IR::P4Parser *a) override;
};

/* Assume one logical expression looks like this: a && (b + c > d), this pass
 * will unroll the expression to {tmp = b + c; if(a && (tmp > d))}. Logical
 * calculation will be unroll in a dedicated pass.
 */
class LogicalExpressionUnroll : public Inspector {
  DpdkVariableCollector *collector;
  P4::ReferenceMap *refMap;
  P4::TypeMap *typeMap;

public:
  IR::IndexedVector<IR::StatOrDecl> stmt;
  IR::IndexedVector<IR::Declaration> decl;
  IR::Expression *root;
  static void sanity(const IR::Expression *e) {
    if (not e->is<IR::Operation_Unary>() and
        not e->is<IR::MethodCallExpression>() and not e->is<IR::Member>() and
        not e->is<IR::PathExpression>() and
        not e->is<IR::Operation_Binary>() and not e->is<IR::Constant>() and
        not e->is<IR::BoolLiteral>()) {
      std::cerr << e->node_type_name() << std::endl;
      BUG("Untraversed node");
    }
  }
  static bool is_logical(const IR::Operation_Binary *bin) {
    if (bin->is<IR::LAnd>() or bin->is<IR::LOr>() or bin->is<IR::Leq>() or
        bin->is<IR::Equ>() or bin->is<IR::Neq>() or bin->is<IR::Grt>() or
        bin->is<IR::Lss>())
      return true;
    else if (bin->is<IR::Geq>() or bin->is<IR::Leq>()) {
      std::cerr << bin->node_type_name() << std::endl;
      BUG("does not implemented");
    } else
      return false;
  }

  LogicalExpressionUnroll(DpdkVariableCollector *collector,
                          P4::ReferenceMap *refMap, P4::TypeMap *typeMap)
      : collector(collector), refMap(refMap), typeMap(typeMap) {}
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
  DpdkVariableCollector *collector;
  DeclarationInjector injector;

public:
  ConvertBinaryOperationTo2Params(DpdkVariableCollector *collector)
      : collector(collector) {}
  const IR::Node *postorder(IR::AssignmentStatement *a) override;
  const IR::Node *postorder(IR::P4Control *a) override;
  const IR::Node *postorder(IR::P4Parser *a) override;
};
// Since in dpdk asm, there is no local variable declaraion, we need to collect
// all local variables and inject them into the metadata struct.
class CollectLocalVariableToMetadata : public Transform {
  BlockInfoMapping *toBlockInfo;
  CollectMetadataHeaderInfo *info;
  std::map<const cstring, IR::IndexedVector<IR::Declaration>> locals_map;
  P4::ReferenceMap *refMap;

public:
  CollectLocalVariableToMetadata(BlockInfoMapping *toBlockInfo,
                                 CollectMetadataHeaderInfo *info,
                                 P4::ReferenceMap *refMap)
      : toBlockInfo(toBlockInfo), info(info), refMap(refMap) {}
  const IR::Node *preorder(IR::P4Program *p) override;
  const IR::Node *postorder(IR::Type_Struct *s) override;
  const IR::Node *postorder(IR::PathExpression *path) override;
  const IR::Node *postorder(IR::P4Control *c) override;
  const IR::Node *postorder(IR::P4Parser *p) override;
};
class RewriteToDpdkArch : public PassManager {
public:
  CollectMetadataHeaderInfo *info;
  RewriteToDpdkArch(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                    DpdkVariableCollector *collector) {
    setName("RewriteToDpdkArch");
    auto *evaluator = new P4::EvaluatorPass(refMap, typeMap);
    auto *parsePsa = new ParsePsa();
    info = new CollectMetadataHeaderInfo(&parsePsa->toBlockInfo);
    passes.push_back(evaluator);
    passes.push_back(new VisitFunctor([evaluator, parsePsa]() {
      auto toplevel = evaluator->getToplevelBlock();
      auto main = toplevel->getMain();
      ERROR_CHECK(main != nullptr, ErrorType::ERR_INVALID,
                  "program: does not instantiate `main`");
      main->apply(*parsePsa);
    }));
    passes.push_back(info);
    passes.push_back(
        new ConvertToDpdkArch(&parsePsa->toBlockInfo, refMap, info));
    passes.push_back(new ReplaceMetadataHeaderName(refMap, info));
    passes.push_back(new InjectJumboStruct(info));
    passes.push_back(new StatementUnroll(collector));
    passes.push_back(new IfStatementUnroll(collector, refMap, typeMap));
    passes.push_back(new P4::ClearTypeMap(typeMap));
    passes.push_back(new P4::TypeChecking(refMap, typeMap, true));
    passes.push_back(new ConvertBinaryOperationTo2Params(collector));
    parsePsa = new ParsePsa();
    passes.push_back(evaluator);
    passes.push_back(new VisitFunctor([evaluator, parsePsa]() {
      auto toplevel = evaluator->getToplevelBlock();
      auto main = toplevel->getMain();
      ERROR_CHECK(main != nullptr, ErrorType::ERR_INVALID,
                  "program: does not instantiate `main`");
      main->apply(*parsePsa);
    }));
    passes.push_back(new CollectLocalVariableToMetadata(&parsePsa->toBlockInfo,
                                                        info, refMap));
  }
};

}; // namespace DPDK
#endif
