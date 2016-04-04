#ifndef _BACKENDS_BMV2_JSONCONVERTER_H_
#define _BACKENDS_BMV2_JSONCONVERTER_H_

#include "lib/json.h"
#include "frontends/common/options.h"
#include "frontends/p4/evaluator/blockMap.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "analyzer.h"
// Currently we are requiring a v1model to be used

// This is based on the specification of the BMv2 JSON input format
// https://github.com/p4lang/behavioral-model/blob/master/docs/JSON_format.md

namespace BMV2 {

class ExpressionConverter;

class JsonConverter {
    friend class ExpressionConverter;

    const CompilerOptions& options;
    Util::JsonObject       toplevel;  // output is constructed here
    P4V1::V1Model&         v1model;
    P4::P4CoreLibrary&     corelib;
    P4::ReferenceMap*      refMap;
    P4::TypeMap*           typeMap;
    ProgramParts           structure;
    cstring                dropAction = ".drop";
    P4::BlockMap*          blockMap;
    ExpressionConverter*   conv;
    
    // Field written by a direct meter (indexed with meter declaration).
    std::map<const IR::IDeclaration*, const IR::Expression*> directMeterDest;
    // Table that uses a direct meter.
    std::map<const IR::IDeclaration*, const IR::TableContainer*> directMeterTable;

 protected:
    Util::IJson* typeToJson(const IR::Type_StructLike* type);
    unsigned nextId(cstring group);
    void addHeaderStacks(const IR::Type_Struct* headersStruct,
                         Util::JsonArray* headers, Util::JsonArray* headerTypes,
                         Util::JsonArray* stacks, std::set<cstring> &headerTypesCreated);
    void addTypesAndInstances(const IR::Type_StructLike* type, bool meta,
                              Util::JsonArray* headerTypes, Util::JsonArray* instances,
                              std::set<cstring> &headerTypesCreated);
    void convertActionBody(const IR::Vector<IR::StatOrDecl>* body,
                           Util::JsonArray* result, Util::JsonArray* fieldLists,
                           Util::JsonArray* calculations, Util::JsonArray* learn_lists);
    Util::IJson* convertTable(const CFG::TableNode* node, Util::JsonArray* counters);
    Util::IJson* convertIf(const CFG::IfNode* node, cstring parent);
    Util::JsonArray* createActions(Util::JsonArray* fieldLists, Util::JsonArray* calculations,
                                   Util::JsonArray* learn_lists);
    Util::IJson* toJson(const IR::ParserContainer* cont);
    Util::IJson* toJson(const IR::ParserState* state);
    void convertDeparserBody(const IR::Vector<IR::StatOrDecl>* body, Util::JsonArray* result);
    Util::IJson* convertDeparser(const IR::ControlContainer* state);
    Util::IJson* convertParserStatement(const IR::StatOrDecl* stat);
    Util::IJson* convertControl(const IR::ControlBlock* block, cstring name,
                                Util::JsonArray* counters, Util::JsonArray* meters,
                                Util::JsonArray* registers);
    cstring createCalculation(cstring algo, const IR::Expression* fields,
                              Util::JsonArray* calculations);
    Util::IJson* nodeName(const CFG::Node* node) const;
    void createForceArith(const IR::Type* stdMetaType, cstring name,
                          Util::JsonArray* force_list) const;
    void setDirectMeterDestination(const IR::IDeclaration* decl, const IR::Expression* dest);
    cstring convertHashAlgorithm(cstring algorithm) const;
    void handleTableImplementation(const IR::TableProperty* implementation,
                                   const IR::Key* key,
                                   Util::JsonObject* table);
    void addToFieldList(const IR::Expression* expr, Util::JsonArray* fl);
    // returns id of created field list
    int createFieldList(const IR::Expression* expr, cstring listName, Util::JsonArray* fieldLists);
    void generateUpdate(const IR::ControlContainer* cont,
                        Util::JsonArray* checksums, Util::JsonArray* calculations);

    // Operates on a select keyset
    void convertSimpleKey(const IR::Expression* keySet,
                          mpz_class& value, mpz_class& mask) const;
    unsigned combine(const IR::Expression* keySet,
                     const IR::ListExpression* select,
                     mpz_class& value, mpz_class& mask) const;
    void buildCfg(IR::ControlContainer* cont);

 public:
    explicit JsonConverter(const CompilerOptions& options);
    void convert(P4::BlockMap *blockMap);
    void serialize(std::ostream& out) const
    { toplevel.serialize(out); }
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_JSONCONVERTER_H_ */
