#include <ir/ir.h>
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/common/resolveReferences/resolveReferences.h"

namespace DPDK {

cstring TypeStruct2Name(const cstring *s);

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

    BlockInfo(cstring p, gress_t g, block_t b) :
        pipe(p), gress(g), block(b) {}

    void dbprint(std::ostream& out) {
        out << "pipe" << pipe << " ";
        out << "gress " << gress << " ";
        out << "block" << block << std::endl;
    }
};

using BlockInfoMapping = std::map<const IR::Node*, BlockInfo>;
using UserMeta = std::set<cstring>;

class CollectMetadataHeaderInfo;

class ConvertToDpdkArch : public Transform {
    BlockInfoMapping* block_info;
    P4::ReferenceMap* refMap;
    CollectMetadataHeaderInfo *info;

    const IR::Type_Control* rewriteControlType(const IR::Type_Control*, cstring);
    const IR::Type_Parser* rewriteParserType(const IR::Type_Parser*, cstring);
    const IR::Node* postorder(IR::P4Program* prog) override;
    const IR::Node* postorder(IR::Type_Control* c) override;
    const IR::Node* postorder(IR::Type_Parser* p) override;
    const IR::Node* postorder(IR::P4Control* c) override;
    const IR::Node* postorder(IR::P4Parser* c) override;
    const IR::Node* postorder(IR::PathExpression* p) override;
    const IR::Node* postorder(IR::Member* m) override;
    const IR::Node* postorder(IR::Type_StructLike* s) override;

 public:
    ConvertToDpdkArch(
        BlockInfoMapping* b, 
        P4::ReferenceMap* refMap, 
        CollectMetadataHeaderInfo* info) :
        block_info(b), 
        refMap(refMap),
        info(info) {}
};

class ParsePsa : public Inspector {
    const IR::PackageBlock*   mainBlock;
 public:
    ParsePsa() { }

    void parseIngressPipeline(const IR::PackageBlock* block);
    void parseEgressPipeline(const IR::PackageBlock* block);
    bool preorder(const IR::PackageBlock* block) override;
    // bool preorder(const IR::Type_Struct* s) override;


 public:
    BlockInfoMapping toBlockInfo;
    UserMeta userMeta;
};

class CollectMetadataHeaderInfo : public Inspector {
    BlockInfoMapping *toBlockInfo;
public:
    CollectMetadataHeaderInfo(BlockInfoMapping *toBlockInfo):toBlockInfo(toBlockInfo){}
    bool preorder(const IR::P4Program *p) override;
    bool preorder(const IR::Type_Struct *s) override;
    cstring local_metadata_type;
    cstring header_type;
    IR::IndexedVector<IR::StructField> fields;
};

class ReplaceMetadataHeaderName : public Transform {
    CollectMetadataHeaderInfo *info;
    P4::ReferenceMap *refMap;
public:
    ReplaceMetadataHeaderName(
        P4::ReferenceMap *refMap, 
        CollectMetadataHeaderInfo *info):
        refMap(refMap),
        info(info){}
    const IR::Node* postorder(IR::P4Program* p) override;
    const IR::Node *preorder(IR::Member* m) override;
    const IR::Node *preorder(IR::Parameter* p) override;

};

class InjectJumboStruct : public Transform {
    CollectMetadataHeaderInfo *info;
    int cnt = 0;
public:
    InjectJumboStruct(
        CollectMetadataHeaderInfo *info
    ): info(info){}
    const IR::Node *preorder(IR::Type_Struct* s) override;
    const IR::Node *postorder(IR::P4Program* p) override{std::cout << p << std::endl; return p;}
};

class RewriteToDpdkArch : public PassManager {
 public:
    RewriteToDpdkArch(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
        setName("RewriteToDpdkArch");
        auto* evaluator = new P4::EvaluatorPass(refMap, typeMap);
        auto* parsePsa = new ParsePsa();
        auto info = new CollectMetadataHeaderInfo(&parsePsa->toBlockInfo);
        passes.push_back(evaluator);
        passes.push_back(new VisitFunctor([evaluator, parsePsa]() {
            auto toplevel = evaluator->getToplevelBlock();
            auto main = toplevel->getMain();
            ERROR_CHECK(main != nullptr, ErrorType::ERR_INVALID,
                    "program: does not instantiate `main`");
            main->apply(*parsePsa);
            }));
        passes.push_back(info);
        passes.push_back(new ConvertToDpdkArch(&parsePsa->toBlockInfo, refMap, info));
        passes.push_back(new ReplaceMetadataHeaderName(refMap, info));
        passes.push_back(new InjectJumboStruct(info));
    }
};

}  // namespace DPDK
