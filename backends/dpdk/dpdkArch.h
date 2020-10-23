#ifndef BACKENDS_CONVERT_TO_DPDK_ARCH_H_
#define BACKENDS_CONVERT_TO_DPDK_ARCH_H_
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/typeMap.h"
#include <ir/ir.h>
namespace DPDK {

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
  }
};

}; // namespace DPDK
#endif
