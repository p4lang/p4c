/**
 * Copyright 2013-2024 Intel Corporation.
 *
 * This software and the related documents are Intel copyrighted materials, and your use of them
 * is governed by the express license under which they were provided to you ("License"). Unless
 * the License provides otherwise, you may not use, modify, copy, publish, distribute, disclose
 * or transmit this software or the related documents without Intel's prior written permission.
 *
 * This software and the related documents are provided as is, with no express or implied
 * warranties, other than those that are expressly stated in the License.
 */

#ifndef BACKENDS_TOFINO_BF_P4C_ARCH_FROMV1_0_PROGRAMSTRUCTURE_H_
#define BACKENDS_TOFINO_BF_P4C_ARCH_FROMV1_0_PROGRAMSTRUCTURE_H_

#include <boost/range/adaptor/reversed.hpp>

#include "bf-p4c/arch/fromv1.0/add_metadata_parser_states.h"
#include "bf-p4c/arch/fromv1.0/checksum.h"
#include "bf-p4c/arch/program_structure.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/lib/assoc.h"
#include "bf-p4c/midend/parser_graph.h"
#include "bf-p4c/midend/path_linearizer.h"
#include "frontends/common/constantFolding.h"
#include "frontends/common/resolveReferences/resolveReferences.h"
#include "frontends/p4-14/fromv1.0/converters.h"
#include "frontends/p4-14/fromv1.0/programStructure.h"
#include "frontends/p4/cloner.h"
#include "frontends/p4/createBuiltins.h"
#include "frontends/p4/directCalls.h"
#include "frontends/p4/typeChecking/bindVariables.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "frontends/p4/typeMap.h"
#include "lib/big_int_util.h"

namespace P4::P4V1 {

using namespace P4;

static const cstring COMPILER_META = "__bfp4c_compiler_generated_meta"_cs;

// TODO older definition of ProgramStructure used by 14-to-v1model conversion path
// to be removed
// ** BEGIN **
class TNA_ProgramStructure : public ProgramStructure {
    const IR::Expression *convertHashAlgorithm(Util::SourceInfo srcInfo, IR::ID algorithm) override;

    const IR::P4Table *convertTable(const IR::V1Table *table, cstring newName,
                                    IR::IndexedVector<IR::Declaration> &stateful,
                                    std::map<cstring, cstring> &) override;

    const IR::Declaration_Instance *convertActionProfile(const IR::ActionProfile *action_profile,
                                                         cstring newName) override;

 public:
    static ProgramStructure *create() { return new TNA_ProgramStructure(); }
};
// ** END **

using StateToExtracts = assoc::map<const IR::ParserState *, std::vector<const IR::Member *>>;
using ExtractToState = assoc::map<const IR::Member *, const IR::ParserState *>;
using DigestFieldList =
    std::map<cstring,
             std::map<unsigned, std::pair<std::optional<cstring>, const IR::Expression *>>>;
/**
 * Convert ingress_intrinsic_metadata_from_parser_aux_t to
 *         ingress_intrinsic_metadata_from_parser_t
 * Convert egress_intrinsic_metadata_from_parser_aux_t to
 *         egress_intrinsic_metadata_from_parser_t
 */
class FixParamType : public Transform {
    const IR::Node *postorder(IR::Parameter *param) override {
        auto tn = param->type->to<IR::Type_Name>();
        if (!tn) return param;
        if (tn->path->name == "ingress_intrinsic_metadata_from_parser_aux_t")
            return new IR::Parameter(
                param->name, param->direction,
                new IR::Type_Name(new IR::Path("ingress_intrinsic_metadata_from_parser_t")),
                param->defaultValue);
        else if (tn->path->name == "egress_intrinsic_metadata_from_parser_aux_t")
            return new IR::Parameter(
                param->name, param->direction,
                new IR::Type_Name(new IR::Path("egress_intrinsic_metadata_from_parser_t")),
                param->defaultValue);
        return param;
    }
};

class ConvertConcreteHeaderRefToPathExpression : public Transform {
    const IR::Node *postorder(IR::ConcreteHeaderRef *href) override {
        if (href->ref->name == "standard_metadata")
            return new IR::Member(new IR::PathExpression("meta"), href->ref->name);
        auto expr =
            new IR::PathExpression(href->srcInfo, href->type, new IR::Path(href->ref->name));
        return expr;
    }
};

/**
 * Extend P4V1::ProgramStructure with TNA-specific info
 */
class TnaProgramStructure : public ProgramStructure {
    // hash
    const IR::Expression *convertHashAlgorithm(Util::SourceInfo srcInfo, IR::ID algorithm) override;
    IR::ConstructorCallExpression *createHashExtern(const IR::Expression *, const IR::Type *);

    // table
    const IR::P4Table *convertTable(const IR::V1Table *table, cstring newName,
                                    IR::IndexedVector<IR::Declaration> &stateful,
                                    std::map<cstring, cstring> &) override;

    // action profile
    const IR::Declaration_Instance *convertActionProfile(const IR::ActionProfile *action_profile,
                                                         cstring newName) override;

    // counter and meter
    const IR::Declaration_Instance *convertDirectCounter(const IR::Counter *c,
                                                         cstring newName) override;
    const IR::Declaration_Instance *convertDirectMeter(const IR::Meter *c,
                                                       cstring newName) override;
    const IR::Statement *convertMeterCall(const IR::Meter *m) override;
    const IR::Statement *convertCounterCall(cstring c) override;
    const IR::Expression *counterType(const IR::CounterOrMeter *cm) override;
    const IR::Declaration_Instance *convert(const IR::Register *reg, cstring newName,
                                            const IR::Type *regElementType) override;

    // control
    IR::Vector<IR::Argument> *createApplyArguments(cstring n) override;
    const IR::P4Control *convertControl(const IR::V1Control *control, cstring newName) override;
    const IR::ParserState *convertParser(const IR::V1Parser *control,
                                         IR::IndexedVector<IR::Declaration> *) override;
    const IR::Declaration_Instance *convert(const IR::CounterOrMeter *cm, cstring newName) override;
    const IR::Type_Control *controlType(IR::ID name) override;

    void addIngressParams(IR::ParameterList *param);
    void addEgressParams(IR::ParameterList *param);
    void setupControlArguments();

    // metadata
    const IR::Expression *stdMetaReference(const IR::Parameter *param);
    void checkHeaderType(const IR::Type_StructLike *hdr, bool toStruct) override;

    // action
    std::vector<cstring> findActionInTables(const IR::V1Control *control);

    // parsers
    IR::IndexedVector<IR::ParserState> *createIngressParserStates();
    IR::IndexedVector<IR::ParserState> *createEgressParserStates();
    const IR::ParserState *createEmptyMirrorState(cstring nextState);
    const IR::ParserState *createMirrorState(gress_t, unsigned, const IR::Expression *, cstring,
                                             cstring);
    const IR::ParserState *createResubmitState(gress_t, unsigned, const IR::Expression *, cstring,
                                               cstring);
    const IR::SelectCase *createSelectCase(gress_t gress, unsigned digestId,
                                           const IR::ParserState *newState);
    IR::IndexedVector<IR::ParserState> *createMirrorStates();
    IR::IndexedVector<IR::ParserState> *createResubmitStates();

    // types
    void removeType(cstring typeName, cstring headerName);
    void removeP14IntrinsicMetadataTypes();
    void createType(cstring typeName, cstring headerName);
    void createIntrinsicMetadataTypes();
    void loadModel() override;

    // helpers
    void collectControlGressMapping();
    void collectBridgedFieldsInControls();
    void collectBridgedFieldsInParsers();
    void collectBridgedFields();
    void collectDigestFields();
    void addBridgedFieldsFromChecksumUpdate(IR::IndexedVector<IR::StructField> *);
    void createCompilerGeneratedTypes();
    cstring convertLinearPathToStructFieldName(BFN::LinearPath *path);
    void checkForReservedTnaTypes();
    void collectHeaderReference(const IR::V1Control *control);
    void parseUpdateLocationAnnotation(std::set<gress_t> &, const IR::Annotation *, cstring);
    bool useResidualChecksum();
    bool useBridgeMetadata();

    // print program structure
    void getStructFieldsFromFieldList(IR::IndexedVector<IR::StructField> *,
                                      const IR::ListExpression *);
    void createDigestHeaderTypeAndInstance(unsigned index, const IR::Expression *expr, cstring name,
                                           std::optional<IR::IndexedVector<IR::StructField>>);
    const IR::StatOrDecl *createDigestEmit(
        cstring, unsigned, std::pair<std::optional<cstring>, const IR::Expression *>, cstring,
        cstring, cstring);
    const IR::StatOrDecl *createBridgeEmit();
    using ChecksumUpdateInfo = std::pair<IR::StatOrDecl *, ChecksumInfo>;
    void createChecksumUpdateStatements(gress_t gress, IR::IndexedVector<IR::Declaration> &,
                                        std::vector<ChecksumUpdateInfo> &);
    void createChecksumVerifyStatements(gress_t gress);
    void createIngressParser();
    void createEgressParser();
    void createParser() override;
    void createIngressDeparser();
    void createEgressDeparser();
    void createDeparser() override;
    void createPipeline();
    void createMain() override;
    const IR::P4Program *create(Util::SourceInfo info) override;

 public:
    // used to indicate which gress the current control block being
    // translated is invoked in.
    gress_t currentGress;
    std::map<cstring, bitvec> controlParamUse;
    std::map<cstring, gress_t> mapControlToGress;
    gress_t getGress(cstring name);
    std::map<unsigned long, unsigned> cloneIndexHashes[2];
    std::map<unsigned long, unsigned> resubmitIndexHashes;
    std::map<unsigned long, unsigned> digestIndexHashes;
    std::map<unsigned long, unsigned> recirculateIndexHashes;
    std::map<cstring, std::vector<const IR::Parameter *>> controlParams;
    // map { prim->name :
    //       map { field_list_index :
    //             pair(field_list_name, field_list_expr) } }
    std::map<cstring, std::map<unsigned, std::pair<std::optional<cstring>, const IR::Expression *>>>
        digestFieldLists;
    ordered_set<cstring> bridgedFields;
    std::map<cstring, BFN::LinearPath> bridgedFieldInfo;
    std::map<cstring, const IR::Type_StructLike *> tna_intr_md_types;

    IR::Vector<IR::StatOrDecl> checksumVerify;

    std::vector<ChecksumInfo> residualChecksums;
    std::vector<ChecksumInfo> verifyChecksums;

    assoc::map<const IR::FieldList *, cstring> residualChecksumNames;

    ordered_map<cstring, ordered_set<cstring>> ingressVerifyChecksumToStates;

    std::map<gress_t, std::map<cstring, IR::Statement *>> checksumDepositToHeader;

    static ProgramStructure *create() { return new TnaProgramStructure(); }

    template <typename Func>
    void forAllResidualChecksums(std::vector<const IR::CalculatedField *> calculated_fields,
                                 Func function) {
        for (auto cf : calculated_fields) {
            for (auto uov : cf->specs) {
                if (!uov.update) continue;
                auto flc = field_list_calculations.get(uov.name.name);
                auto fl = getFieldLists(flc);
                if (fl == nullptr) continue;
                if (fl->payload) {
                    function(fl);
                }
            }
        }
    }

    template <typename Func>
    void forAllChecksums(std::vector<const IR::CalculatedField *> calculated_fields,
                         Func function) {
        for (auto cf : calculated_fields) {
            for (auto uov : cf->specs) {
                if (!uov.update) continue;
                auto flc = field_list_calculations.get(uov.name.name);
                auto fl = getFieldLists(flc);
                if (fl == nullptr) continue;
                function(cf, flc, uov, fl);
            }
        }
    }
};

// convert P4-14 metadata to the corresponding P4-16 intrinsic metadata
class ConvertMetadata : public Transform {
    TnaProgramStructure *structure;
    std::map<cstring, const IR::Expression *> ingressMetadataNameMap;
    std::map<cstring, const IR::Expression *> egressMetadataNameMap;
    std::map<cstring, int> widthMap;

    void cvt(gress_t gress, cstring src, int src_width, const IR::Member *dst) {
        auto &nameMap =
            (gress == gress_t::INGRESS) ? ingressMetadataNameMap : egressMetadataNameMap;
        if (src_width > dst->type->width_bits()) {
            auto casted = new IR::Cast(IR::Type_Bits::get(src_width), dst);
            nameMap.emplace(src, casted);
        } else {
            nameMap.emplace(src, dst);
        }
        widthMap.emplace(src, src_width);
    }

    const IR::Member *mkMember(cstring meta, cstring strct, cstring field, int width) {
        auto mem =
            new IR::Member(new IR::Member(new IR::PathExpression(IR::ID(meta)), strct), field);
        mem->type = IR::Type::Bits::get(width);
        return mem;
    }

    const IR::Member *mkMember(cstring strct, cstring field, int width) {
        auto mem = new IR::Member(new IR::PathExpression(IR::ID(strct)), field);
        mem->type = IR::Type::Bits::get(width);
        return mem;
    }

 public:
    explicit ConvertMetadata(TnaProgramStructure *s) : structure(s) {
        int portWidth = Device::portBitWidth();
        // metadata to translate in ingress
        cvt(INGRESS, "ig_intr_md_from_parser_aux.ingress_parser_err"_cs, 16,
            mkMember("ig_intr_md_from_parser_aux"_cs, "parser_err"_cs, 16));
        cvt(INGRESS, "meta.standard_metadata.egress_spec"_cs, 9,
            mkMember("ig_intr_md_for_tm"_cs, "ucast_egress_port"_cs, portWidth));
        cvt(INGRESS, "meta.standard_metadata.ingress_port"_cs, 9,
            mkMember("ig_intr_md"_cs, "ingress_port"_cs, portWidth));
        cvt(INGRESS, "ig_intr_md_for_tm.drop_ctl"_cs, 1,
            mkMember("ig_intr_md_for_dprsr"_cs, "drop_ctl"_cs, 1));
        cvt(INGRESS, "ig_intr_md_from_parser_aux.ingress_global_tstamp"_cs, 48,
            mkMember("ig_intr_md_from_parser_aux"_cs, "global_tstamp"_cs, 48));
        cvt(INGRESS, "ig_intr_md_for_mb.mirror_hash"_cs, 13,
            mkMember("ig_intr_md_for_dprsr"_cs, "mirror_hash"_cs, 13));
        cvt(INGRESS, "ig_intr_md_for_mb.mirror_io_select"_cs, 1,
            mkMember("ig_intr_md_for_dprsr"_cs, "mirror_io_select"_cs, 1));
        cvt(INGRESS, "ig_intr_md_for_mb.mirror_multicast_ctrl"_cs, 1,
            mkMember("ig_intr_md_for_dprsr"_cs, "mirror_multicast_ctrl"_cs, 1));
        cvt(INGRESS, "ig_intr_md_for_mb.mirror_ingress_cos"_cs, 3,
            mkMember("ig_intr_md_for_dprsr"_cs, "mirror_ingress_cos"_cs, 3));
        cvt(INGRESS, "ig_intr_md_for_mb.mirror_deflect_on_drop"_cs, 1,
            mkMember("ig_intr_md_for_dprsr"_cs, "mirror_deflect_on_drop"_cs, 1));
        cvt(INGRESS, "ig_intr_md_for_mb.mirror_copy_to_cpu_ctrl"_cs, 1,
            mkMember("ig_intr_md_for_dprsr"_cs, "mirror_copy_to_cpu_ctrl"_cs, 1));
        cvt(INGRESS, "ig_intr_md_for_mb.mirror_copy_to_cpu_ctrl"_cs, 1,
            mkMember("ig_intr_md_for_dprsr"_cs, "mirror_copy_to_cpu_ctrl"_cs, 1));
        cvt(INGRESS, "ig_intr_md_for_mb.mirror_egress_port"_cs, portWidth,
            mkMember("ig_intr_md_for_dprsr"_cs, "mirror_egress_port"_cs, portWidth));
        cvt(INGRESS, "standard_metadata.ingress_port"_cs, 9,
            mkMember("ig_intr_md"_cs, "ingress_port"_cs, portWidth));

        // metadata to translate in egress
        cvt(EGRESS, "eg_intr_md_from_parser_aux.egress_parser_err"_cs, 16,
            mkMember("eg_intr_md_from_parser_aux"_cs, "parser_err"_cs, 16));
        cvt(EGRESS, "eg_intr_md_from_parser_aux.clone_src"_cs, 4,
            mkMember("meta"_cs, COMPILER_META, "clone_src"_cs, 4));
        cvt(EGRESS, "eg_intr_md_from_parser_aux.egress_global_tstamp"_cs, 48,
            mkMember("eg_intr_md_from_parser_aux"_cs, "global_tstamp"_cs, 48));
        cvt(EGRESS, "eg_intr_md_for_oport.drop_ctl"_cs, 3,
            mkMember("eg_intr_md_for_dprsr"_cs, "drop_ctl"_cs, 3));
        cvt(EGRESS, "eg_intr_md_from_parser_aux.egress_global_ver"_cs, 32,
            mkMember("eg_intr_md_from_parser_aux"_cs, "global_ver"_cs, 32));
        cvt(EGRESS, "eg_intr_md.deq_timedelta"_cs, 32,
            mkMember("eg_intr_md"_cs, "deq_timedelta"_cs, 18));
        cvt(EGRESS, "eg_intr_md_for_mb.mirror_hash"_cs, 13,
            mkMember("eg_intr_md_for_dprsr"_cs, "mirror_hash"_cs, 13));
        cvt(EGRESS, "eg_intr_md_for_mb.mirror_io_select"_cs, 1,
            mkMember("eg_intr_md_for_dprsr"_cs, "mirror_io_select"_cs, 1));
        cvt(EGRESS, "eg_intr_md_for_mb.mirror_multicast_ctrl"_cs, 1,
            mkMember("eg_intr_md_for_dprsr"_cs, "mirror_multicast_ctrl"_cs, 1));
        cvt(EGRESS, "eg_intr_md_for_mb.mirror_ingress_cos"_cs, 3,
            mkMember("eg_intr_md_for_dprsr"_cs, "mirror_ingress_cos"_cs, 3));
        cvt(EGRESS, "eg_intr_md_for_mb.mirror_deflect_on_drop"_cs, 1,
            mkMember("eg_intr_md_for_dprsr"_cs, "mirror_deflect_on_drop"_cs, 1));
        cvt(EGRESS, "eg_intr_md_for_mb.mirror_copy_to_cpu_ctrl"_cs, 1,
            mkMember("eg_intr_md_for_dprsr"_cs, "mirror_copy_to_cpu_ctrl"_cs, 1));
        cvt(EGRESS, "eg_intr_md_for_mb.mirror_copy_to_cpu_ctrl"_cs, 1,
            mkMember("eg_intr_md_for_dprsr"_cs, "mirror_copy_to_cpu_ctrl"_cs, 1));
        cvt(EGRESS, "eg_intr_md_for_mb.mirror_egress_port"_cs, portWidth,
            mkMember("eg_intr_md_for_dprsr"_cs, "mirror_egress_port"_cs, portWidth));
        cvt(EGRESS, "meta.standard_metadata.ingress_port"_cs, 9,
            mkMember("meta"_cs, "ig_intr_md"_cs, "ingress_port"_cs, portWidth));
        cvt(EGRESS, "standard_metadata.ingress_port"_cs, 9,
            mkMember("meta"_cs, "ig_intr_md"_cs, "ingress_port"_cs, portWidth));
    }

    // convert P4-14 metadata to P4-16 tna metadata
    const IR::Node *preorder(IR::Member *member) override {
        prune();
        if (!member->type->is<IR::Type_Bits>()) return member;

        gress_t thread;
        if (auto *parser = findOrigCtxt<IR::P4Parser>()) {
            thread = (parser->name == "IngressParserImpl") ? INGRESS : EGRESS;
        } else if (auto *control = findOrigCtxt<IR::P4Control>()) {
            if (structure->mapControlToGress.count(control->name)) {
                thread = structure->mapControlToGress.at(control->name);
            } else if (control->name == "EgressDeparserImpl") {
                thread = EGRESS;
            } else if (control->name == "IngressDeparserImpl") {
                thread = INGRESS;
            } else {
                // control might be dead-code, ignore.
                return member;
            }
        } else {
            return member;
        }

        auto &nameMap = (thread == INGRESS) ? ingressMetadataNameMap : egressMetadataNameMap;

        auto linearizer = new BFN::PathLinearizer();
        member->apply(*linearizer);
        if (!linearizer->linearPath) return member;
        auto path = *linearizer->linearPath;
        auto fn = path.to_cstring("."_cs, false);
        if (nameMap.count(fn)) {
            LOG3("Translating " << member << " to " << nameMap.at(fn));
            return nameMap.at(fn);
        }
        return member;
    }
};

class FixApplyStatement : public Transform {
    TnaProgramStructure *structure;
    const IR::Node *postorder(IR::MethodCallExpression *m) override;

 public:
    explicit FixApplyStatement(TnaProgramStructure *s) : structure(s) {}
};

class CollectBridgedFields : public Inspector, P4WriteContext {
    // the gress of the control block
    gress_t gress;
    ordered_set<cstring> &mayWrite;
    ordered_set<cstring> &mayRead;
    std::map<cstring, BFN::LinearPath> &fieldInfo;

 public:
    CollectBridgedFields(gress_t g, ordered_set<cstring> &w, ordered_set<cstring> &r,
                         std::map<cstring, BFN::LinearPath> &p)
        : gress(g), mayWrite(w), mayRead(r), fieldInfo(p) {}
    bool preorder(const IR::Member *expr) override;
    bool preorder(const IR::MethodCallExpression *m) override;
};

unsigned long computeHashOverFieldList(const IR::Expression *expr,
                                       std::map<unsigned long, unsigned> &hm,
                                       bool reserve_entry_zero = false);

class CollectDigestFields : public Inspector {
    TnaProgramStructure *structure;
    gress_t gress;
    DigestFieldList &digestFieldLists;

    IR::Expression *flatten(const IR::ListExpression *);
    void convertFieldList(const IR::Primitive *prim, size_t fieldListIndex,
                          std::map<unsigned long, unsigned> &indexHashes, bool);

 public:
    explicit CollectDigestFields(TnaProgramStructure *s, gress_t g, DigestFieldList &dfl)
        : structure(s), gress(g), digestFieldLists(dfl) {}
    bool preorder(const IR::Primitive *p) override;
};

class RenameFieldPath : public Transform {
 public:
    cstring prefix;
    std::set<cstring> paths;
    RenameFieldPath() {}

    IR::Node *preorder(IR::Member *member) {
        BUG_CHECK(prefix != "", "must provide a prefix to the RenameFieldPath pass");
        BFN::PathLinearizer linearizer;
        member->apply(linearizer);
        auto path = linearizer.linearPath;
        if (!path) return member;
        if (auto hdr = path->components.front()->to<IR::PathExpression>()) {
            if (paths.count(hdr->path->name)) {
                auto meta = new IR::Member(
                    new IR::Member(new IR::PathExpression(prefix), IR::ID(hdr->path->name)),
                    member->member);
                LOG3("adding " << prefix << " to " << member);
                return meta;
            }
        } else if (auto hdr = path->components.front()->to<IR::ConcreteHeaderRef>()) {
            if (paths.count(hdr->ref->name)) {
                auto meta = new IR::Member(
                    new IR::Member(new IR::PathExpression(prefix), IR::ID(hdr->ref->name)),
                    member->member);
                LOG3("adding " << prefix << " to " << member);
                return meta;
            }
        }
        return member;
    }

    IR::Node *preorder(IR::PathExpression *path) {
        BUG_CHECK(prefix != "", "must provide a prefix to the RenameFieldPath pass");
        if (paths.count(path->path->name)) {
            auto meta = new IR::Member(new IR::PathExpression(prefix), IR::ID(path->path->name));
            LOG3("adding " << prefix << " to " << path);
            return meta;
        }
        return path;
    }
};

/**
 * This pass is used after converting to tna to fix the path to bridged
 * ingress intrinsic metadata.
 */
class FixBridgedIntrinsicMetadata : public RenameFieldPath {
    TnaProgramStructure *structure;

 public:
    explicit FixBridgedIntrinsicMetadata(TnaProgramStructure *s) : structure(s) {
        paths = {"ig_intr_md"_cs, "ig_intr_md_for_tm"_cs};
        prefix = "meta"_cs;
    }
    IR::Node *preorder(IR::P4Parser *parser) override {
        if (parser->name == "IngressParserImpl") {
            prune();
        }
        return parser;
    }
    IR::Node *preorder(IR::P4Control *control) override {
        LOG5("control " << control->name);
        if (structure->mapControlToGress.count(control->name)) {
            if (structure->mapControlToGress.at(control->name) == INGRESS) {
                LOG5("\tskip " << control->name);
                prune();
            }
        }
        return control;
    }
};

// rename 'pktgen_port_down' to 'hdr.pktgen_port_down'
class FixPktgenHeaderPath : public RenameFieldPath {
 public:
    FixPktgenHeaderPath() {
        paths = {"pktgen_port_down"_cs, "pktgen_recirc"_cs, "pktgen_generic"_cs, "pktgen_timer"_cs};
        prefix = "hdr"_cs;
    }
};

/**
 *  Temporary remove reference to ig_prsr_ctrl until parser priority is supported
 */
class FixParserPriority : public Transform {
 public:
    FixParserPriority() {}
    IR::Node *preorder(IR::AssignmentStatement *assign) override;
};

class ParserCounterSelectCaseConverter : public Transform {
 public:
    bool isNegative = false;
    bool needsCast = false;
    int counterIdx = -1;

 public:
    ParserCounterSelectCaseConverter() {}

    void cannotFit(const IR::AssignmentStatement *stmt, const char *what) {
        error("Parser counter %1% amount cannot fit into 8-bit. %2%", what, stmt);
    }

    bool isParserCounter(const IR::Member *member) {
        auto linearizer = new BFN::PathLinearizer();
        member->apply(*linearizer);
        if (!linearizer->linearPath) return false;
        auto path = linearizer->linearPath->to_cstring();
        return (path == "ig_prsr_ctrl.parser_counter");
    }

    std::pair<unsigned, unsigned> getAlignLoHi(const IR::Member *member) {
        auto header = member->expr->type->to<IR::Type_Header>();

        CHECK_NULL(header);

        unsigned bits = 0;

        for (auto field : boost::adaptors::reverse(header->fields)) {
            auto size = field->type->width_bits();

            if (field->name == member->member) {
                if (size > 8) {
                    ::fatal_error(
                        "Parser counter load field is of width %1% bits"
                        " which is greater than what HW supports (8 bits): %2%",
                        size, member);
                }

                return {bits % 8, (bits + size - 1) % 8};
            }

            bits += size;
        }

        BUG("%1% not found in header?", member->member);
    }

    // Simplified strength reduction to simplify
    // the expressions that is possible to implement
    // for parser counter:
    // prsr_ctr = prsr_ctr - N;
    // prsr_ctr = field * M - N;
    struct StrengthReduction : public Transform {
        bool isZero(const IR::Expression *expr) const {
            auto cst = expr->to<IR::Constant>();
            if (cst == nullptr) return false;
            return cst->value == 0;
        }

        bool isOne(const IR::Expression *expr) const {
            auto cst = expr->to<IR::Constant>();
            if (cst == nullptr) return false;
            return cst->value == 1;
        }

        int isPowerOf2(const IR::Expression *expr) const {
            auto cst = expr->to<IR::Constant>();
            if (cst == nullptr) return -1;
            big_int value = cst->value;
            if (value <= 0) return -1;
            auto bitcnt = bitcount(value);
            if (bitcnt != 1) return -1;
            auto log = Util::scan1(value, 0);
            // Assumes value does not have more than 2 billion bits
            return log;
        }

        const IR::Node *postorder(IR::Sub *expr) {
            if (isZero(expr->right)) return expr->left;
            if (isZero(expr->left)) return new IR::Neg(expr->srcInfo, expr->right);
            // Replace `a - constant` with `a + (-constant)`
            if (expr->right->is<IR::Constant>()) {
                auto cst = expr->right->to<IR::Constant>();
                auto neg = new IR::Constant(cst->srcInfo, cst->type, -cst->value, cst->base, true);
                auto result = new IR::Add(expr->srcInfo, expr->left, neg);
                return result;
            }
            return expr;
        }

        const IR::Node *postorder(IR::Mul *expr) {
            if (isZero(expr->left)) return expr->left;
            if (isZero(expr->right)) return expr->right;
            if (isOne(expr->left)) return expr->right;
            if (isOne(expr->right)) return expr->left;
            auto exp = isPowerOf2(expr->left);
            if (exp >= 0) {
                auto amt = new IR::Constant(exp);
                auto sh = new IR::Shl(expr->srcInfo, expr->right, amt);
                return sh;
            }
            exp = isPowerOf2(expr->right);
            if (exp >= 0) {
                auto amt = new IR::Constant(exp);
                auto sh = new IR::Shl(expr->srcInfo, expr->left, amt);
                return sh;
            }
            return expr;
        }
    };

    const IR::Node *preorder(IR::AssignmentStatement *assign) {
        prune();
        auto stmt = getOriginal<IR::AssignmentStatement>();
        auto parserCounter = new IR::PathExpression("ig_prsr_ctrl_parser_counter"_cs);
        auto right = stmt->right;
        auto left = stmt->left;

        // Remove slice around the destination of the assignment.
        while (auto slice = left->to<IR::Slice>()) left = slice->e0;
        while (auto cast = left->to<IR::Cast>()) left = cast->expr;
        auto linearizer = new BFN::PathLinearizer();
        left->apply(*linearizer);
        auto path = *linearizer->linearPath;
        if (path.to_cstring() != "ig_prsr_ctrl.parser_counter") return assign;

        // Remove any casts around the source of the assignment.
        if (auto cast = right->to<IR::Cast>()) {
            if (cast->destType->is<IR::Type_Bits>()) {
                right = cast->expr;
            }
        }

        // Strength reduce IR::Sub to IR::Add
        if (right->is<IR::Sub>()) right = right->apply(StrengthReduction());

        IR::MethodCallStatement *methodCall = nullptr;

        if (right->to<IR::Constant>() || right->to<IR::Member>()) {
            // Load operation (immediate or field)
            methodCall = new IR::MethodCallStatement(
                stmt->srcInfo, new IR::MethodCallExpression(
                                   stmt->srcInfo, new IR::Member(parserCounter, "set"),
                                   new IR::Vector<IR::Type>({stmt->right->type}),
                                   new IR::Vector<IR::Argument>({new IR::Argument(stmt->right)})));
        } else if (auto add = right->to<IR::Add>()) {
            auto member = add->left->to<IR::Member>();

            auto counterWidth = IR::Type::Bits::get(8);
            auto maskWidth = IR::Type::Bits::get(Device::currentDevice() == Device::TOFINO ? 3 : 8);
            // How does user specify the max in P4-14?
            auto max = new IR::Constant(counterWidth, 255);

            // Add operaton
            if (member && isParserCounter(member)) {
                methodCall = new IR::MethodCallStatement(stmt->srcInfo,
                                                         new IR::Member(parserCounter, "increment"),
                                                         {new IR::Argument(add->right)});
            } else {
                if (auto *amt = add->right->to<IR::Constant>()) {
                    // Load operation (expression of field)
                    if (member) {
                        auto shr = new IR::Constant(counterWidth, 0);
                        auto mask = new IR::Constant(
                            maskWidth, Device::currentDevice() == Device::TOFINO ? 7 : 255);
                        auto add = new IR::Constant(counterWidth, amt->asUnsigned());

                        methodCall = new IR::MethodCallStatement(
                            stmt->srcInfo, new IR::MethodCallExpression(
                                               stmt->srcInfo, new IR::Member(parserCounter, "set"),
                                               new IR::Vector<IR::Type>({member->type}),
                                               new IR::Vector<IR::Argument>(
                                                   {new IR::Argument(member), new IR::Argument(max),
                                                    new IR::Argument(shr), new IR::Argument(mask),
                                                    new IR::Argument(add)})));
                    } else if (auto *shl = add->left->to<IR::Shl>()) {
                        if (auto *rot = shl->right->to<IR::Constant>()) {
                            auto left = shl->left;
                            // Remove any casts around the parameter;
                            if (auto cast = left->to<IR::Cast>()) {
                                if (cast->destType->is<IR::Type_Bits>()) {
                                    left = cast->expr;
                                }
                            }

                            if (auto *field = left->to<IR::Member>()) {
                                if (!rot->fitsUint() || rot->asUnsigned() >> 8)
                                    cannotFit(stmt, "multiply");

                                if (!amt->fitsUint() || amt->asUnsigned() >> 8)
                                    cannotFit(stmt, "immediate");

                                unsigned lo = 0, hi = 7;
                                std::tie(lo, hi) = getAlignLoHi(field);

                                auto shr =
                                    new IR::Constant(counterWidth, 8 - rot->asUnsigned() - lo);

                                unsigned rot_hi = std::min(hi + rot->asUnsigned(), 7u);
                                auto mask = new IR::Constant(
                                    maskWidth, Device::currentDevice() == Device::TOFINO
                                                   ? rot_hi
                                                   : (1 << (rot_hi + 1)) - 1);
                                auto add = new IR::Constant(counterWidth, amt->asUnsigned());

                                methodCall = new IR::MethodCallStatement(
                                    stmt->srcInfo,
                                    new IR::MethodCallExpression(
                                        stmt->srcInfo, new IR::Member(parserCounter, "set"),
                                        new IR::Vector<IR::Type>({field->type}),
                                        new IR::Vector<IR::Argument>(
                                            {new IR::Argument(field), new IR::Argument(max),
                                             new IR::Argument(shr), new IR::Argument(mask),
                                             new IR::Argument(add)})));
                            }
                        }
                    }
                }
            }
        }
        if (!methodCall) error("Unsupported syntax for parser counter: %1%", stmt);
        return methodCall;
    }

    const IR::Node *preorder(IR::SelectExpression *node) {
        bool isPrsrCtr = false;
        for (unsigned i = 0; i < node->select->components.size(); i++) {
            auto select = node->select->components[i];
            if (auto member = select->to<IR::Member>()) {
                if (isParserCounter(member)) {
                    if (counterIdx >= 0) error("Multiple selects on parser counter in %1%", node);
                    counterIdx = i;
                    isPrsrCtr = true;
                }
            }
        }
        if (!isPrsrCtr) prune();
        return node;
    }

    const IR::Node *postorder(IR::SelectExpression *node) {
        counterIdx = -1;
        return node;
    }

    struct RewriteSelectCase : Transform {
        bool isNegative = false;
        bool needsCast = false;

        const IR::Expression *convert(const IR::Constant *c, bool toBool = true,
                                      bool check = true) {
            auto val = c->asUnsigned();
            if (check) {
                if (val & 0x80) {
                    isNegative = true;
                } else if (val) {
                    error(
                        "Parser counter only supports test "
                        "of value being zero or negative.");
                }
            }
            if (toBool)
                return new IR::BoolLiteral(~val);
            else
                return new IR::Constant(IR::Type::Bits::get(1), ~val & 1);
        }

        const IR::Node *preorder(IR::Mask *mask) override {
            prune();
            mask->left = convert(mask->left->to<IR::Constant>(), false);
            mask->right = convert(mask->right->to<IR::Constant>(), false, false);
            needsCast = true;
            return mask;
        }

        const IR::Node *preorder(IR::Constant *c) override { return convert(c); }
    };

    const IR::Node *preorder(IR::SelectCase *node) {
        RewriteSelectCase rewrite;

        if (auto list = node->keyset->to<IR::ListExpression>()) {
            auto newList = list->clone();
            for (unsigned i = 0; i < newList->components.size(); i++) {
                if (int(i) == counterIdx) {
                    newList->components[i] = newList->components[i]->apply(rewrite);
                    break;
                }
            }
            node->keyset = newList;
        } else {
            node->keyset = node->keyset->apply(rewrite);
        }
        isNegative |= rewrite.isNegative;
        needsCast |= rewrite.needsCast;
        return node;
    }
};

struct ParserCounterSelectExprConverter : Transform {
    const ParserCounterSelectCaseConverter &caseConverter;
    std::set<cstring> need_parser_counter;

    explicit ParserCounterSelectExprConverter(const ParserCounterSelectCaseConverter &cc)
        : caseConverter(cc) {}

    bool isParserCounter(const IR::Member *member) {
        auto linearizer = new BFN::PathLinearizer();
        member->apply(*linearizer);
        if (!linearizer->linearPath) return false;
        auto path = linearizer->linearPath->to_cstring();
        return (path == "ig_prsr_ctrl.parser_counter");
    }

    const IR::Node *postorder(IR::Member *node) {
        if (isParserCounter(node)) {
            auto prsr = findContext<IR::P4Parser>();
            need_parser_counter.insert(prsr->name);
            auto parserCounter = new IR::PathExpression("ig_prsr_ctrl_parser_counter");
            auto testExpr =
                new IR::Member(parserCounter, caseConverter.isNegative ? "is_negative" : "is_zero");
            const IR::Expression *methodCall = new IR::MethodCallExpression(
                node->srcInfo, testExpr, new IR::Vector<IR::Argument>());
            if (caseConverter.needsCast)
                methodCall = new IR::Cast(IR::Type::Bits::get(1), methodCall);
            return methodCall;
        }
        return node;
    }

    const IR::Node *postorder(IR::P4Parser *parser) {
        if (need_parser_counter.count(parser->name) != 0) {
            auto type = new IR::Type_Name("ParserCounter");
            auto decl = new IR::Declaration_Instance("ig_prsr_ctrl_parser_counter", type,
                                                     new IR::Vector<IR::Argument>());
            auto locals = parser->parserLocals.clone();
            locals->push_back(decl);
            return new IR::P4Parser(parser->srcInfo, parser->name, parser->type,
                                    parser->constructorParams, *locals, parser->states);
        }
        return parser;
    }
};

class FixParserCounter : public PassManager {
 public:
    FixParserCounter() {
        auto convertSelectCase = new ParserCounterSelectCaseConverter;
        auto convertSelectExpr = new ParserCounterSelectExprConverter(*convertSelectCase);
        addPasses({convertSelectCase, convertSelectExpr});
    }
};

class FixIdleTimeout : public Transform {
 public:
    FixIdleTimeout() {}
    const IR::Node *preorder(IR::Property *prop) override {
        if (prop->name == "support_timeout") {
            return new IR::Property("idle_timeout", prop->annotations, prop->value,
                                    prop->isConstant);
        }
        return prop;
    }
};

class FixDuplicatePathExpression : public Transform {
 public:
    FixDuplicatePathExpression() {}
    const IR::Node *preorder(IR::P4Program *program) override {
        prune();
        P4::CloneExpressions cloner;
        return program->apply(cloner);
    }
};

// undo the changes to program for checksum translation
class RemoveBuiltins : public Modifier {
 public:
    RemoveBuiltins() {}
    void postorder(IR::P4Parser *parser) override {
        parser->states.removeByName(IR::ParserState::accept);
        parser->states.removeByName(IR::ParserState::reject);
    }
};

class ModifyParserForChecksum : public Modifier {
    P4::ReferenceMap *refMap;
    P4::TypeMap *typeMap;
    TnaProgramStructure *structure;
    P4ParserGraphs *graph;
    ordered_map<cstring, cstring> checksum;
    std::map<cstring, std::set<cstring>> need_checksum;
    std::map<cstring, ordered_set<const IR::Member *>> residualChecksumPayloadFields;

 public:
    template <typename Func>
    void forAllExtracts(const IR::ParserState *state, Func function) {
        P4::CloneExpressions cloner;
        for (auto expr : state->components) {
            if (!expr->is<IR::MethodCallStatement>()) continue;
            auto mce = expr->to<IR::MethodCallStatement>()->methodCall;
            auto inst = P4::MethodInstance::resolve(mce, refMap, typeMap, nullptr /* ctxt */,
                                                    true /* incomplete */);
            if (inst == nullptr) continue;
            if (auto em = inst->to<P4::ExternMethod>()) {
                if (em->actualExternType->name == "packet_in" && em->method->name == "extract") {
                    auto extracted = inst->substitution.lookupByName("hdr"_cs)->apply(cloner);
                    if (extracted == nullptr ||
                        !extracted->to<IR::Argument>()->expression->is<IR::Member>())
                        continue;
                    function(extracted->to<IR::Argument>()->expression->to<IR::Member>());
                }
            }
        }
    }

    static bool equiv(const IR::Expression *a, const IR::Expression *b) {
        if (a == b) return true;
        if (typeid(*a) != typeid(*b)) return false;
        if (auto ma = a->to<IR::Member>()) {
            auto mb = b->to<IR::Member>();
            return ma->member == mb->member && equiv(ma->expr, mb->expr);
        }
        if (auto pa = a->to<IR::PathExpression>()) {
            auto pb = b->to<IR::PathExpression>();
            return pa->path->name == pb->path->name;
        }
        return false;
    }

    static bool belongsTo(const IR::Member *a, const IR::Member *b) {
        if (!a || !b) return false;

        // case 1: a is field, b is field
        if (equiv(a, b)) return true;

        // case 2: a is field, b is header
        if (a->type->is<IR::Type::Bits>()) {
            if (equiv(a->expr, b)) return true;
        }

        return false;
    }

    ModifyParserForChecksum(P4::ReferenceMap *refMap, P4::TypeMap *typeMap,
                            TnaProgramStructure *structure, P4ParserGraphs *graph)
        : refMap(refMap), typeMap(typeMap), structure(structure), graph(graph) {}

    // this function must be invoked from IR::ParserState visitor.
    ordered_set<const IR::Member *> collectResidualChecksumPayloadFields() {
        auto orig = getOriginal<IR::ParserState>();
        BUG_CHECK(orig != nullptr, "function must be invoked within ParserState visitor");
        ordered_set<const IR::Member *> rv;
        auto descendants = graph->get_all_descendants(orig);
        for (auto d : descendants) {
            forAllExtracts(d, [&](const IR::Member *member) { rv.insert(member); });
        }
        return rv;
    }

    // iterate through residual checksum calculation,
    // create subtract call for the fields that belong to the provided member;
    void createSubtractCallsForResidualChecksum(IR::IndexedVector<IR::StatOrDecl> *statements,
                                                std::vector<ChecksumInfo> &checksum_fields,
                                                const IR::Member *member,
                                                ordered_map<cstring, cstring> &checksum,
                                                gress_t gress) {
        if (checksum_fields.empty()) return;
        P4::CloneExpressions cloner;
        const IR::Expression *constant = nullptr;
        for (auto csum : checksum_fields) {
            if (!csum.parserUpdateLocations.count(gress)) continue;
            std::vector<const IR::Expression *> exprList;
            auto path = BFN::PathLinearizer::convert(csum.destField);
            if (!checksum.count(path))
                checksum.emplace(path, structure->makeUniqueName("checksum"_cs));
            auto fieldList = csum.fieldList;
            if (!fieldList->is<IR::ListExpression>()) continue;
            for (auto f : fieldList->to<IR::ListExpression>()->components) {
                if (f->is<IR::Constant>()) {
                    constant = f;
                } else if (belongsTo(f->to<IR::Member>(), member)) {
                    if (constant) {
                        exprList.emplace_back(constant);
                        constant = nullptr;
                        // If immediate next field after the constant is extracted in this field
                        // then the constant belongs to subtract field list of this state
                    }
                    exprList.emplace_back(f->apply(cloner));

                } else {
                    constant = nullptr;
                }
            }
            for (auto e : exprList) {
                auto mce = new IR::MethodCallExpression(
                    new IR::Member(new IR::PathExpression(checksum.at(path)), "subtract"),
                    new IR::Vector<IR::Type>({e->type}),
                    new IR::Vector<IR::Argument>({new IR::Argument(e)}));
                auto subtractCall = new IR::MethodCallStatement(mce);
                statements->push_back(subtractCall);
            }

            if (belongsTo(csum.destField->to<IR::Member>(), member)) {
                auto destField = csum.destField->apply(cloner);
                auto mce = new IR::MethodCallExpression(
                    new IR::Member(new IR::PathExpression(checksum.at(path)), "subtract"),
                    new IR::Vector<IR::Type>({destField->type}),
                    new IR::Vector<IR::Argument>({new IR::Argument(destField)}));
                auto subtractCall = new IR::MethodCallStatement(mce);
                statements->push_back(subtractCall);

                if (csum.with_payload) {
                    BUG_CHECK(csum.residulChecksumName != std::nullopt,
                              "residual checksum field name cannot be empty");
                    auto *rmember = new IR::Member(
                        IR::Type::Bits::get(16),
                        new IR::Member(new IR::PathExpression("meta"), IR::ID("bridged_header")),
                        IR::ID(*csum.residulChecksumName));
                    auto *mce = new IR::MethodCallExpression(
                        new IR::Member(new IR::PathExpression(checksum.at(path)),
                                       "subtract_all_and_deposit"),
                        new IR::Vector<IR::Type>({rmember->type}),
                        new IR::Vector<IR::Argument>({new IR::Argument(rmember)}));
                    auto *deposit = new IR::MethodCallStatement(mce);
                    std::cout << member->member << std::endl;
                    structure->checksumDepositToHeader[gress][member->member] = deposit;
                }
            }
        }
    }

    // iterate through verify checksum.
    // create add call for the fields that belongs to the provided member;
    void createAddCallsForVerifyChecksum(IR::IndexedVector<IR::StatOrDecl> *statements,
                                         std::vector<ChecksumInfo> &checksum_fields,
                                         const IR::Member *member,
                                         ordered_map<cstring, cstring> &checksum, gress_t gress,
                                         cstring state) {
        P4::CloneExpressions cloner;
        for (auto csum : checksum_fields) {
            if (!csum.parserUpdateLocations.count(gress)) continue;
            auto path = BFN::PathLinearizer::convert(csum.destField);
            if (!checksum.count(path))
                checksum.emplace(path, structure->makeUniqueName("checksum"_cs));
            auto csumInst = checksum.at(path);
            auto fieldList = csum.fieldList;
            if (!fieldList->is<IR::ListExpression>()) continue;
            for (auto f : fieldList->to<IR::ListExpression>()->components) {
                if (belongsTo(f->to<IR::Member>(), member)) {
                    auto mce = new IR::MethodCallExpression(
                        new IR::Member(new IR::PathExpression(checksum.at(path)), "add"),
                        new IR::Vector<IR::Type>({f->type}),
                        new IR::Vector<IR::Argument>({new IR::Argument(f->apply(cloner))}));

                    auto addCall = new IR::MethodCallStatement(mce);
                    statements->push_back(addCall);
                    structure->ingressVerifyChecksumToStates[csumInst].insert(state);
                }
            }

            auto destField = csum.destField->apply(cloner);
            if (belongsTo(destField->to<IR::Member>(), member)) {
                auto mce = new IR::MethodCallExpression(
                    new IR::Member(new IR::PathExpression(checksum.at(path)), "add"),
                    new IR::Vector<IR::Type>({destField->type}),
                    new IR::Vector<IR::Argument>({new IR::Argument(destField->apply(cloner))}));
                auto addCall = new IR::MethodCallStatement(mce);
                statements->push_back(addCall);
                structure->ingressVerifyChecksumToStates[csumInst].insert(state);
            }
        }
    }

    void postorder(IR::ParserState *state) override {
        // IR::IndexedVector<IR::StatOrDecl>* subtractCalls = nullptr;
        // IR::IndexedVector<IR::StatOrDecl>* addCalls = nullptr;
        auto subtractCalls = new IR::IndexedVector<IR::StatOrDecl>();
        auto addCalls = new IR::IndexedVector<IR::StatOrDecl>();
        auto parser = findContext<IR::P4Parser>();
        auto gress = parser->name == "IngressParserImpl" ? INGRESS : EGRESS;
        forAllExtracts(state, [&](const IR::Member *member) {
            createAddCallsForVerifyChecksum(addCalls, structure->verifyChecksums, member, checksum,
                                            gress, state->name);
        });
        forAllExtracts(state, [&](const IR::Member *member) {
            auto rc = structure->residualChecksums;
            createSubtractCallsForResidualChecksum(subtractCalls, rc, member, checksum, gress);
        });
        if (subtractCalls->size() != 0) {
            state->components.append(*subtractCalls);
        }
        if (addCalls->size() != 0) state->components.append(*addCalls);

        if (subtractCalls->size() != 0 || addCalls->size() != 0) {
            auto parser = findContext<IR::P4Parser>();
            for (auto it = checksum.begin(); it != checksum.end(); it++)
                need_checksum[parser->name].insert(it->second);
        }
    }

    bool preorder(IR::P4Parser *) override {
        checksum.clear();
        return true;
    }

    void postorder(IR::P4Parser *parser) override {
        if (need_checksum.count(parser->name)) {
            auto csums = need_checksum.at(parser->name);
            for (auto csum : csums) {
                auto inst = new IR::Type_Name("Checksum");
                auto args = new IR::Vector<IR::Argument>();
                auto decl = new IR::Declaration_Instance(csum, inst, args);
                parser->parserLocals.push_back(decl);
            }
        }
    }

    void postorder(IR::MethodCallExpression *mce) override {
        auto inst = P4::MethodInstance::resolve(mce, refMap, typeMap, nullptr /* ctxt */,
                                                true /* incomplete */);
        if (!inst->is<P4::ExternMethod>()) return;
        auto em = inst->to<P4::ExternMethod>();
        if (em->actualExternType->name != "Checksum" || em->method->name != "update") return;
        auto assign = findOrigCtxt<IR::AssignmentStatement>();
        if (assign == nullptr) return;
        auto destField = assign->left;
        // auto parser = findContext<IR::P4Control>();
        // FIXME: should check parser for null!!
        // auto gress = parser->name == "IngressDeparserImpl" ? INGRESS : EGRESS;
        if (structure->residualChecksums.empty()) return;
        auto path = BFN::PathLinearizer::convert(destField);
        if (!residualChecksumPayloadFields.count(path)) return;
        auto data = inst->substitution.lookupByName("data"_cs);
        P4::CloneExpressions cloner;
        if (auto expr = data->expression->to<IR::ListExpression>()) {
            // add checksum fields
            IR::ListExpression *fl = expr->clone();
            // add payload
            auto payload = residualChecksumPayloadFields.at(path);
            for (auto p : payload) fl->push_back(p->apply(cloner));
            auto args = new IR::Vector<IR::Argument>();
            args->push_back(new IR::Argument(fl));
            mce->arguments = args;
        }
    }
};

using EndStates = std::map<cstring, ordered_map<cstring, ordered_set<cstring>>>;

static IR::AssignmentStatement *createChecksumError(cstring decl, gress_t gress) {
    auto methodCall = new IR::Member(new IR::PathExpression(decl), "verify");
    auto verifyCall = new IR::MethodCallExpression(methodCall, {});
    auto rhs = new IR::Cast(IR::Type::Bits::get(1), verifyCall);

    cstring intr_md;

    if (gress == INGRESS)
        intr_md = "ig_intr_md_from_prsr"_cs;
    else if (gress == EGRESS)
        intr_md = "eg_intr_md_from_prsr"_cs;
    else
        BUG("Unhandled gress: %1%.", gress);

    auto parser_err = new IR::Member(new IR::PathExpression(intr_md), "parser_err");

    auto lhs = new IR::Slice(parser_err, 12, 12);
    return new IR::AssignmentStatement(lhs, rhs);
}

class InsertChecksumDeposit : public Transform {
 public:
    TnaProgramStructure *structure;
    explicit InsertChecksumDeposit(TnaProgramStructure *structure) : structure(structure) {}
    const IR::Node *preorder(IR::ParserState *state) override {
        auto parser = findContext<IR::P4Parser>();
        auto gress = parser->name == "IngressParserImpl" ? INGRESS : EGRESS;
        if (!structure->checksumDepositToHeader.count(gress)) return state;
        auto components = new IR::IndexedVector<IR::StatOrDecl>();
        auto &checksumDeposit = structure->checksumDepositToHeader.at(gress);
        for (auto component : state->components) {
            components->push_back(component);
            if (auto methodCall = component->to<IR::MethodCallStatement>()) {
                if (auto call = methodCall->methodCall->to<IR::MethodCallExpression>()) {
                    if (auto method = call->method->to<IR::Member>()) {
                        if (method->member == "extract") {
                            for (auto arg : *call->arguments) {
                                auto member = arg->expression->to<IR::Member>();
                                if (!member) continue;
                                if (checksumDeposit.count(member->member)) {
                                    components->push_back(checksumDeposit.at(member->member));
                                }
                            }
                        }
                    }
                }
            }
        }
        state->components = *components;
        return state;
    }
};

class InsertChecksumError : public PassManager {
 public:
    EndStates endStates;
    struct ComputeEndStates : public Inspector {
        TnaProgramStructure *structure;
        const P4ParserGraphs *graph;
        EndStates &endStates;
        explicit ComputeEndStates(TnaProgramStructure *structure, const P4ParserGraphs *graph,
                                  EndStates &endStates)
            : structure(structure), graph(graph), endStates(endStates) {}

        void printStates(const ordered_set<cstring> &states) {
            for (auto s : states) std::cout << "   " << s << std::endl;
        }

        ordered_set<cstring> computeChecksumEndStates(const ordered_set<cstring> &calcStates) {
            if (LOGGING(3)) {
                std::cout << "calc states are:" << std::endl;
                printStates(calcStates);
            }

            ordered_set<cstring> endStates;

            // A calculation state is a verification end state if no other state of the
            // same calculation is its descendant. Otherwise, include all of the state's
            // children states that are not a calculation state.

            for (auto a : calcStates) {
                bool isEndState = true;
                for (auto b : calcStates) {
                    if (graph->is_ancestor(a, b)) {
                        isEndState = false;
                        break;
                    }
                }
                if (isEndState) {
                    endStates.insert(a);
                } else {
                    if (graph->succs.count(a)) {
                        for (auto succ : graph->succs.at(a)) {
                            if (calcStates.count(succ)) continue;

                            for (auto s : calcStates) {
                                if (!graph->is_ancestor(succ, s)) {
                                    endStates.insert(succ);
                                }
                            }
                        }
                    }
                }
            }

            BUG_CHECK(!endStates.empty(), "Unable to find verification end state?");

            if (LOGGING(3)) {
                std::cout << "end states are:" << std::endl;
                printStates(endStates);
            }

            return endStates;
        }

        bool preorder(const IR::P4Parser *parser) override {
            if (parser->name != "IngressParserImpl") return false;

            // compute checksum end states
            for (auto kv : structure->ingressVerifyChecksumToStates)
                endStates[parser->name][kv.first] = computeChecksumEndStates(kv.second);

            return false;
        }
    };

    // TODO we probably don't want to insert statement into the "accept" state
    // since this is a special state. Add a dummy state before "accept" if it is
    // a checksum verification end state.
    struct InsertBeforeAccept : public Transform {
        const IR::Node *preorder(IR::P4Parser *parser) override {
            for (auto &kv : endStates[parser->name]) {
                if (kv.second.count("accept"_cs)) {
                    if (!dummy) {
                        dummy =
                            BFN::createGeneratedParserState("before_accept"_cs, {}, "accept"_cs);
                        parser->states.push_back(dummy);
                    }
                    kv.second.erase("accept"_cs);
                    kv.second.insert("__before_accept"_cs);
                    LOG3("add dummy state before \"accept\"");
                }
            }
            return parser;
        }

        const IR::Node *postorder(IR::PathExpression *path) override {
            auto parser = findContext<IR::P4Parser>();
            auto state = findContext<IR::ParserState>();
            auto select = findContext<IR::SelectCase>();

            if (parser && state && select) {
                bool isCalcState = false;

                for (auto kv : structure->ingressVerifyChecksumToStates) {
                    for (auto s : kv.second) {
                        if (s == state->name) {
                            isCalcState = true;
                            break;
                        }
                    }
                }

                if (!isCalcState) return path;

                for (auto &kv : endStates[parser->name]) {
                    if (path->path->name == "accept" && kv.second.count("__before_accept"_cs)) {
                        path = new IR::PathExpression("__before_accept");
                        LOG3("modify transition to \"before_accept\"");
                    }
                }
            }

            return path;
        }

        const IR::ParserState *dummy = nullptr;
        TnaProgramStructure *structure;
        EndStates &endStates;

        explicit InsertBeforeAccept(TnaProgramStructure *structure, EndStates &endStates)
            : structure(structure), endStates(endStates) {
            CHECK_NULL(structure);
        }
    };

    struct InsertEndStates : public Transform {
        const IR::Node *preorder(IR::ParserState *state) override {
            auto parser = findContext<IR::P4Parser>();

            if (state->name == "reject") return state;

            for (auto &kv : endStates[parser->name]) {
                auto decl = kv.first;
                for (auto endState : kv.second) {
                    if (endState == state->name) {
                        auto thread = (parser->name == "IngressParserImpl") ? INGRESS : EGRESS;
                        auto *checksumError = createChecksumError(decl, thread);
                        state->components.push_back(checksumError);

                        LOG3("verify " << toString(parser->name) << " " << decl << " in state "
                                       << endState);
                    }
                }
            }
            return state;
        }
        EndStates &endStates;
        explicit InsertEndStates(EndStates &endStates) : endStates(endStates) {}
    };

    explicit InsertChecksumError(TnaProgramStructure *structure, const P4ParserGraphs *graph) {
        addPasses({new ComputeEndStates(structure, graph, endStates),
                   new InsertBeforeAccept(structure, endStates), new InsertEndStates(endStates)});
    }
};

class FixChecksum : public PassManager {
    P4::ReferenceMap refMap;
    P4::TypeMap typeMap;

 public:
    explicit FixChecksum(TnaProgramStructure *structure);
};

/**
 * Must be applied after FixExtracts, as FixExtracts relies on
 * a map that built during the Rewriter pass.
 */
class FixEgressParserDuplicateReference : public Transform {
    P4::CloneExpressions cloner;

 public:
    FixEgressParserDuplicateReference() {}
    const IR::Node *postorder(IR::ParserState *state) { return state->apply(cloner); }
};

// Misc fixup passes to ensure a valid tna program, specifically, they deal with
// the following difference between a P4-14 program and a P4-16 tna program.
// - tna egress cannot refer to ingress intrinsic metadata
// - no support for standard metadata in tna.
class PostTranslationFix : public PassManager {
 public:
    explicit PostTranslationFix(ProgramStructure *s) {
        auto tnaStruct = dynamic_cast<TnaProgramStructure *>(s);
        if (!tnaStruct) error("Cannot convert to TnaProgramStructure");
        addPasses({
            new FixExtracts(tnaStruct),
            new FixBridgedIntrinsicMetadata(tnaStruct),
            new FixIdleTimeout(),
            new ConvertMetadata(tnaStruct),
            new BFN::ElimUnusedMetadataStates(),
            new FixEgressParserDuplicateReference(),
            new FixParserPriority(),  // FIXME
            new FixParserCounter(),
            new FixDuplicatePathExpression(),
            new FixChecksum(tnaStruct),
        });
    }
};

IR::BlockStatement *generate_hash_block_statement(P4V1::ProgramStructure *structure,
                                                  const IR::Primitive *prim, const cstring temp,
                                                  ExpressionConverter &conv, unsigned num_ops);

void add_custom_enum_if_crc_poly(const IR::Expression *expr, IR::Vector<IR::Argument> &declArgs);

IR::BlockStatement *generate_tna_hash_block_statement(P4V1::TnaProgramStructure *structure,
                                                      const IR::Primitive *prim, const cstring temp,
                                                      ExpressionConverter &conv, unsigned num_ops,
                                                      const IR::Expression *dest);

IR::BlockStatement *generate_arch_neutral_hash_block_statement(P4V1::ProgramStructure *structure,
                                                               const IR::Primitive *prim,
                                                               const cstring temp,
                                                               ExpressionConverter &conv,
                                                               unsigned num_ops);

// used by P4-14 & v1model translation
bool use_v1model();

///////////////////////////////////////////////////////////////
// convert a P4-14 program to an equivalent P4-16 TNA program.
class TnaConverter : public PassManager {
    ProgramStructure *structure;

 public:
    TnaConverter();
    void loadModel() { structure->loadModel(); }
    Visitor::profile_t init_apply(const IR::Node *node) override;
};

}  // namespace P4::P4V1

#endif /* BACKENDS_TOFINO_BF_P4C_ARCH_FROMV1_0_PROGRAMSTRUCTURE_H_ */
