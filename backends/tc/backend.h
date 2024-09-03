/*
Copyright (C) 2023 Intel Corporation

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing,
software distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions
and limitations under the License.
*/

#ifndef BACKENDS_TC_BACKEND_H_
#define BACKENDS_TC_BACKEND_H_

#include <deque>

#include "backends/ebpf/psa/ebpfPsaGen.h"
#include "control-plane/p4RuntimeArchHandler.h"
#include "ebpfCodeGen.h"
#include "frontends/p4/evaluator/evaluator.h"
#include "frontends/p4/parseAnnotations.h"
#include "frontends/p4/parserCallGraph.h"
#include "introspection.h"
#include "ir/ir.h"
#include "lib/error.h"
#include "lib/nullstream.h"
#include "lib/stringify.h"
#include "options.h"
#include "pnaProgramStructure.h"
#include "tcAnnotations.h"
#include "tc_defines.h"

namespace P4::TC {

extern cstring PnaMainParserInputMetaFields[TC::MAX_PNA_PARSER_META];
extern cstring PnaMainInputMetaFields[TC::MAX_PNA_INPUT_META];
extern cstring PnaMainOutputMetaFields[TC::MAX_PNA_OUTPUT_META];

class PNAEbpfGenerator;

/**
 * Backend code generation from midend IR
 */
class ConvertToBackendIR : public Inspector {
 public:
    struct ExternInstance {
        cstring instance_name;
        unsigned instance_id;
        bool is_num_elements;
        int num_elements;
    };
    struct ExternBlock {
        cstring externId;
        cstring control_name;
        unsigned no_of_instances;
        cstring permissions;
        safe_vector<struct ExternInstance *> eInstance;
    };
    enum CounterType { PACKETS, BYTES, PACKETS_AND_BYTES };
    const IR::ToplevelBlock *tlb;
    IR::TCPipeline *tcPipeline;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    TCOptions &options;
    unsigned int tableCount = 0;
    unsigned int actionCount = 0;
    unsigned int metadataCount = 0;
    unsigned int labelCount = 0;
    unsigned int externCount = 0;
    cstring pipelineName = nullptr;
    cstring mainParserName = nullptr;
    ordered_map<cstring, const IR::P4Action *> actions;
    ordered_map<unsigned, cstring> tableIDList;
    ordered_map<unsigned, cstring> actionIDList;
    ordered_map<unsigned, unsigned> tableKeysizeList;
    safe_vector<const IR::P4Table *> add_on_miss_tables;
    ordered_map<cstring, std::pair<cstring, cstring> *> tablePermissions;
    ordered_map<cstring, const IR::Type_Struct *> ControlStructPerExtern;
    ordered_map<cstring, struct ExternBlock *> externsInfo;

 public:
    ConvertToBackendIR(const IR::ToplevelBlock *tlb, IR::TCPipeline *pipe, P4::ReferenceMap *refMap,
                       P4::TypeMap *typeMap, TCOptions &options)
        : tlb(tlb), tcPipeline(pipe), refMap(refMap), typeMap(typeMap), options(options) {}
    void setPipelineName();
    cstring getPipelineName() { return pipelineName; };
    bool preorder(const IR::P4Program *p) override;
    void postorder(const IR::P4Action *a) override;
    void postorder(const IR::P4Table *t) override;
    void postorder(const IR::P4Program *p) override;
    void postorder(const IR::Declaration_Instance *d) override;
    void postorder(const IR::Type_Struct *ts) override;
    safe_vector<const IR::TCKey *> processExternConstructor(const IR::Type_Extern *extn,
                                                            const IR::Declaration_Instance *decl,
                                                            struct ExternInstance *instance);
    safe_vector<const IR::TCKey *> processExternControlPath(const IR::Type_Extern *extn,
                                                            const IR::Declaration_Instance *decl,
                                                            cstring eName);
    cstring getControlPathKeyAnnotation(const IR::StructField *field);
    unsigned GetAccessNumericValue(std::string_view access);
    bool isDuplicateAction(const IR::P4Action *action);
    bool isDuplicateOrNoAction(const IR::P4Action *action);
    void updateDefaultHitAction(const IR::P4Table *t, IR::TCTable *tdef);
    void updateDefaultMissAction(const IR::P4Table *t, IR::TCTable *tdef);
    void updateConstEntries(const IR::P4Table *t, IR::TCTable *tdef);
    void updateMatchType(const IR::P4Table *t, IR::TCTable *tabledef);
    void updateTimerProfiles(IR::TCTable *tabledef);
    void updatePnaDirectCounter(const IR::P4Table *t, IR::TCTable *tabledef, unsigned tentries);
    void updatePnaDirectMeter(const IR::P4Table *t, IR::TCTable *tabledef, unsigned tentries);
    bool isPnaParserMeta(const IR::Member *mem);
    bool isPnaMainInputMeta(const IR::Member *mem);
    bool isPnaMainOutputMeta(const IR::Member *mem);
    unsigned int findMappedKernelMeta(const IR::Member *mem);
    const IR::Expression *ExtractExpFromCast(const IR::Expression *exp);
    unsigned getTcType(const IR::StringLiteral *sl);
    unsigned getTableId(cstring tableName) const;
    unsigned getActionId(cstring actionName) const;
    cstring getExternId(cstring externName) const;
    unsigned getExternInstanceId(cstring externName, cstring instanceName) const;
    cstring processExternPermission(const IR::Type_Extern *ext);
    unsigned getTableKeysize(unsigned tableId) const;
    cstring externalName(const IR::IDeclaration *declaration) const;
    cstring HandleTableAccessPermission(const IR::P4Table *t);
    std::pair<cstring, cstring> *GetAnnotatedAccessPath(const IR::Annotation *anno);
    void updateAddOnMissTable(const IR::P4Table *t);
    bool checkParameterDirection(const IR::TCAction *tcAction);
    bool hasExecuteMethod(const IR::Type_Extern *extn);
    void addExternTypeInstance(const IR::Declaration_Instance *decl,
                               IR::TCExternInstance *tcExternInstance, cstring eName);
    safe_vector<const IR::TCKey *> HandleTypeNameStructField(const IR::StructField *field,
                                                             const IR::Type_Extern *extn,
                                                             const IR::Declaration_Instance *decl,
                                                             int &kId, cstring annoName);
    safe_vector<const IR::TCKey *> processCounterControlPathKeys(
        const IR::Type_Struct *extern_control_path, const IR::Type_Extern *extn,
        const IR::Declaration_Instance *decl);
    CounterType toCounterType(const int type);
};

class Extern {
 public:
    static const cstring dropPacket;
    static const cstring sendToPort;
};

class Backend : public PassManager {
 public:
    const IR::ToplevelBlock *toplevel;
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    TCOptions &options;
    IR::TCPipeline *pipeline = new IR::TCPipeline();
    TC::ConvertToBackendIR *tcIR;
    TC::IntrospectionGenerator *genIJ;
    TC::ParseTCAnnotations *parseTCAnno;
    const IR::ToplevelBlock *top = nullptr;
    EbpfOptions ebpfOption;
    EBPF::Target *target;
    const PNAEbpfGenerator *ebpf_program;

 public:
    explicit Backend(const IR::ToplevelBlock *toplevel, P4::ReferenceMap *refMap,
                     P4::TypeMap *typeMap, TCOptions &options)
        : toplevel(toplevel), refMap(refMap), typeMap(typeMap), options(options) {
        setName("BackEnd");
    }
    bool process();
    bool ebpfCodeGen(P4::ReferenceMap *refMap, P4::TypeMap *typeMap);
    void serialize() const;
    bool serializeIntrospectionJson(std::ostream &out) const;
    bool emitCFile();
};

}  // namespace P4::TC

#endif /* BACKENDS_TC_BACKEND_H_ */
