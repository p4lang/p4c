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

#ifndef BF_P4C_MAU_INSTRUCTION_ADJUSTMENT_H_
#define BF_P4C_MAU_INSTRUCTION_ADJUSTMENT_H_

#include <fstream>

#include "bf-p4c/ir/tofino_write_context.h"
#include "bf-p4c/mau/action_analysis.h"
#include "bf-p4c/mau/mau_visitor.h"
#include "bf-p4c/mau/resource.h"
#include "bf-p4c/mau/tofino/input_xbar.h"
#include "bf-p4c/phv/phv.h"
#include "lib/safe_vector.h"

using namespace P4;

namespace PHV {
class Field;
}  // namespace PHV

/** The purpose of this pass is to adjust shift instruction that only write to a single slice but
 * sourced from multiple containers. This is only seen with signed shifted right operation
 * currently since all of the unsigned shift operation are either sliced or splitted. This pass
 * have to adjust the operation, shift value and slice boundary to match the expected result. For
 * example these P4_14 lines:
 *     header_type my_header_t {
 *         fields {
 *             a: 32;
 *         }
 *     }
 *     header my_header_t my_header;
 *     header_type my_md_t {
 *         fields {
 *             a: 32 (signed);
 *         }
 *    }
 *    ...
 *    modify_field_with_shift(my_header.a, my_md.a, 9, 0xff);
 *
 * Will be translated by the compiler to this operation:
 *    instruction:shrs(ingress::my_header.a[7:0], ingress::my_md.a[7:0], 9);
 *
 * As we can see, this operation does not make any sense but the compiler does not decode the
 * shift value to adjust the slice so the source and destination slice will always be at the same
 * position regardless of the shift value. This pass will translate such instructions using
 * shrs, set or funnel-shift depending on the situation. The example above will get translated to:
 *     instruction:funnel-shift(ingress::my_header.a[7:0], ingress::my_md.a[23:16],
 *                              ingress::my_md.a[15:8], 1)
 */
class AdjustShiftInstructions : public MauTransform, TofinoWriteContext {
    const PhvInfo &phv;

    const IR::Node *preorder(IR::MAU::Instruction *) override;
    // ignore stuff related to stateful alus
    const IR::Node *preorder(IR::MAU::AttachedOutput *ao) override {
        prune();
        return ao;
    }
    const IR::Node *preorder(IR::MAU::StatefulAlu *salu) override {
        prune();
        return salu;
    }
    const IR::Node *preorder(IR::MAU::HashDist *hd) override {
        prune();
        return hd;
    }

 public:
    explicit AdjustShiftInstructions(const PhvInfo &p) : phv(p) {}
};

/** The purpose of these classes is to adjust the instructions in a single action that perform on
 *  multiple containers into one single action for the entire container.  The pass also
 *  verifies that many Tofino specific constraints for the individual ALUs either through the
 *  PHVs being adapted or the action data being manipulated.
 *
 *  The adjustment specificially is from a field to field instruction into a container by
 *  container instruction for the more complex requirements.  This either can break an field
 *  by field instruction into multiple container instructions, performed by SplitInstructions,
 *  or combine them into a single one, performed by MergeInstructions.
 *
 *
 *  For example, the instructions before this pass look like the following:
 *      -set header.field1, header.field2
 *      -set header.field3, header.field4
 *      -set header.field5, action_data_param
 *
 *  In these instructions, there is only one field written to, and up to two fields read from,
 *  in i.e. a bit or an arithmetic operation.  However, if header.field1 and header.field3 were
 *  in the same PHV container, the assembler does not understand multiple instruction calls on
 *  the same container, and would fail.  Thus this pass would remove these instruction, and in
 *  their place put the names of the PHV containers instead, along the lines of:
 *      -set W1(0..23), W5(8..31)
 *
 *  Another major case is the splitting of fields. Say header.field5 is in multiple containers.
 *  The action_data_param may not come into the ALUs contiguously, and thus must be broken into
 *  instruction based on the allocation within the containers:
 *      -set header.field5.0-31 action_data_param.0-31
 *      -set header.field5.32-47 action_data_param.32-47
 *
 *  In particular, some constants have to be converted to action data, based on how they are
 *  use in an instruction within a container.  These constraints are fully detailed by
 *  comments in action_analysis, but are summarized by the restrictions from load const and one
 *  of the sources of an action
 */

/** Responsible for splitting all field instructions over multiple containers into multiple
 *  field instructions over a single container, for example, let's say the program has
 *  the following field instruction:
 *     -set hdr.f1, hdr.f2
 *
 *  where hdr.f1 is in two PHV containers.  This will split this into
 *
 *     -set hdr.f1(A..B), hdr.f2(A..B)
 *     -set hdr.f1(C..D), hdr.f2(C..D)
 *
 *  where A..B and C..D are the write ranges of f1 within its associated container
 */
class SplitInstructions : public MauTransform, TofinoWriteContext {
    const PhvInfo &phv;

    const IR::Node *preorder(IR::MAU::Instruction *) override;
    // ignore stuff related to stateful alus
    const IR::Node *preorder(IR::MAU::AttachedOutput *ao) override {
        prune();
        return ao;
    }
    const IR::Node *preorder(IR::MAU::StatefulAlu *salu) override {
        prune();
        return salu;
    }
    const IR::Node *preorder(IR::MAU::HashDist *hd) override {
        prune();
        return hd;
    }

 public:
    explicit SplitInstructions(const PhvInfo &p) : phv(p) {}
};

/** Responsible for converting IR::Constant to IR::MAU::ActionDataConstants when necessary.
 *  If a constant is not able to be saved within an instruction, this will turn the constant
 *  into an action data slot location
 */
class ConstantsToActionData : public MauTransform, TofinoWriteContext {
    const PhvInfo &phv;
    const ReductionOrInfo &red_info;
    ActionAnalysis::ContainerActionsMap container_actions_map;

    const IR::MAU::Action *preorder(IR::MAU::Action *) override;
    const IR::MAU::Instruction *preorder(IR::MAU::Instruction *) override;
    const IR::MAU::ActionArg *preorder(IR::MAU::ActionArg *) override;
    const IR::Expression *preorder(IR::Expression *) override;
    const IR::Constant *preorder(IR::Constant *) override;
    const IR::Slice *preorder(IR::Slice *) override;
    void analyze_phv_field(IR::Expression *);
    const IR::MAU::Primitive *preorder(IR::MAU::Primitive *) override;
    const IR::MAU::Instruction *postorder(IR::MAU::Instruction *) override;
    const IR::MAU::Action *postorder(IR::MAU::Action *) override;

    const IR::MAU::AttachedOutput *preorder(IR::MAU::AttachedOutput *) override;
    const IR::MAU::StatefulAlu *preorder(IR::MAU::StatefulAlu *) override;
    const IR::MAU::HashDist *preorder(IR::MAU::HashDist *) override;
    const IR::Node *preorder(IR::Node *) override;
    bool has_constant = false;
    bool write_found = false;
    ordered_set<PHV::Container> constant_containers;
    // ActionFormat::ArgKey constant_renames_key;
    ActionData::UniqueLocationKey constant_rename_key;
    cstring action_name;

 public:
    ConstantsToActionData(const PhvInfo &p, const ReductionOrInfo &ri) : phv(p), red_info(ri) {
        visitDagOnce = false;
    }
};

class ExpressionsToHash : public MauTransform {
    const PhvInfo &phv;
    const ReductionOrInfo &red_info;
    ActionAnalysis::ContainerActionsMap container_actions_map;
    ordered_set<PHV::Container> expr_to_hash_containers;

    const IR::MAU::Action *preorder(IR::MAU::Action *) override;
    const IR::MAU::Instruction *preorder(IR::MAU::Instruction *) override;
    const IR::Node *preorder(IR::Node *n) override {
        visitOnce();
        return n;
    }
    const IR::MAU::StatefulAlu *preorder(IR::MAU::StatefulAlu *s) override {
        prune();
        return s;
    }

 public:
    ExpressionsToHash(const PhvInfo &p, const ReductionOrInfo &ri) : phv(p), red_info(ri) {
        visitDagOnce = false;
    }
};

/** Responsible for converting all FieldInstructions within a single Container operation into
 *  one large Container Instruction over IR::MAU::MultiOperands.  Let say the following fields
 *  f1 and f2 are in container B1, while f3 and f4 are in container B2.  The instructions:
 *
 *      -set hdr.f1, hdr.f3
 *      -set hdr.f2, hdr.f4
 *
 *  will get converted to:
 *      -set B1, B2
 *
 *  This will also merge many constants into a single container constant if the action is
 *  possible to do.
 */
class MergeInstructions : public MauTransform, TofinoWriteContext {
 private:
    struct ByteRotateMergeInfo {
        int src1_shift = 0;
        int src2_shift = 0;
        bitvec src1_byte_mask;
    };

    bool saturationArith = false;

    const PhvInfo &phv;
    const ReductionOrInfo &red_info;
    ActionAnalysis::ContainerActionsMap container_actions_map;

    const IR::MAU::Action *preorder(IR::MAU::Action *) override;
    const IR::MAU::Instruction *preorder(IR::MAU::Instruction *) override;
    const IR::Expression *preorder(IR::Expression *) override;
    const IR::Slice *preorder(IR::Slice *) override;
    void analyze_phv_field(IR::Expression *);
    const IR::MAU::ActionDataConstant *preorder(IR::MAU::ActionDataConstant *) override;
    const IR::MAU::ActionArg *preorder(IR::MAU::ActionArg *) override;
    const IR::Constant *preorder(IR::Constant *) override;
    const IR::MAU::Primitive *preorder(IR::MAU::Primitive *) override;

    const IR::MAU::AttachedOutput *preorder(IR::MAU::AttachedOutput *ao) override;
    const IR::MAU::StatefulAlu *preorder(IR::MAU::StatefulAlu *salu) override;
    const IR::MAU::HashDist *preorder(IR::MAU::HashDist *hd) override;

    const IR::Node *preorder(IR::Node *) override;
    const IR::MAU::Instruction *postorder(IR::MAU::Instruction *) override;
    const IR::MAU::Action *postorder(IR::MAU::Action *) override;

    ordered_set<PHV::Container> merged_fields;

    bool write_found = false;
    ordered_set<PHV::Container>::iterator merged_location;

    IR::MAU::Instruction *dest_slice_to_container(PHV::Container container,
                                                  ActionAnalysis::ContainerAction &cont_action);

    void build_actiondata_source(ActionAnalysis::ContainerAction &cont_action,
                                 const IR::Expression **src1_p, bitvec &src1_writebits,
                                 ByteRotateMergeInfo &brm_info, PHV::Container container);
    void build_phv_source(ActionAnalysis::ContainerAction &cont_action,
                          const IR::Expression **src1_p, const IR::Expression **src2_p,
                          bitvec &src1_writebits, bitvec &src2_writebits,
                          ByteRotateMergeInfo &brm_info, PHV::Container container);

    IR::MAU::Instruction *build_merge_instruction(PHV::Container container,
                                                  ActionAnalysis::ContainerAction &cont_action);
    void fill_out_write_multi_operand(ActionAnalysis::ContainerAction &cont_action,
                                      IR::MAU::MultiOperand *mo);
    void fill_out_read_multi_operand(ActionAnalysis::ContainerAction &cont_action,
                                     ActionAnalysis::ActionParam::type_t type, cstring match_name,
                                     IR::MAU::MultiOperand *mo);
    const IR::Expression *fill_out_hash_operand(PHV::Container container,
                                                ActionAnalysis::ContainerAction &cont_action);
    const IR::Expression *fill_out_rand_operand(PHV::Container container,
                                                ActionAnalysis::ContainerAction &cont_action);
    const IR::Constant *find_field_action_constant(ActionAnalysis::ContainerAction &cont_action);

 public:
    MergeInstructions(const PhvInfo &p, const ReductionOrInfo &ri) : phv(p), red_info(ri) {
        visitDagOnce = false;
    }
};

class AdjustStatefulInstructions : public MauTransform {
 private:
    const PhvInfo &phv;
    const IR::Expression *preorder(IR::Expression *expr) override;
    const IR::Annotations *preorder(IR::Annotations *) override;
    const IR::MAU::IXBarExpression *preorder(IR::MAU::IXBarExpression *) override;

    bool check_bit_positions(std::map<int, le_bitrange> &salu_inputs, le_bitrange field_bits,
                             int starting_bit);
    bool verify_on_search_bus(const IR::MAU::StatefulAlu *, const Tofino::IXBar::Use &salu_ixbar,
                              const PHV::Field *field, le_bitrange &bits, bool &is_hi);
    bool verify_on_hash_bus(const IR::MAU::StatefulAlu *salu,
                            const Tofino::IXBar::Use::MeterAluHash &mah, const IR::Expression *expr,
                            le_bitrange &bits, bool &is_hi);

 public:
    explicit AdjustStatefulInstructions(const PhvInfo &p) : phv(p) {}
};

// Remove instructions which are effectively Noops in the code
// This class is when run post phv allocation can determine if an instruction is not performing an
// actual operation e.g. OR A, A, A / AND A, A, A
class EliminateNoopInstructions : public MauTransform {
 private:
    enum OP_TYPE { DST = 0, SRC1 = 1, SRC2 = 2 };
    const PhvInfo &phv;
    typedef std::set<std::pair<PHV::Container, le_bitrange>> AllocContainerSlice;
    bool get_alloc_slice(IR::MAU::Instruction *ins, OP_TYPE type,
                         AllocContainerSlice &op_alloc) const;
    const IR::MAU::Instruction *preorder(IR::MAU::Instruction *) override;
    const IR::MAU::Synth2Port *preorder(IR::MAU::Synth2Port *s) override;
    cstring toString(OP_TYPE ot) const {
        if (ot == DST)
            return "DST"_cs;
        else if (ot == SRC1)
            return "SRC1"_cs;
        else if (ot == SRC2)
            return "SRC2"_cs;
        return "-"_cs;
    }

 public:
    explicit EliminateNoopInstructions(const PhvInfo &p) : phv(p) {}
};

class InstructionAdjustment : public PassManager {
 public:
    InstructionAdjustment(const PhvInfo &p, const ReductionOrInfo &ri);
};

#endif /* BF_P4C_MAU_INSTRUCTION_ADJUSTMENT_H_ */
