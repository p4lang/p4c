#ifndef BACKENDS_CONVERT_TO_DPDK_ARCH_H_
#define BACKENDS_CONVERT_TO_DPDK_ARCH_H_ 
#include <ir/ir.h>
#include "frontends/p4/typeMap.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "DpdkVariableCollector.h"
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
    IR::Vector<IR::Type> used_metadata;
    void pushMetadata(const IR::Parameter* p);
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
};

class DeclarationInjector {
    std::map<const IR::Node*, IR::IndexedVector<IR::Declaration>*> decl_map;
public:
    void collect(const IR::P4Control * control, const IR::P4Parser *parser, const IR::Declaration* decl){
        IR::IndexedVector<IR::Declaration> *decls;
        if(parser) {
            auto res = decl_map.find(parser);
            if(res != decl_map.end()){
                decls = res->second;
            }
            else{
                decls = new IR::IndexedVector<IR::Declaration>;
                decl_map.emplace(parser, decls);
            }
        }
        else if(control) {
            auto res = decl_map.find(control);
            if(res != decl_map.end()){
                decls = res->second;
            }
            else{
                decls = new IR::IndexedVector<IR::Declaration>;
                decl_map.emplace(control, decls);            
            }
        }
        decls->push_back(decl);
    }
    IR::Node* inject_control(const IR::Node *orig, IR::P4Control *control){
        auto res = decl_map.find(orig);
        if(res == decl_map.end()){
            return control;
        }
        for(auto d: *(res->second))
            control->controlLocals.push_back(d);
        return control;
    }
    IR::Node* inject_parser(const IR::Node *orig, IR::P4Parser *parser){
        auto res = decl_map.find(orig);
        if(res == decl_map.end()){
            return parser;
        }
        for(auto d: *(res->second))
            parser->parserLocals.push_back(d);
        return parser;
    }
};

class StatementUnroll: public Transform {
private:
    DpdkVariableCollector *collector;
    DeclarationInjector injector;
public:
    StatementUnroll(DpdkVariableCollector* collector):collector(collector){}
    const IR::Node *preorder(IR::AssignmentStatement *a) override;
    const IR::Node *postorder(IR::IfStatement *a) override;
    const IR::Node *postorder(IR::P4Control *a) override;
    const IR::Node *postorder(IR::P4Parser *a) override;
    const IR::Node *preorder(IR::MethodCallStatement *a) override;
};

class ExpressionUnroll: public Inspector {
    DpdkVariableCollector *collector;
public:
    IR::IndexedVector<IR::StatOrDecl> stmt;
    IR::IndexedVector<IR::Declaration> decl;
    IR::PathExpression *root;
    static void sanity(const IR::Expression* e){
        if(not e->is<IR::Operation_Unary>() and
            not e->is<IR::MethodCallExpression>() and
            not e->is<IR::Member>() and
            not e->is<IR::PathExpression>() and
            not e->is<IR::Operation_Binary>() and
            not e->is<IR::Constant>() and
            not e->is<IR::BoolLiteral>()) {
                std::cerr << e->node_type_name() << std::endl;
                BUG("Untraversed node");
            }
    }
    ExpressionUnroll(DpdkVariableCollector* collector):collector(collector){}
    bool preorder(const IR::Operation_Unary *a) override;
    bool preorder(const IR::Operation_Binary *a) override;
    bool preorder(const IR::MethodCallExpression *a) override;
    bool preorder(const IR::Member *a) override;
    bool preorder(const IR::PathExpression *a) override;
    bool preorder(const IR::Constant *a) override;
    bool preorder(const IR::BoolLiteral *a) override;

};

class ConvertBinaryOperationTo2Params: public Transform {
    DpdkVariableCollector *collector;
    DeclarationInjector injector;
public:
    ConvertBinaryOperationTo2Params(DpdkVariableCollector *collector): collector(collector){}
    const IR::Node *postorder(IR::AssignmentStatement *a) override;
    const IR::Node *postorder(IR::P4Control *a) override;
    const IR::Node *postorder(IR::P4Parser *a) override;
};

class printP4: public Inspector {
    public:
    bool preorder(const IR::P4Program *p) override{std::cout << p << std::endl; return false;}
    
};

class CollectLocalVariableToMetadata: public Transform {
    BlockInfoMapping* toBlockInfo;
    CollectMetadataHeaderInfo *info;
    std::map<const cstring, IR::IndexedVector<IR::Declaration>> locals_map;
    P4::ReferenceMap *refMap;
public:
    CollectLocalVariableToMetadata(
        BlockInfoMapping *toBlockInfo, 
        CollectMetadataHeaderInfo *info,
        P4::ReferenceMap *refMap):
        toBlockInfo(toBlockInfo),
        info(info),
        refMap(refMap){}
    const IR::Node *preorder(IR::P4Program* p) override;
    const IR::Node *postorder(IR::Type_Struct* s) override;
    const IR::Node *postorder(IR::PathExpression* path) override;
    const IR::Node *postorder(IR::P4Control * c) override;
    const IR::Node *postorder(IR::P4Parser * p) override;

};

class RewriteToDpdkArch : public PassManager {
 public:
    CollectMetadataHeaderInfo *info;
    RewriteToDpdkArch(P4::ReferenceMap* refMap, P4::TypeMap* typeMap, DpdkVariableCollector *collector) {
        setName("RewriteToDpdkArch");
        auto* evaluator = new P4::EvaluatorPass(refMap, typeMap);
        auto* parsePsa = new ParsePsa();
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
        passes.push_back(new ConvertToDpdkArch(&parsePsa->toBlockInfo, refMap, info));
        passes.push_back(new ReplaceMetadataHeaderName(refMap, info));
        passes.push_back(new InjectJumboStruct(info));
        passes.push_back(new StatementUnroll(collector));
        passes.push_back(new P4::ClearTypeMap(typeMap));
        passes.push_back(new P4::TypeChecking(refMap, typeMap, true));
        // passes.push_back(new ConvertBinaryOperationTo2Params(collector));
        parsePsa = new ParsePsa();
        passes.push_back(evaluator);
        passes.push_back(new VisitFunctor([evaluator, parsePsa]() {
            auto toplevel = evaluator->getToplevelBlock();
            auto main = toplevel->getMain();
            ERROR_CHECK(main != nullptr, ErrorType::ERR_INVALID,
                    "program: does not instantiate `main`");
            main->apply(*parsePsa);
            }));
        passes.push_back(new CollectLocalVariableToMetadata(&parsePsa->toBlockInfo, info, refMap));
        // passes.push_back(new printP4());
    }
};

class PrependHDotToActionArgs: public Transform {
    P4::ReferenceMap *refMap;
public:
    PrependHDotToActionArgs(P4::ReferenceMap *refMap): refMap(refMap){}
    const IR::Node *postorder(IR::PathExpression *path){
        auto declaration = refMap->getDeclaration(path->path);
        if(auto action = findContext<IR::DpdkAction>()){
            if(auto p = declaration->to<IR::Parameter>()){    
                for(auto para: action->para){
                    if(para->equiv(*p)){
                        return new IR::Member(new IR::PathExpression(IR::ID("t")), path->path->name);
                    }
                }
            }
        }
        return path;
    }
};

};  // namespace DPDK
#endif