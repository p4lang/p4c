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

#ifndef BF_P4C_MAU_INSTRUCTION_SELECTION_H_
#define BF_P4C_MAU_INSTRUCTION_SELECTION_H_

#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/reduction_or.h"
#include "bf-p4c/phv/phv_fields.h"
#include "ir/ir-generated.h"

using namespace P4;

/**
 * Until the Register.read and Register.write functions have a separate Instruction
 * Selection to convert these to RegisterActions, Brig does not support these read/write
 * calls.  Eventually someone has to write a pass to do this conversion.
 */
class UnimplementedRegisterMethodCalls : public MauInspector {
    bool preorder(const IR::MAU::Primitive *prim) override;

 public:
    UnimplementedRegisterMethodCalls() {}
};

/**
 * This class convert shift expression using concatenation to a funnel-shift instruction. This
 * pass work in tendem with the Midend EliminateWidthCasts one.
 */
class ToFunnelShiftInstruction : public MauTransform {
    const IR::Expression *preorder(IR::Cast *) override;

 public:
    ToFunnelShiftInstruction() {}
};

/**
 * This class convert extern funnel_shift_right method call to funnel-shift instruction.
 */
class ConvertFunnelShiftExtern : public MauTransform {
    const IR::Expression *preorder(IR::Primitive *) override;

 public:
    ConvertFunnelShiftExtern() {}
};

/**
 * The purpose of this class is to convert expressions that require the Hash pathway in actions
 * to IR::MAU::Instructions where the instruction setting tempvars to HashDist objects.  These
 * can later be further converted to the correct instruction in DoInstructionSelection
 */
class HashGenSetup : public PassManager {
    ordered_map<const IR::Expression *, IR::MAU::HashGenExpression *> hash_gen_injections;
    ordered_set<const IR::Expression *> hash_dist_injections;
    const PhvInfo &phv;
    const BFN_Options &options;

    Visitor::profile_t init_apply(const IR::Node *node) override {
        auto rv = PassManager::init_apply(node);
        hash_gen_injections.clear();
        hash_dist_injections.clear();
        return rv;
    }

    /**
     * Find the expressions that are going to be converted HashGenExpressions
     */
    class CreateHashGenExprs : public MauInspector, TofinoWriteContext {
        HashGenSetup &self;
        std::vector<const IR::Expression *> per_prim_hge;
        bool preorder(const IR::MAU::StatefulAlu *) override { return false; }
        bool preorder(const IR::BFN::SignExtend *) override;
        bool preorder(const IR::Concat *) override;
        bool preorder(const IR::Expression *) override;
        bool preorder(const IR::Constant *) override;
        bool preorder(const IR::MAU::Primitive *) override;
        void postorder(const IR::MAU::Primitive *) override;

        void check_for_symmetric(const IR::Declaration_Instance *decl, const IR::ListExpression *le,
                                 IR::MAU::HashFunction hf, LTBitMatrix *sym_keys);
        bool isBetter(const IR::Expression *dest, const IR::Expression *a, const IR::Expression *b);

     public:
        explicit CreateHashGenExprs(HashGenSetup &s) : self(s) {}
    };

    /**
     * Find the expressions that are to be converted to HashDists
     */
    class ScanHashDists : public MauInspector {
        HashGenSetup &self;
        bool preorder(const IR::MAU::StatefulAlu *) override { return false; }
        bool preorder(const IR::Expression *) override;

     public:
        explicit ScanHashDists(HashGenSetup &s) : self(s) {}
    };

    /**
     * Replace these found expressions with said HashGenExpressions and HashDists
     */
    class UpdateHashDists : public MauTransform {
        HashGenSetup &self;
        const IR::Expression *postorder(IR::Expression *) override;

     public:
        explicit UpdateHashDists(HashGenSetup &s) : self(s) {}
    };

 public:
    explicit HashGenSetup(const PhvInfo &p, const BFN_Options &o) : phv(p), options(o) {
        addPasses(
            {new CreateHashGenExprs(*this), new ScanHashDists(*this), new UpdateHashDists(*this)});
    }
};

/** The purpose of this pass is to determine the types, and per flow enables of actions.
 *  Eventually a second inspector will be required, after StatefulAttachmentSetup, in order
 *  to verify that the way that these synth2port tables are being used is correct.
 *
 *  FIXME: In order to fully remain sequential, for addressing being set up from potential
 *  fields, we cannot necessarily remove instructions.  Instead of using Primitive, a new
 *  IR node could be necessary.
 */
class Synth2PortSetup : public MauTransform {
    safe_vector<const IR::MAU::Primitive *> stateful;
    std::set<UniqueAttachedId> per_flow_enables;
    std::map<UniqueAttachedId, IR::MAU::MeterType> meter_types;

    safe_vector<IR::MAU::Instruction *> created_instrs;

    const IR::MAU::Action *preorder(IR::MAU::Action *) override;
    const IR::Node *postorder(IR::MAU::Primitive *) override;
    const IR::MAU::Action *postorder(IR::MAU::Action *) override;

    void clear_action() {
        stateful.clear();
        per_flow_enables.clear();
        meter_types.clear();
    }

 public:
    explicit Synth2PortSetup(const PhvInfo &) {}
};

/** The general purpose of instruction selection is to completely transform all P4 frontend code
 *  to parallel instructions that the compiler can completely understand.  These parallel
 *  instructions are encoded in IR::MAU::Instruction.
 *
 *  Currently the following classes exist to perform the conversion:
 *    - InstructionSelection: generic all-purpose class for handling the complete translation
 *      between frontend IR and backend.
 *    - StatefulAttachmentSetup: specifically to create or possibly link IR::MAU::HashDist units
 *      with their associated IR::MAU::BackendAttached tables, or setup other attachment modes
 *    - ConvertCastsToSlices: Conversion of all IR::Casts to IR::Slices as expressions.
 *      Specifically on assignment, will extend or possibly not extend the value that is being
 *      written
 *    - VerifyActions: Verifies that all generated instructions will be correctly understood
 *      and interpreted by the remainder of the compiler
 */
class DoInstructionSelection : public MauTransform, TofinoWriteContext {
    const PhvInfo &phv;
    IR::MAU::Action *af = nullptr;
    class SplitInstructions;
    int synth_arg_num = 0;
    const IR::MAU::TableSeq *ts = nullptr;
    ordered_map<cstring, const IR::MAU::Instruction *> const_to_phv;

    profile_t init_apply(const IR::Node *root) override;
    const IR::MAU::HashDist *preorder(IR::MAU::HashDist *hd) override {
        prune();
        return hd;
    }
    const IR::GlobalRef *preorder(IR::GlobalRef *) override;
    const IR::MAU::TableSeq *postorder(IR::MAU::TableSeq *) override;
    const IR::MAU::Action *postorder(IR::MAU::Action *) override;
    const IR::MAU::Action *preorder(IR::MAU::Action *) override;
    const IR::MAU::SaluAction *preorder(IR::MAU::SaluAction *a) override {
        prune();
        return a;
    }
    const IR::Expression *postorder(IR::Add *) override;
    const IR::Expression *postorder(IR::AddSat *) override;
    const IR::Expression *postorder(IR::Sub *) override;
    const IR::Expression *postorder(IR::SubSat *) override;
    const IR::Expression *postorder(IR::Neg *) override;
    const IR::Expression *postorder(IR::Shr *) override;
    const IR::Expression *postorder(IR::Shl *) override;
    const IR::Expression *postorder(IR::BAnd *) override;
    const IR::Expression *postorder(IR::BOr *) override;
    const IR::Expression *postorder(IR::BXor *) override;
    const IR::Expression *postorder(IR::Cmpl *) override;
    const IR::Expression *preorder(IR::BFN::SignExtend *) override;
    const IR::Expression *preorder(IR::Concat *) override;
    // const IR::Expression *postorder(IR::Cast *) override;
    const IR::Expression *postorder(IR::Operation_Relation *) override;
    const IR::Expression *postorder(IR::Mux *) override;
    const IR::Slice *postorder(IR::Slice *) override;
    const IR::Expression *postorder(IR::BoolLiteral *) override;
    const IR::Node *postorder(IR::MAU::Primitive *) override;
    const IR::MAU::Instruction *postorder(IR::MAU::Instruction *i) override { return i; }

    bool checkPHV(const IR::Expression *);
    bool checkActionBus(const IR::Expression *e);
    bool checkSrc1(const IR::Expression *);
    bool checkConst(const IR::Expression *ex, long &value);
    bool equiv(const IR::Expression *a, const IR::Expression *b);
    IR::Member *genIntrinsicMetadata(gress_t gress, cstring header, cstring field);

    void limitWidth(const IR::Expression *);

 public:
    explicit DoInstructionSelection(const PhvInfo &phv) : phv(phv) {}
};

/** This pass was specifically created to deal with adding the HashDist object to different
 *  stateful objects.  On one particular case, execute_stateful_alu_from_hash was creating
 *  two separate instructions, a TempVar = hash function call, and an execute stateful call
 *  addressed by this TempVar.  This pass combines these instructions into one instruction,
 *  and correctly saves the HashDist IR into these attached tables
 */
class StatefulAttachmentSetup : public PassManager {
    const PhvInfo &phv;
    const IR::TempVar *saved_tempvar = nullptr;
    const IR::MAU::HashDist *saved_hashdist = nullptr;
    typedef std::pair<const IR::MAU::AttachedMemory *, const IR::MAU::Table *> HashDistKey;
    typedef std::pair<const IR::MAU::StatefulCall *, const IR::MAU::Table *> StatefulCallKey;
    ordered_set<cstring> remove_tempvars;
    ordered_set<cstring> copy_propagated_tempvars;
    ordered_set<const IR::Node *> remove_instr;
    ordered_map<cstring, const IR::MAU::HashDist *> stateful_alu_from_hash_dists;
    ordered_map<HashDistKey, const IR::MAU::HashDist *> update_hd;
    ordered_map<StatefulCallKey, const IR::Expression *> update_calls;
    typedef IR::MAU::StatefulUse use_t;
    ordered_map<const IR::MAU::Action *, ordered_map<const IR::MAU::AttachedMemory *, use_t>>
        action_use;
    typedef std::pair<const IR::MAU::Synth2Port *, const IR::MAU::Table *> IndexCheck;
    struct clear_info_t {
        const IR::MAU::AttachedMemory *attached = 0;
        bitvec clear_value;
        uint32_t busy_value = 0;
    };
    ordered_map<const IR::MAU::AttachedMemory *, clear_info_t> salu_clears;
    ordered_map<const IR::MAU::Table *, clear_info_t *> table_clears;

    ordered_set<IndexCheck> addressed_by_hash;
    ordered_set<IndexCheck> addressed_by_index;

    profile_t init_apply(const IR::Node *root) override {
        remove_tempvars.clear();
        copy_propagated_tempvars.clear();
        remove_instr.clear();
        stateful_alu_from_hash_dists.clear();
        update_hd.clear();
        return PassManager::init_apply(root);
    }
    class Scan : public MauInspector, TofinoWriteContext {
        StatefulAttachmentSetup &self;
        bool preorder(const IR::MAU::Action *) override;
        bool preorder(const IR::MAU::Instruction *) override;
        bool preorder(const IR::TempVar *) override;
        bool preorder(const IR::MAU::HashDist *) override;
        void postorder(const IR::MAU::Instruction *) override;
        void postorder(const IR::MAU::Primitive *) override;
        void postorder(const IR::MAU::Table *) override;
        void setup_index_operand(const IR::Expression *index_expr,
                                 const IR::MAU::Synth2Port *synth2port, const IR::MAU::Table *tbl,
                                 const IR::MAU::StatefulCall *call);
        void simpl_concat(std::vector<const IR::Expression *> &slices, const IR::Concat *concat);

     public:
        explicit Scan(StatefulAttachmentSetup &self) : self(self) {}
    };
    class Update : public MauTransform {
        StatefulAttachmentSetup &self;
        const IR::MAU::StatefulCall *preorder(IR::MAU::StatefulCall *) override;
        const IR::MAU::BackendAttached *preorder(IR::MAU::BackendAttached *ba) override;
        const IR::MAU::StatefulAlu *preorder(IR::MAU::StatefulAlu *salu) override;
        const IR::MAU::Instruction *preorder(IR::MAU::Instruction *sp) override;

     public:
        explicit Update(StatefulAttachmentSetup &self) : self(self) {
            dontForwardChildrenBeforePreorder = true;
        }
    };

    const IR::MAU::HashDist *find_hash_dist(const IR::Expression *expr,
                                            const IR::MAU::Primitive *prim);
    IR::MAU::HashDist *create_hash_dist(const IR::Expression *e, const IR::MAU::Primitive *prim);

 public:
    explicit StatefulAttachmentSetup(const PhvInfo &p) : phv(p) {
        addPasses({new Scan(*this), new Update(*this)});
    }
};

class NullifyAllStatefulCallPrim : public MauModifier {
    bool preorder(IR::MAU::StatefulCall *sc) override;

 public:
    NullifyAllStatefulCallPrim() {}
};

/** Local copy propagates any writes in set operations to later reads in other operations.
 *  Marks these fields as copy propagated.
 */
class BackendCopyPropagation : public MauTransform, TofinoWriteContext {
    const PhvInfo &phv;
    struct FieldImpact {
        le_bitrange dest_bits;
        const IR::Expression *read;

     public:
        FieldImpact(le_bitrange db, const IR::Expression *r) : dest_bits(db), read(r) {}
        const IR::Expression *getSlice(bool isSalu, le_bitrange bits);
    };
    ordered_map<const PHV::Field *, safe_vector<FieldImpact>> copy_propagation_replacements;
    bool elem_copy_propagated = false;
    IR::Vector<IR::MAU::Primitive> *split_set = nullptr;

    const IR::Node *preorder(IR::Node *n) override {
        visitOnce();
        return n;
    }
    const IR::Node *preorder(IR::MAU::Instruction *i) override;
    const IR::MAU::StatefulCall *preorder(IR::MAU::StatefulCall *sc) override {
        prune();
        return sc;
    }
    const IR::MAU::Action *preorder(IR::MAU::Action *a) override;
    const IR::Expression *preorder(IR::Expression *a) override;
    void update(const IR::MAU::Instruction *instr, const IR::Expression *e);
    const IR::Expression *propagate(const IR::MAU::Instruction *instr, const IR::Expression *e);
    const IR::MAU::Action *postorder(IR::MAU::Action *) override;

 public:
    explicit BackendCopyPropagation(const PhvInfo &p) : phv(p) { visitDagOnce = false; }
};

/** Verifies that an action is entirely parallel, after BackendCopyPropagation occurs.
 *  Essentially, if a read in an action has previously been written, and this read has
 *  not been copy propagated, then this is a multi-stage action
 */
class VerifyParallelWritesAndReads : public MauInspector, TofinoWriteContext {
 public:
    typedef ordered_map<const PHV::Field *, safe_vector<le_bitrange>> FieldWrites;

 private:
    const PhvInfo &phv;
    FieldWrites writes;

    bool preorder(const IR::MAU::Action *) override;
    void postorder(const IR::MAU::Instruction *) override;
    bool preorder(const IR::MAU::StatefulCall *) override { return false; }
    bool preorder(const IR::MAU::BackendAttached *) override { return false; }

 public:
    explicit VerifyParallelWritesAndReads(const PhvInfo &p) : phv(p) {}
};

/** Ensures that only the last write to a field is the only instruction kept in a parallel
 *  action, as if the action was sequential, then only the the last write matters.
 */
class EliminateAllButLastWrite : public PassManager {
    const PhvInfo &phv;

 public:
    using LastInstrMap = ordered_map<PHV::FieldSlice, const IR::MAU::Instruction *>;
    ordered_map<const IR::MAU::Action *, LastInstrMap> last_instr_per_action_map;
    // Store field and all its phv bits in current action. Used for handling overlapping set
    ordered_map<const PHV::Field *, safe_vector<le_bitrange>> fs_bits;
    // Need to handle overlapping set if it's set to true.
    ordered_map<const PHV::Field *, bool> has_overlap_set;

 private:
    class Scan : public MauInspector, TofinoWriteContext {
        EliminateAllButLastWrite &self;
        LastInstrMap last_instr_map;

        bool preorder(const IR::MAU::Action *) override;
        bool preorder(const IR::MAU::Instruction *) override;
        bool preorder(const IR::MAU::StatefulCall *) override { return false; }
        bool preorder(const IR::MAU::BackendAttached *) override { return false; }
        void postorder(const IR::MAU::Action *) override;

     public:
        explicit Scan(EliminateAllButLastWrite &self) : self(self) {}
    };

    class Update : public MauTransform, TofinoWriteContext {
        EliminateAllButLastWrite &self;
        const IR::MAU::Action *current_af = nullptr;
        const IR::MAU::Action *preorder(IR::MAU::Action *) override;
        const IR::Node *preorder(IR::MAU::Instruction *) override;
        const IR::MAU::StatefulCall *preorder(IR::MAU::StatefulCall *sc) override {
            prune();
            return sc;
        }
        const IR::MAU::BackendAttached *preorder(IR::MAU::BackendAttached *ba) override {
            prune();
            return ba;
        }

     public:
        explicit Update(EliminateAllButLastWrite &self) : self(self) {}
    };

 public:
    explicit EliminateAllButLastWrite(const PhvInfo &p) : phv(p) {
        addPasses({new Scan(*this), new Update(*this)});
    }
};

/** Applies two transformations to arithmetic compare operations:
 *
 *    1. Narrow write destinations are expanded to match the read field widths.
 *       Currently done by expanding the write field width. This should be
 *       changed in the future to add and write to padding.
 *
 *    2. A compare + set 0 that writes across all bits in a wide write target
 *       are merged into a single compare. (The compare must write to bit 0,
 *       and the set must write to bits width-1 .. 1 .)
 *
 *  These transformations are necessary because of the mismatch between P4
 *  comparison operators and MAU arithmetic compare instructions. P4
 *  comparison operators always produce a 1b boolean value. MAU compare
 *  instructions write to the entire PHV container: the LSB is set to the
 *  comparison result and all other bits are zeroed.
 *
 *  E.g.:
 *    header data_t {
 *        bit<8> f1;
 *        bit<8> f2;
 *        bit<8> f3;
 *    }
 *
 *    struct metadata {
 *        bool is_gt;
 *    };
 *
 *    action gt_action() {
 *        meta.is_gt = hdr.data.f1 > hdr.data.f2;
 *        hdr.data.f3 = (bit<8>)(bit<1>)(hdr.data.f1 > data.data.f2);
 *    }
 *
 *  This pass will:
 *
 *    - Change the type of 'meta.is_gt' from bool to bit<8>. All references to
 *      'meta.is_gt' other than the comparions would be replaced with
 *      'meta.is_gt[0:0]'
 *
 *    - The hdr.data.f3 action is translated to two IR::MAU:Instruction
 *      objects:
 *        set(ingress::hdr.data.f3[7:1], 0);
 *        gtu(ingress::hdr.data.f3[0:0], ingress::hdr.data.f1, ingress::hdr.data.f2);
 *      This is changed to:
 *        gtu(ingress::hdr.data.f3, ingress::hdr.data.f1, ingress::hdr.data.f2);
 *
 *  Details:
 *  This pass has two sub-passes: the first scans the design to find
 *  comparisons to transform, and the second applies the transforms.
 *
 *  Usage:
 *  This pass should be run after ElimAllButLastWrite. This ensures that only
 *  the last write to a location is used when verifying whether the comparison
 *  transforms can be safely applied.
 *
 *  Limitations:
 *   - Currently only works on metadata.
 *
 *  FIXME(glen):
 *    The first transformation currently expands the width of the write field
 *    to match the width of the read fields. This should be changed to add a
 *    pad field and then write to the pad + original field. Unfortunately the
 *    compiler doesn't currently support writing to more than one field with a
 *    single instruction.
 *
 *    MultiOperand seems an appropriate candidate to allow writes to multiple
 *    fields at the same time, but this require updates to the PHV allocation
 *    location logic and the dead code elimination logic.
 */
class ArithCompareAdjustment : public PassManager {
    const PhvInfo &phv;

 public:
    std::map<const PHV::Field *, int> comp_adj_field_width_map;
    std::map<const cstring, int> comp_adj_name_width_map;
    std::map<const IR::MAU::Action *, std::set<const PHV::Field *>> comp_adj_fields_per_action_map;

 private:
    class Scan : public MauInspector {
        ArithCompareAdjustment &self;
        std::map<const PHV::Field *, bitvec> comp_or_set_zero_writes;
        ordered_map<const PHV::Field *, int> comp_targets;
        std::set<const PHV::Field *> other_targets;

        bool preorder(const IR::MAU::Action *) override;
        bool preorder(const IR::MAU::Instruction *) override;
        bool preorder(const IR::MAU::StatefulCall *) override { return false; }
        bool preorder(const IR::MAU::BackendAttached *) override { return false; }
        void postorder(const IR::MAU::Action *) override;

     public:
        explicit Scan(ArithCompareAdjustment &self) : self(self) {}
    };

    class Update : public Transform {
     private:
        ArithCompareAdjustment &self;
        const IR::MAU::Action *current_af = nullptr;
        std::set<const PHV::Field *> comp_adj_fields;

        const IR::MAU::Action *preorder(IR::MAU::Action *) override;
        const IR::MAU::Instruction *preorder(IR::MAU::Instruction *) override;
        const IR::Metadata *preorder(IR::Metadata *) override;
        const IR::ConcreteHeaderRef *preorder(IR::ConcreteHeaderRef *) override;
        const IR::Node *preorder(IR::Member *) override;
        const IR::Type_StructLike *adjust_type_struct(const IR::Type_StructLike *, cstring name);
        const IR::MAU::StatefulCall *preorder(IR::MAU::StatefulCall *sc) override {
            prune();
            return sc;
        }
        const IR::MAU::BackendAttached *preorder(IR::MAU::BackendAttached *ba) override {
            prune();
            return ba;
        }
        const IR::MAU::Action *postorder(IR::MAU::Action *) override;

     public:
        explicit Update(ArithCompareAdjustment &self) : self(self) {}
    };

    bool is_compare(cstring name) {
        return name == "gtequ" || name == "gteqs" || name == "ltu" || name == "lts" ||
               name == "lequ" || name == "leqs" || name == "gtu" || name == "gts" || name == "eq" ||
               name == "neq" || name == "eq64" || name == "neq64";
    }

 public:
    explicit ArithCompareAdjustment(const PhvInfo &p) : phv(p) {
        addPasses({new Scan(*this), new Update(*this)});
    }
};

/** This pass will set up the IR to include any inputs to an LPF/WRED meter, as well
 *  as determine the pre-color for any meter.  The primitive information holds the
 *  field that is either the input/pre-color and in the update, the IR::MAU::Meter is
 *  updated.
 */
class MeterSetup : public PassManager {
    // Tracks the inputs per lpf
    ordered_map<const IR::MAU::Meter *, const IR::Expression *> update_lpfs;
    // Tracks the pre-color per meter
    ordered_map<const IR::MAU::Meter *, const IR::Expression *> update_pre_colors;
    // Marks an action for setting a meter type
    ordered_map<const IR::MAU::Action *, UniqueAttachedId> pre_color_types;
    ordered_map<const IR::MAU::Action *, UniqueAttachedId> standard_types;
    const PhvInfo &phv;

    class Scan : public MauInspector {
        MeterSetup &self;
        void find_input(const IR::MAU::Primitive *);
        void find_pre_color(const IR::MAU::Primitive *);
        const IR::Expression *convert_cast_to_slice(const IR::Expression *);
        profile_t init_apply(const IR::Node *root) override;
        bool preorder(const IR::MAU::Instruction *) override;
        bool preorder(const IR::MAU::Primitive *) override;

     public:
        explicit Scan(MeterSetup &self) : self(self) {}
    };

    class Update : public MauModifier {
        MeterSetup &self;
        void update_input(IR::MAU::Meter *);
        void update_pre_color(IR::MAU::Meter *);
        bool preorder(IR::MAU::Meter *) override;
        bool preorder(IR::MAU::Action *) override;

     public:
        explicit Update(MeterSetup &self) : self(self) {}
    };

 public:
    explicit MeterSetup(const PhvInfo &p) : phv(p) {
        addPasses({new Scan(*this), new Update(*this)});
    }
};

class SetupAttachedAddressing : public PassManager {
    struct AttachedActionCoord {
        bool all_per_flow_enabled = true;
        bool all_same_meter_type = true;
        bool meter_type_set = false;
        IR::MAU::MeterType meter_type = IR::MAU::MeterType::UNUSED;
        const IR::MAU::AttachedMemory *am = nullptr;
    };

    using AttachedInfo = std::map<UniqueAttachedId, AttachedActionCoord>;
    // coordinate the Attached Info across all inspectors
    ordered_map<const IR::MAU::Table *, AttachedInfo> all_attached_info;
    // Just to run the modifier on BackendAttached Objects
    ordered_map<const IR::MAU::BackendAttached *, AttachedActionCoord> attached_coord;

    class InitializeAttachedInfo : public MauInspector {
        SetupAttachedAddressing &self;
        bool preorder(const IR::MAU::BackendAttached *) override;

     public:
        explicit InitializeAttachedInfo(SetupAttachedAddressing &self) : self(self) {}
    };

    class ScanTables : public MauInspector {
        SetupAttachedAddressing &self;
        bool preorder(const IR::MAU::Table *) override;

     public:
        explicit ScanTables(SetupAttachedAddressing &self) : self(self) {}
    };

    class VerifyAttached : public MauInspector {
        SetupAttachedAddressing &self;
        bool preorder(const IR::MAU::BackendAttached *) override;

     public:
        explicit VerifyAttached(SetupAttachedAddressing &self) : self(self) {}
    };

    class UpdateAttached : public MauModifier {
        SetupAttachedAddressing &self;
        void simple_attached(IR::MAU::BackendAttached *);
        bool preorder(IR::MAU::BackendAttached *) override;

     public:
        explicit UpdateAttached(SetupAttachedAddressing &self) : self(self) {}
    };

 public:
    SetupAttachedAddressing() {
        addPasses({new InitializeAttachedInfo(*this), new ScanTables(*this),
                   new VerifyAttached(*this), new UpdateAttached(*this)});
    }
};

class RemoveUnnecessaryActionArgSlice : public MauTransform {
    const IR::Node *preorder(IR::Slice *) override;

 public:
    RemoveUnnecessaryActionArgSlice() {}
};

class GuaranteeHashDistSize : public PassManager {
    ordered_set<const IR::MAU::Instruction *> hash_dist_instrs;

    Visitor::profile_t init_apply(const IR::Node *node) override {
        auto rv = PassManager::init_apply(node);
        hash_dist_instrs.clear();
        return rv;
    }

    class Scan : public MauInspector {
        GuaranteeHashDistSize &self;
        bool contains_hash_dist = false;

        bool preorder(const IR::MAU::BackendAttached *) override { return false; }
        bool preorder(const IR::MAU::StatefulCall *) override { return false; }
        bool preorder(const IR::MAU::Instruction *) override;
        bool preorder(const IR::MAU::HashDist *) override;
        void postorder(const IR::MAU::Instruction *) override;

     public:
        explicit Scan(GuaranteeHashDistSize &s) : self(s) {}
    };

    class Update : public MauTransform {
        GuaranteeHashDistSize &self;

        const IR::Node *postorder(IR::MAU::Instruction *) override;

     public:
        explicit Update(GuaranteeHashDistSize &s) : self(s) {}
    };

 public:
    GuaranteeHashDistSize() { addPasses({new Scan(*this), new Update(*this)}); }
};

class SimplifyConditionalActionArg : public MauTransform {
 public:
    SimplifyConditionalActionArg() {}
    const IR::Node *postorder(IR::Mux *prim) override;
};

class InstructionSelection : public PassManager {
 public:
    InstructionSelection(const BFN_Options &, PhvInfo &, const ReductionOrInfo &);
};

#endif /* BF_P4C_MAU_INSTRUCTION_SELECTION_H_ */
