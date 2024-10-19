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

#include "collect_hardware_constrained_fields.h"

namespace BFN {

IR::Member *create_member(cstring hdr, cstring member) {
    return new IR::Member(new IR::PathExpression(IR::ID(hdr)), member);
}

void AddHardwareConstrainedFields::postorder(IR::BFN::Pipe *pipe) {
    bool disable_reserved_i2e_drop_implementation = false;
    for (auto anno : pipe->global_pragmas) {
        if (anno->name != PragmaDisableI2EReservedDropImplementation::name) continue;
        disable_reserved_i2e_drop_implementation = true;
    }
    cstring ig_intr_md_for_tm;
    ig_intr_md_for_tm = "ingress::ig_intr_md_for_tm"_cs;
    ordered_map<cstring, IR::BFN::HardwareConstrainedField *> name_to_field;
    name_to_field["mcast_grp_a"_cs] =
        new IR::BFN::HardwareConstrainedField(create_member(ig_intr_md_for_tm, "mcast_grp_a"_cs));
    name_to_field["mcast_grp_b"_cs] =
        new IR::BFN::HardwareConstrainedField(create_member(ig_intr_md_for_tm, "mcast_grp_b"_cs));
    name_to_field["ucast_egress_port"_cs] = new IR::BFN::HardwareConstrainedField(
        create_member(ig_intr_md_for_tm, "ucast_egress_port"_cs));
    name_to_field["level1_mcast_hash"_cs] = new IR::BFN::HardwareConstrainedField(
        create_member(ig_intr_md_for_tm, "level1_mcast_hash"_cs));
    name_to_field["level2_mcast_hash"_cs] = new IR::BFN::HardwareConstrainedField(
        create_member(ig_intr_md_for_tm, "level2_mcast_hash"_cs));
    name_to_field["level1_exclusion_id"_cs] = new IR::BFN::HardwareConstrainedField(
        create_member(ig_intr_md_for_tm, "level1_exclusion_id"_cs));
    name_to_field["level2_exclusion_id"_cs] = new IR::BFN::HardwareConstrainedField(
        create_member(ig_intr_md_for_tm, "level2_exclusion_id"_cs));
    name_to_field["rid"_cs] =
        new IR::BFN::HardwareConstrainedField(create_member(ig_intr_md_for_tm, "rid"_cs));

    cstring ig_intr_md_for_dprsr = "ingress::ig_intr_md_for_dprsr"_cs;
    name_to_field["resubmit_type"_cs] = new IR::BFN::HardwareConstrainedField(
        create_member(ig_intr_md_for_dprsr, "resubmit_type"_cs));
    name_to_field["ig_digest_type"_cs] = new IR::BFN::HardwareConstrainedField(
        create_member(ig_intr_md_for_dprsr, "digest_type"_cs));
    name_to_field["ig_mirror_type"_cs] = new IR::BFN::HardwareConstrainedField(
        create_member(ig_intr_md_for_dprsr, "mirror_type"_cs));
    name_to_field["drop_ctl"_cs] =
        new IR::BFN::HardwareConstrainedField(create_member(ig_intr_md_for_dprsr, "drop_ctl"_cs));
    cstring eg_intr_md_for_dprsr;

    eg_intr_md_for_dprsr = "egress::eg_intr_md_for_dprsr"_cs;

    name_to_field["eg_mirror_type"_cs] = new IR::BFN::HardwareConstrainedField(
        create_member(eg_intr_md_for_dprsr, "mirror_type"_cs));

    cstring eg_intr_md;
    eg_intr_md = "egress::eg_intr_md"_cs;

    name_to_field["egress_port"_cs] =
        new IR::BFN::HardwareConstrainedField(create_member(eg_intr_md, "egress_port"_cs));

    name_to_field["egress_port"_cs]->constraint_type.setbit(
        IR::BFN::HardwareConstrainedField::DEPARSED_BOTTOM_BITS);
    name_to_field["mcast_grp_a"_cs]->constraint_type.setbit(
        IR::BFN::HardwareConstrainedField::DEPARSED_BOTTOM_BITS);
    name_to_field["mcast_grp_b"_cs]->constraint_type.setbit(
        IR::BFN::HardwareConstrainedField::DEPARSED_BOTTOM_BITS);
    name_to_field["ucast_egress_port"_cs]->constraint_type.setbit(
        IR::BFN::HardwareConstrainedField::DEPARSED_BOTTOM_BITS);

    if (Device::currentDevice() == Device::TOFINO) {
        name_to_field["level1_mcast_hash"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::DEPARSED_BOTTOM_BITS);
        name_to_field["level2_mcast_hash"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::DEPARSED_BOTTOM_BITS);
        name_to_field["level1_exclusion_id"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::DEPARSED_BOTTOM_BITS);
        name_to_field["level2_exclusion_id"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::DEPARSED_BOTTOM_BITS);
        name_to_field["rid"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::DEPARSED_BOTTOM_BITS);
        name_to_field["mcast_grp_a"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::INVALIDATE_FROM_ARCH);
        name_to_field["mcast_grp_b"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::INVALIDATE_FROM_ARCH);
        name_to_field["ucast_egress_port"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::INVALIDATE_FROM_ARCH);
        name_to_field["resubmit_type"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::INVALIDATE_FROM_ARCH);
        name_to_field["ig_digest_type"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::INVALIDATE_FROM_ARCH);

        // field that are required to be initialized to zero based on requirement from architecture.
        // ig_intr_md_for_dprsr.mirror_type must be init to zero to workaround ibuf hardware bug.
        // It is validated by default in parser, unless explicitly disabled by the pragma
        // @disable_reserved_i2e_drop_implementation.
        if (disable_reserved_i2e_drop_implementation) {
            name_to_field["ig_mirror_type"_cs]->constraint_type.setbit(
                IR::BFN::HardwareConstrainedField::INVALIDATE_FROM_ARCH);
        }

        name_to_field["eg_mirror_type"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::INVALIDATE_FROM_ARCH);
        name_to_field["drop_ctl"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::INIT_BY_ARCH);
        name_to_field["ig_mirror_type"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::INIT_BY_ARCH);
    }

    // Treat ig_intr_md_for_dprsr.mirror_type the same way in Tofino2
    if (Device::currentDevice() == Device::JBAY) {
        if (disable_reserved_i2e_drop_implementation) {
            name_to_field["ig_mirror_type"_cs]->constraint_type.setbit(
                IR::BFN::HardwareConstrainedField::INVALIDATE_FROM_ARCH);
        }

        name_to_field["ig_mirror_type"_cs]->constraint_type.setbit(
            IR::BFN::HardwareConstrainedField::INIT_BY_ARCH);
    }

    ordered_set<cstring> ingress_fields = {"mcast_grp_a"_cs,         "mcast_grp_b"_cs,
                                           "ucast_egress_port"_cs,   "level1_mcast_hash"_cs,
                                           "level2_mcast_hash"_cs,   "level1_exclusion_id"_cs,
                                           "level2_exclusion_id"_cs, "rid"_cs};
    for (auto name : {"resubmit_type"_cs, "ig_digest_type"_cs, "ig_mirror_type"_cs, "drop_ctl"_cs})
        ingress_fields.insert(name);
    for (auto name : ingress_fields) {
        pipe->thread[INGRESS].hw_constrained_fields.push_back(name_to_field[name]);
    }

    auto egress_fields = {"eg_mirror_type"_cs, "egress_port"_cs};
    for (auto name : egress_fields) {
        pipe->thread[EGRESS].hw_constrained_fields.push_back(name_to_field[name]);
    }
}

}  // namespace BFN
