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

#ifndef _BACKENDS_BMV2_JSONCONVERTER_H_
#define _BACKENDS_BMV2_JSONCONVERTER_H_

#include "lib/json.h"
#include "frontends/common/options.h"
#include "frontends/p4/fromv1.0/v1model.h"
#include "midend/convertEnums.h"
#include "analyzer.h"
// Currently we are requiring a v1model to be used

// This is based on the specification of the BMv2 JSON input format
// https://github.com/p4lang/behavioral-model/blob/master/docs/JSON_format.md

namespace BMV2 {

class ExpressionConverter;

class DirectMeterMap final {
 public:
    struct DirectMeterInfo {
        const IR::Expression* destinationField;
        const IR::P4Table* table;
        unsigned tableSize;

        DirectMeterInfo() : destinationField(nullptr), table(nullptr), tableSize(0) {}
    };

 private:
    // key is declaration of direct meter
    std::map<const IR::IDeclaration*, DirectMeterInfo*> directMeter;
    DirectMeterInfo* createInfo(const IR::IDeclaration* meter);
 public:
    DirectMeterInfo* getInfo(const IR::IDeclaration* meter);
    void setDestination(const IR::IDeclaration* meter, const IR::Expression* destination);
    void setTable(const IR::IDeclaration* meter, const IR::P4Table* table);
    void setSize(const IR::IDeclaration* meter, unsigned size);
};

class JsonConverter final {
 public:
    const CompilerOptions& options;
    Util::JsonObject       toplevel;  // output is constructed here
    P4V1::V1Model&         v1model;
    P4::P4CoreLibrary&     corelib;
    P4::ReferenceMap*      refMap;
    P4::TypeMap*           typeMap;
    ProgramParts           structure;
    cstring                scalarsName;  // name of struct in JSON holding all scalars
    const IR::ToplevelBlock* toplevelBlock;
    ExpressionConverter*   conv;
    DirectMeterMap         meterMap;
    std::map<cstring, const IR::P4Table*> directCountersMap;  // map counter name to table
    const IR::Parameter*   headerParameter;
    const IR::Parameter*   userMetadataParameter;
    const IR::Parameter*   stdMetadataParameter;
    cstring                jsonMetadataParameterName = "standard_metadata";
    const unsigned         boolWidth = 1;
    // We place scalar user metadata fields (i.e., bit<>, bool)
    // in the "scalars" metadata object, so we may need to rename
    // these fields.  This map holds the new names.
    std::map<const IR::StructField*, cstring> scalarMetadataFields;
    // we map error codes to numerical values for bmv2
    using ErrorValue = unsigned int;
    using ErrorCodesMap = std::unordered_map<const IR::IDeclaration *, ErrorValue>;
    ErrorCodesMap errorCodesMap{};
    P4::ConvertEnums::EnumMapping* enumMap;

 private:
    Util::JsonArray *headerTypes;
    std::map<cstring, cstring> headerTypesCreated;
    Util::JsonArray *headerInstances;
    Util::JsonArray *headerStacks;
    Util::JsonObject *scalarsStruct;
    unsigned scalars_width = 0;
    friend class ExpressionConverter;

 protected:
    void pushFields(cstring prefix, const IR::Type_StructLike *st, Util::JsonArray *fields);
    cstring createJsonType(const IR::Type_StructLike *type);
    unsigned nextId(cstring group);
    void addHeaderStacks(const IR::Type_Struct* headersStruct);
    void addLocals();
    void padScalars();
    void addTypesAndInstances(const IR::Type_StructLike* type, bool meta);
    void convertActionBody(const IR::Vector<IR::StatOrDecl>* body,
                           Util::JsonArray* result, Util::JsonArray* fieldLists,
                           Util::JsonArray* calculations, Util::JsonArray* learn_lists);
    Util::IJson* convertTable(const CFG::TableNode* node,
                              Util::JsonArray* counters,
                              Util::JsonArray* action_profiles);
    Util::IJson* convertIf(const CFG::IfNode* node, cstring parent);
    Util::JsonArray* createActions(Util::JsonArray* fieldLists, Util::JsonArray* calculations,
                                   Util::JsonArray* learn_lists);
    Util::IJson* toJson(const IR::P4Parser* cont);
    Util::IJson* toJson(const IR::ParserState* state);
    void convertDeparserBody(const IR::Vector<IR::StatOrDecl>* body, Util::JsonArray* result);
    Util::IJson* convertDeparser(const IR::P4Control* state);
    Util::IJson* convertParserStatement(const IR::StatOrDecl* stat);
    Util::IJson* convertControl(const IR::ControlBlock* block, cstring name,
                                Util::JsonArray* counters, Util::JsonArray* meters,
                                Util::JsonArray* registers);
    cstring createCalculation(cstring algo, const IR::Expression* fields,
                              Util::JsonArray* calculations,
                              const IR::Node* node);
    Util::IJson* nodeName(const CFG::Node* node) const;
    void createForceArith(const IR::Type* stdMetaType, cstring name,
                          Util::JsonArray* force_list) const;
    cstring convertHashAlgorithm(cstring algorithm) const;
    // Return 'true' if the table is 'simple'
    bool handleTableImplementation(const IR::Property* implementation,
                                   const IR::Key* key,
                                   Util::JsonObject* table,
                                   Util::JsonArray* action_profiles);
    void convertTableEntries(const IR::P4Table *table, Util::JsonObject *jsonTable);
    cstring getKeyMatchType(const IR::KeyElement *ke);
    void addToFieldList(const IR::Expression* expr, Util::JsonArray* fl);
    // returns id of created field list
    int createFieldList(const IR::Expression* expr, cstring group,
                        cstring listName, Util::JsonArray* fieldLists);
    void generateUpdate(const IR::BlockStatement *block,
                        Util::JsonArray* checksums, Util::JsonArray* calculations);
    void generateUpdate(const IR::P4Control* cont,
                        Util::JsonArray* checksums, Util::JsonArray* calculations);

    // Operates on a select keyset
    void convertSimpleKey(const IR::Expression* keySet,
                          mpz_class& value, mpz_class& mask) const;
    unsigned combine(const IR::Expression* keySet,
                     const IR::ListExpression* select,
                     mpz_class& value, mpz_class& mask) const;
    void buildCfg(IR::P4Control* cont);

    // Adds meta information (such as version) to the json
    void addMetaInformation();

    // Adds declared errors to json
    void addErrors();
    // Retrieve assigned numerical value for given error constant
    ErrorValue retrieveErrorValue(const IR::Member* mem) const;

    // Adds declared enums to json
    void addEnums();

 public:
    explicit JsonConverter(const CompilerOptions& options);
    void convert(P4::ReferenceMap* refMap, P4::TypeMap* typeMap, const IR::ToplevelBlock *toplevel,
                 P4::ConvertEnums::EnumMapping* enumMap);
    void serialize(std::ostream& out) const
    { toplevel.serialize(out); }
};

}  // namespace BMV2

#endif /* _BACKENDS_BMV2_JSONCONVERTER_H_ */
