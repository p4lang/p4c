#ifndef BACKENDS_COMMON_PNA_H_
#define BACKENDS_COMMON_PNA_H_

#include "ir/ir.h"
#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/typeMap.h"

namespace P4 {

struct ConversionContext {

};

class PnaProgramStructure {
    P4::ReferenceMap*    refMap;
    P4::TypeMap*         typeMap;

 public:
    // We place scalar user metadata fields (i.e., bit<>, bool)
    // in the scalars map.
    ordered_map<cstring, const IR::Declaration_Variable*> scalars;
    unsigned                            scalars_width = 0;
    unsigned                            error_width = 32;
    unsigned                            bool_width = 1;

    // architecture related information
    // ordered_map<const IR::Node*, std::pair<gress_t, block_t>> block_type;

    ordered_map<cstring, const IR::Type_Header*> header_types;
    ordered_map<cstring, const IR::Type_Struct*> metadata_types;
    ordered_map<cstring, const IR::Type_HeaderUnion*> header_union_types;
    ordered_map<cstring, const IR::Declaration_Variable*> headers;
    ordered_map<cstring, const IR::Declaration_Variable*> metadata;
    ordered_map<cstring, const IR::Declaration_Variable*> header_stacks;
    ordered_map<cstring, const IR::Declaration_Variable*> header_unions;
    ordered_map<cstring, const IR::Type_Error*> errors;
    ordered_map<cstring, const IR::Type_Enum*> enums;
    ordered_map<cstring, const IR::P4Parser*> parsers;
    ordered_map<cstring, const IR::P4ValueSet*> parse_vsets;
    ordered_map<cstring, const IR::P4Control*> deparsers;
    ordered_map<cstring, const IR::P4Control*> pipelines;
    ordered_map<cstring, const IR::Declaration_Instance*> extern_instances;
    ordered_map<cstring, cstring> field_aliases;

    std::vector<const IR::ExternBlock*> globals;

 public:
    PnaProgramStructure(P4::ReferenceMap* refMap, P4::TypeMap* typeMap)
        : refMap(refMap), typeMap(typeMap) {
        CHECK_NULL(refMap);
        CHECK_NULL(typeMap);
    }

    void create(ConversionContext* ctxt);
    void createStructLike(ConversionContext* ctxt, const IR::Type_StructLike* st);
    void createTypes(ConversionContext* ctxt);
    void createHeaders(ConversionContext* ctxt);
    void createScalars(ConversionContext* ctxt);
    void createParsers(ConversionContext* ctxt);
    void createExterns();
    void createActions(ConversionContext* ctxt);
    void createControls(ConversionContext* ctxt);
    void createDeparsers(ConversionContext* ctxt);
    void createGlobals();

    std::set<cstring> non_pipeline_controls;
    std::set<cstring> pipeline_controls;

    bool hasVisited(const IR::Type_StructLike* st) {
        if (auto h = st->to<IR::Type_Header>())
            return header_types.count(h->getName());
        else if (auto s = st->to<IR::Type_Struct>())
            return metadata_types.count(s->getName());
        else if (auto u = st->to<IR::Type_HeaderUnion>())
            return header_union_types.count(u->getName());
        return false;
    }
};

class ParsePnaArchitecture : public Inspector {
    PnaProgramStructure* structure;

 public:
    explicit ParsePnaArchitecture(PnaProgramStructure* structure) :
        structure(structure) { CHECK_NULL(structure); }

    void modelError(const char* format, const IR::INode* node) {
        ::error(ErrorType::ERR_MODEL,
                (cstring(format) + "Are you using an up-to-date 'pna.p4'?").c_str(),
                node->getNode());
    }

    bool preorder(const IR::ToplevelBlock* block) override;
    bool preorder(const IR::PackageBlock* block) override;
    bool preorder(const IR::ExternBlock* block) override;

    profile_t init_apply(const IR::Node *root) override {
        structure->globals.clear();
        return Inspector::init_apply(root);
    }
};

}  // P4

#endif /* BACKENDS_COMMON_PNA_H_ */
