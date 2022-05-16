#ifndef BACKENDS_DPDK_DPDKPROGRAMSTRUCTURE_H_
#define BACKENDS_DPDK_DPDKPROGRAMSTRUCTURE_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"

/* Collect information related to P4 programs targeting dpdk */
struct DpdkProgramStructure {
    cstring p4arch;  // 'pna' or 'psa'
    ordered_map<cstring, const IR::Declaration_Variable*> scalars;
    unsigned                            scalars_width = 0;

    std::map<const IR::StructField *, cstring> scalarMetadataFields;
    ordered_map<cstring, const IR::Type_Header*> header_types;
    ordered_map<cstring, const IR::Type_Struct*> metadata_types;
    ordered_map<cstring, const IR::Type_HeaderUnion*> header_union_types;
    ordered_map<cstring, const IR::Declaration_Variable*> headers;
    ordered_map<cstring, const IR::Declaration_Variable*> metadata;
    ordered_map<cstring, const IR::Declaration_Variable*> header_stacks;
    ordered_map<cstring, const IR::Declaration_Variable*> header_unions;
    ordered_map<cstring, const IR::P4Action*> actions;

    // table and action info for learner tables
    ordered_set<cstring> learner_tables;
    ordered_set<cstring> learner_actions;
    ordered_map<cstring, std::vector<cstring>> learner_action_params;
    ordered_map<cstring, const IR::P4Table *> learner_action_table;

    IR::IndexedVector<IR::DpdkDeclaration>       variables;

    ordered_map<cstring, const IR::P4Parser*> parsers;
    ordered_map<const IR::P4Parser*, cstring> parsers_rev;
    ordered_map<cstring, const IR::P4Control*> deparsers;
    ordered_map<const IR::P4Control*, cstring> deparsers_rev;
    ordered_map<cstring, const IR::P4Control*> pipelines;
    ordered_map<const IR::P4Control*, cstring> pipelines_rev;

    std::map<const cstring, IR::IndexedVector<IR::Parameter> *> args_struct_map;
    std::map<const IR::Declaration_Instance *, cstring>         csum_map;
    std::map<cstring, int>                                      error_map;
    std::vector<const IR::Declaration_Instance *>               externDecls;
    std::map<cstring, std::vector<std::pair<cstring, cstring>>> key_map;
    std::map<cstring, const IR::P4Table *>                      group_tables;
    std::map<cstring, const IR::P4Table *>                      member_tables;

    std::set<cstring> pipeline_controls;
    std::set<cstring> non_pipeline_controls;

    IR::Type_Struct * metadataStruct;
    cstring local_metadata_type;
    cstring header_type;
    IR::IndexedVector<IR::StructField> compiler_added_fields;
    IR::IndexedVector<IR::StructField> key_fields;
    IR::Vector<IR::Type> used_metadata;
    ordered_map<cstring, std::vector<struct hdrFieldInfo>> hdrFieldInfoList;
    ordered_map<cstring, IR::ParameterList*> defActionParamList;

    void push_variable(const IR::DpdkDeclaration * d) {
        variables.push_back(d); }
    IR::IndexedVector<IR::DpdkDeclaration> &get_globals() {
        return variables; }

    bool hasVisited(const IR::Type_StructLike* st) {
        if (auto h = st->to<IR::Type_Header>())
            return header_types.count(h->getName());
        else if (auto s = st->to<IR::Type_Struct>())
            return metadata_types.count(s->getName());
        else if (auto u = st->to<IR::Type_HeaderUnion>())
            return header_union_types.count(u->getName());
        return false;
    }

    /**
     * @brief Predicate that states whether architecture is PSA or not.
     *
     * Architecture is known after pass ParseDpdkArchitecture
     *
     * @return true If architecture is PSA.
     * @return false If architecture is not PSA.
     */
    bool isPSA(void) {
        return (p4arch == "psa") ? true : false;
    }

    /**
     * @brief Predicate that states whether architecture is PNA or not.
     *
     * Architecture is known after pass ParseDpdkArchitecture
     *
     * @return true If architecture is PNA.
     * @return false If architecture is not PNA.
     */
    bool isPNA(void) {
        return (p4arch == "pna") ? true : false;
    }
};

struct hdrFieldInfo {
    cstring modifiedName;
    cstring headerStr;
    unsigned modifiedWidth;
    unsigned offset;
    unsigned lsb;
    unsigned msb;
    unsigned fieldWidth;
    hdrFieldInfo() {
        modifiedName = "";
        headerStr = "";
        modifiedWidth = 0;
        offset = 0;
        lsb = 0;
        msb = 0;
        fieldWidth = 0;
    }
};

class ParseDpdkArchitecture : public Inspector {
    DpdkProgramStructure *structure;

 public:
    explicit ParseDpdkArchitecture(DpdkProgramStructure *structure) :
        structure(structure) { CHECK_NULL(structure); }

    bool preorder(const IR::ToplevelBlock *block) override;
    bool preorder(const IR::PackageBlock *block) override;
    void parse_psa_block(const IR::PackageBlock*);
    void parse_pna_block(const IR::PackageBlock*);

    profile_t init_apply(const IR::Node* root) override {
        structure->variables.clear();
        structure->header_types.clear();
        structure->metadata_types.clear();
        structure->parsers.clear();
        structure->deparsers.clear();
        structure->pipelines.clear();
        structure->actions.clear();
        return Inspector::init_apply(root);
    }
};

class InspectDpdkProgram : public Inspector {
    P4::ReferenceMap     *refMap;
    P4::TypeMap          *typeMap;
    DpdkProgramStructure *structure;

 public:
    InspectDpdkProgram(P4::ReferenceMap* refMap, P4::TypeMap* typeMap,
            DpdkProgramStructure *structure) :
        refMap(refMap), typeMap(typeMap),
        structure(structure) { CHECK_NULL(structure); }

    bool isHeaders(const IR::Type_StructLike* st);
    void addTypesAndInstances(const IR::Type_StructLike* type, bool meta);
    void addHeaderType(const IR::Type_StructLike *st);
    void addHeaderInstance(const IR::Type_StructLike *st, cstring name);
    bool preorder(const IR::Declaration_Variable* dv) override;
    bool preorder(const IR::Parameter* parameter) override;
    bool preorder(const IR::P4Action*) override;
    bool isStandardMetadata(cstring);
};

#endif /* BACKENDS_DPDK_DPDKPROGRAMSTRUCTURE_H_ */
