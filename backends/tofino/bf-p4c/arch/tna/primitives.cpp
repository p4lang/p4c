/**
 * Copyright (C) 2024 Intel Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may not
 * use this file except in compliance with the License.  You may obtain a copy
 * of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software distributed
 * under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations under the License.
 *
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <cmath>

#include "bf-p4c/arch/fromv1.0/programStructure.h"
#include "bf-p4c/arch/intrinsic_metadata.h"
#include "bf-p4c/bf-p4c-options.h"
#include "bf-p4c/device.h"
#include "frontends/p4-14/fromv1.0/converters.h"

// Converters for converting from P4-14 to TNA For a full list of supported
// tofino primitives, see google doc titled: "Supported Tofino Primitives"

namespace P4 {
namespace P4V1 {

// Primitives are ordered alphabetically, if a primitive can reuse the open-source
// implementation, we leave a comment in this file and refer the programmer to
// the implementation in frontends/p4/fromv1.0/programStructure.cpp

// Here is a list of p4-14 primitives that are suppported on tofino. The column
// 'p4c' meaning the primitive is converted with open-source implementation.
// The column 'tna' meaning the primitive is converted in this file.
// +---------------------------------------+-----+-----+
// |               primitives              | p4c | tna |
// +---------------------------------------+-----+-----+
// |   add                                 |  v  |     |
// |   add_header                          |  v  |     |
// |   add_to_field                        |  v  |     |
// |   bit_and                             |  v  |     |
// |   bit_andca                           |  v  |     |
// |   bit_andcb                           |  v  |     |
// |   bit_nand                            |  v  |     |
// |   bit_nor                             |  v  |     |
// |   bit_not                             |  v  |     |
// |   bit_or                              |  v  |     |
// |   bit_orca                            |  v  |     |
// |   bit_orcb                            |  v  |     |
// |   bit_xnor                            |  v  |     |
// |   bit_xor                             |  v  |     |
// |   bypass_egress                       |     |  v  |
// |   clone_ingress_pkt_to_egress         |     |  v  |
// |   clone_egress_pkt_to_egress          |     |  v  |
// |   copy_header                         |  v  |     |
// |   count                               |     |  v  |
// |   count_from_hash                     |     |  v  |
// |   drop                                |     |  v  |
// |   execute_meter                       |     |  v  |
// |   execute_meter_from_hash             |     |  v  |
// |   execute_meter_from_hash_with_or     |     |  v  |
// |   execute_meter_with_or               |     |  v  |
// |   execute_stateful_alu                |    |     |
// |   execute_stateful_alu_from_hash      |    |     |
// |   execute_stateful_log                |    |     |
// |   exit                                |    |     |
// |   funnel_shift_right                  |    |     |
// |   generate_digest                     |    |     |
// |   invalidate                          |    |     |
// |   invalidate_digest                   |    |  v   |
// |   invalidate_clone                    |    |     |
// |   mark_for_drop                       |    |     |
// |   max                                 |    |     |
// |   min                                 |    |     |
// |   modify_field                        |  v  |     |
// |   modify_field_conditionally          |     |     |
// |   modify_field_rng_uniform            |     |  v  |
// |   modify_field_with_hash_based_offset |    |     |
// |   modify_field_with_shift             |    |     |
// |   no_op                               |  v  |     |
// |   pop                                 |  v  |     |
// |   push                                |  v  |     |
// |   recirculate                         |     |  v  |
// |   remove_header                       |  v  |     |
// |   resubmit                            |     |  v  |
// |   sample_e2e                          |     |  v  |
// |   shift_left                          |  v  |     |
// |   shift_right                         |  v  |     |
// |   swap                                |     |  v  |
// |   subtract                            |  v  |     |
// |   subtract_from_field                 |  v  |     |
// +---------------------------------------+-----+-----+

static const std::set<cstring> tna_architectures = {"tna"_cs,  "t2na"_cs, "TNA"_cs,
                                                    "T2NA"_cs, "Tna"_cs,  "T2na"_cs};

static bool using_tna_arch() {
    // Don't assume we have Barefoot-specific options, in case the current compiler context is not
    // a Barefoot-specific one. This occurs when this compilation unit is linked into an
    // executable, but the user elects to use a non-Barefoot backend with non-Barefoot options.
    // In this scenario, this function still gets called from primitive converters that are
    // statically registered by this compilation unit.
    auto &context = P4CContext::get();
    auto *bfn_context = dynamic_cast<BFNContext *>(&context);

    if (bfn_context != nullptr && tna_architectures.count(bfn_context->options().arch) != 0)
        return true;
    return false;
}

CONVERT_PRIMITIVE(bypass_egress, 1) {
    if (!using_tna_arch()) return nullptr;
    if (primitive->operands.size() != 0) return nullptr;
    (void)structure;  // use the parameter to avoid warning
    return BFN::createSetMetadata("ig_intr_md_for_tm"_cs, "bypass_egress"_cs, 1, 1);
}

/*
 * FIXME: duplicate with helper function in TnaProgramStructure
 */
static unsigned computeDigestIndex(TnaProgramStructure *structure, const IR::Primitive *prim,
                                   const IR::Expression *expr) {
    if (!expr) expr = new IR::ListExpression({});
    unsigned index = 0;
    if (prim->name == "resubmit") {
        index = computeHashOverFieldList(expr, structure->resubmitIndexHashes);
    }
    if (prim->name == "clone_ingress_pkt_to_egress") {
        index = computeHashOverFieldList(expr, structure->cloneIndexHashes[INGRESS], true);
    }
    if (prim->name == "clone_egress_pkt_to_egress") {
        index = computeHashOverFieldList(expr, structure->cloneIndexHashes[EGRESS]);
    }
    if (prim->name == "generate_digest") {
        index = computeHashOverFieldList(expr, structure->digestIndexHashes);
    }
    if (prim->name == "recirculate") {
        index = computeHashOverFieldList(expr, structure->recirculateIndexHashes);
    }
    return index;
}

/*
 * FIXME: duplicate with helper function in CollectDigestFields
 */
static IR::Expression *flatten(const IR::ListExpression *args) {
    IR::Vector<IR::Expression> components;
    for (const auto *expr : args->components) {
        if (const auto *list_arg = expr->to<IR::ListExpression>()) {
            auto *flattened = flatten(list_arg);
            BUG_CHECK(flattened->is<IR::ListExpression>(), "flatten must return ListExpression");
            for (const auto *comp : flattened->to<IR::ListExpression>()->components)
                components.push_back(comp);
        } else {
            components.push_back(expr);
        }
    }
    return new IR::ListExpression(components);
}

static const IR::Statement *convertClone(ProgramStructure *structure,
                                         const IR::Primitive *primitive, gress_t gress,
                                         bool /* reserve_entry_zero */ = false) {
    BUG_CHECK(primitive->operands.size() == 1 || primitive->operands.size() == 2,
              "Expected 1 or 2 operands for %1%", primitive);
    ExpressionConverter conv(structure);

    auto *deparserMetadataPath = new IR::PathExpression(
        (gress == INGRESS) ? "ig_intr_md_for_dprsr" : "eg_intr_md_for_dprsr");

    const IR::Expression *list = nullptr;
    if (primitive->operands.size() == 2) {
        list = structure->convertFieldList(primitive->operands.at(1));
        // flatten nested field_list in p4-14 to one-level list expression in P4-16
        if (auto liste = list->to<IR::ListExpression>()) list = flatten(liste);
    }

    auto *block = new IR::BlockStatement;

    auto st = dynamic_cast<TnaProgramStructure *>(structure);
    if (!st) return nullptr;
    auto index = computeDigestIndex(st, primitive, list);
    auto *rhs = new IR::Constant(index);
    // Set `mirror_type`, which is used as the digest selector in the
    // deparser (in other words, it selects the field list to use).
    auto *mirrorIdx = new IR::Member(deparserMetadataPath, "mirror_type");
    block->components.push_back(new IR::AssignmentStatement(mirrorIdx, rhs));

    auto *compilerMetadataPath = new IR::Member(new IR::PathExpression("meta"), COMPILER_META);
    auto *mirrorId = new IR::Member(compilerMetadataPath, "mirror_id");
    auto *mirrorIdValue = conv.convert(primitive->operands.at(0));
    /// p4-14 mirror_id is 32bit, cast to bit<10>
    auto *castedMirrorIdValue =
        new IR::Cast(IR::Type::Bits::get(Device::cloneSessionIdWidth()), mirrorIdValue);
    block->components.push_back(new IR::AssignmentStatement(mirrorId, castedMirrorIdValue));

    // Construct a value for `mirror_source`, which is
    // compiler-generated metadata that's prepended to the user field
    // list. Its layout (in network order) is:
    //   [  0    1       2          3         4       5    6   7 ]
    //     [unused] [coalesced?] [gress] [mirrored?] [mirror_type]
    // Here `gress` is 0 for I2E mirroring and 1 for E2E mirroring.
    //
    // This information is used to set intrinsic metadata in the egress
    // parser. The `mirrored?` bit is particularly important; if that
    // bit is zero, the egress parser expects the following bytes to be
    // bridged metadata rather than mirrored fields.
    //
    // TODO: Glass is able to reuse `mirror_type` for last three
    // bits of this data, which eliminates the need for an extra PHV
    // container. We'll start doing that soon as well, but we need to
    // work out some issues with PHV allocation constraints first.
    const unsigned isMirroredTag = 1 << 3;
    const unsigned gressTag = (gress == INGRESS) ? 0 : 1 << 4;
    unsigned mirror_source = index | isMirroredTag | gressTag;
    block->components.push_back(
        BFN::createSetMetadata("meta"_cs, COMPILER_META, "mirror_source"_cs, 8, mirror_source));

    return block;
}

CONVERT_PRIMITIVE(clone_egress_pkt_to_egress, 1) {
    return convertClone(structure, primitive, EGRESS);
}
CONVERT_PRIMITIVE(clone_e2e, 1) { return convertClone(structure, primitive, EGRESS); }
CONVERT_PRIMITIVE(clone_ingress_pkt_to_egress, 1) {
    return convertClone(structure, primitive, INGRESS);
}
CONVERT_PRIMITIVE(clone_i2e, 1) { return convertClone(structure, primitive, INGRESS); }

static const IR::Statement *convertResubmit(ProgramStructure *structure,
                                            const IR::Primitive *primitive) {
    BUG_CHECK(primitive->operands.size() <= 1, "Expected 0 or 1 operands for %1%", primitive);
    ExpressionConverter conv(structure);

    auto *deparserMetadataPath = new IR::PathExpression("ig_intr_md_for_dprsr");

    const IR::Expression *list = nullptr;
    if (primitive->operands.size() == 1) {
        list = structure->convertFieldList(primitive->operands.at(0));
        if (auto liste = list->to<IR::ListExpression>()) list = flatten(liste);
    }
    auto *block = new IR::BlockStatement;

    auto st = dynamic_cast<TnaProgramStructure *>(structure);
    if (!st) return nullptr;
    auto index = computeDigestIndex(st, primitive, list);
    auto *rhs = new IR::Constant(IR::Type::Bits::get(3), index);
    // Set `resubmit_type`, which is used as the digest selector in the
    // deparser (in other words, it selects the field list to use).
    auto *resubmitIdx = new IR::Member(deparserMetadataPath, "resubmit_type");
    block->components.push_back(new IR::AssignmentStatement(resubmitIdx, rhs));

    // Construct a value for `resubmit_source`, which is
    // compiler-generated metadata that's prepended to the user field
    // list. Its layout (in network order) is:
    //   [  0    1       2          3         4       5     6    7 ]
    //     [               unused             ]      [resubmit_type]
    //
    // TODO: reuse the PHV container for `resubmit_type`
    unsigned resubmit_source = index;
    block->components.push_back(
        BFN::createSetMetadata("meta"_cs, COMPILER_META, "resubmit_source"_cs, 8, resubmit_source));

    return block;
}

CONVERT_PRIMITIVE(resubmit, 1) {
    if (!using_tna_arch()) return nullptr;
    return convertResubmit(structure, primitive);
}

static const IR::Statement *convertDrop(ProgramStructure *structure, const IR::Primitive *) {
    // walk all controls.
    // walk all tables.
    // walk all actions.
    // walk all drop.
    auto st = dynamic_cast<TnaProgramStructure *>(structure);
    if (!st) return nullptr;
    cstring meta =
        (st->currentGress == INGRESS) ? "ig_intr_md_for_dprsr"_cs : "eg_intr_md_for_dprsr"_cs;
    BUG_CHECK(st != nullptr, "Cannot cast structure to TnaProgramStructure");
    auto path = new IR::Member(new IR::PathExpression(meta), "drop_ctl");
    auto val = new IR::Constant(IR::Type::Bits::get(3), 1);
    auto stmt = new IR::AssignmentStatement(path, val);
    return stmt;
}

CONVERT_PRIMITIVE(drop, 1) {
    if (!using_tna_arch()) return nullptr;
    return convertDrop(structure, primitive);
}

CONVERT_PRIMITIVE(invalidate, 1) {
    if (!using_tna_arch()) return nullptr;
    if (primitive->operands.size() != 1) return nullptr;
    ExpressionConverter conv(structure);
    auto arg = conv.convert(primitive->operands.at(0));
    if (arg->is<IR::Constant>()) error("Expected a field reference %1%", arg);
    return new IR::MethodCallStatement(
        primitive->srcInfo, IR::ID(primitive->srcInfo, "invalidate"_cs), {new IR::Argument(arg)});
}

CONVERT_PRIMITIVE(invalidate_digest, 1) {
    if (!using_tna_arch()) return nullptr;
    (void)structure;  // use the parameter to avoid warning
    IR::PathExpression *path = new IR::PathExpression("ig_intr_md_for_dprsr");
    auto mem = new IR::Member(path, "digest_type");
    auto stmt = new IR::MethodCallStatement(primitive->srcInfo, IR::ID("invalidate"_cs),
                                            {new IR::Argument(mem)});
    return stmt;
}

CONVERT_PRIMITIVE(mark_to_drop, 1) {
    if (!using_tna_arch()) return nullptr;
    return convertDrop(structure, primitive);
}

CONVERT_PRIMITIVE(mark_for_drop, 1) {
    if (!using_tna_arch()) return nullptr;
    return convertDrop(structure, primitive);
}

static bool makeTnaMeterExecCall(const Util::SourceInfo &srcInfo, ProgramStructure *structure,
                                 IR::BlockStatement *block, const IR::Expression *mref,
                                 const IR::Expression *index, const IR::Expression *dest,
                                 const IR::Expression *color = nullptr) {
    const IR::Meter *meter = nullptr;
    if (auto gr = mref->to<IR::GlobalRef>())
        meter = gr->obj->to<IR::Meter>();
    else if (auto nr = mref->to<IR::PathExpression>())
        meter = structure->meters.get(nr->path->name);
    if (meter == nullptr) {
        error("Expected a meter reference %1%", mref);
        return false;
    }
    auto meterref = new IR::PathExpression(structure->meters.get(meter));
    auto method = new IR::Member(meterref, "execute");
    auto arg = new IR::Cast(IR::Type::Bits::get(meter->index_width()), index);
    auto args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(arg));
    if (color) {
        if (color->type->is<IR::Type_Bits>()) {
            if (color->type->width_bits() < 8) {
                color = new IR::Cast(IR::Type::Bits::get(8), color);
            } else if (color->type->width_bits() > 8) {
                color = new IR::Slice(color, 7, 0);
            }
            color = new IR::Cast(new IR::Type_Name("MeterColor_t"), color);
        }
        args->push_back(new IR::Argument(color));
    }
    IR::Expression *expr =
        new IR::MethodCallExpression(srcInfo, IR::Type::Bits::get(8), method, args);
    block->push_back(
        structure->assign(srcInfo, dest, expr, IR::Type::Bits::get(dest->type->width_bits())));
    return true;
}

CONVERT_PRIMITIVE(count_from_hash, 2) {
    if (!using_tna_arch()) return nullptr;
    if (primitive->operands.size() != 2) return nullptr;
    ExpressionConverter conv(structure);
    auto ref = primitive->operands.at(0);
    const IR::Counter *counter = nullptr;
    if (auto gr = ref->to<IR::GlobalRef>())
        counter = gr->obj->to<IR::Counter>();
    else if (auto nr = ref->to<IR::PathExpression>())
        counter = structure->counters.get(nr->path->name);
    if (counter == nullptr) {
        error("Expected a counter reference %1%", ref);
        return nullptr;
    }
    cstring temp = structure->makeUniqueName("temp"_cs);
    auto block = P4V1::generate_hash_block_statement(structure, primitive, temp, conv, 2);
    auto counterref = new IR::PathExpression(structure->counters.get(counter));
    auto method = new IR::Member(counterref, structure->v1model.counter.increment.Id());
    auto arg = new IR::Cast(IR::Type::Bits::get(counter->index_width()),
                            new IR::PathExpression(new IR::Path(temp)));
    block->push_back(
        new IR::MethodCallStatement(primitive->srcInfo, method, {new IR::Argument(arg)}));
    return block;
}

CONVERT_PRIMITIVE(execute_meter, 6) {
    if (!using_tna_arch()) return nullptr;
    ExpressionConverter conv(structure);
    auto block = new IR::BlockStatement;
    LOG3("convert meter primitive " << primitive);
    if (primitive->operands.size() == 3) {
        if (!makeTnaMeterExecCall(
                primitive->srcInfo, structure, block, conv.convert(primitive->operands.at(0)),
                conv.convert(primitive->operands.at(1)), conv.convert(primitive->operands.at(2)))) {
            return nullptr;
        }
    } else if (primitive->operands.size() == 4) {
        if (!makeTnaMeterExecCall(
                primitive->srcInfo, structure, block, conv.convert(primitive->operands.at(0)),
                conv.convert(primitive->operands.at(1)), conv.convert(primitive->operands.at(2)),
                conv.convert(primitive->operands.at(3)))) {
            return nullptr;
        }
    }
    return block;
}

// used by p4-14 to tna conversion
CONVERT_PRIMITIVE(modify_field_with_hash_based_offset, 2) {
    if (!using_tna_arch()) return nullptr;
    auto st = dynamic_cast<P4V1::TnaProgramStructure *>(structure);
    ExpressionConverter conv(st);
    auto dest = conv.convert(primitive->operands.at(0));
    return generate_tna_hash_block_statement(st, primitive, ""_cs, conv, 4, dest);
}

// used by p4-14 to tna conversion
CONVERT_PRIMITIVE(modify_field_rng_uniform, 2) {
    if (!using_tna_arch()) return nullptr;
    ExpressionConverter conv(structure);
    BUG_CHECK(primitive->operands.size() == 3, "Expected 3 operands for %1%", primitive);
    auto field = conv.convert(primitive->operands.at(0));
    auto lo = conv.convert(primitive->operands.at(1));
    auto hi = conv.convert(primitive->operands.at(2));
    auto max_value = hi->to<IR::Constant>()->value;
    auto min_value = lo->to<IR::Constant>()->value;
    if (lo->type != field->type) lo = new IR::Cast(primitive->srcInfo, field->type, lo);
    if (hi->type != field->type) hi = new IR::Cast(primitive->srcInfo, field->type, hi);

    auto typeArgs = new IR::Vector<IR::Type>({field->type});
    auto type = new IR::Type_Specialized(new IR::Type_Name("Random"), typeArgs);
    auto randName = structure->makeUniqueName("random"_cs);

    // check hi bound must be 2**W-1
    if (max_value > LONG_MAX)
        error("The random declaration %s max size %d is too large to be supported", randName,
              max_value);
    bool isPowerOfTwoMinusOne = false;
    isPowerOfTwoMinusOne = ((max_value & (max_value + 1)) == 0);
    if (!isPowerOfTwoMinusOne)
        error("The random declaration %s max size must be a power of two minus one", randName);
    // check lo bound must be zero
    if (min_value != 0) error("The random declaration %s min size must be zero", randName);
    auto args = new IR::Vector<IR::Argument>();

    auto method = new IR::PathExpression(randName);
    auto member = new IR::Member(method, "get");
    auto call = new IR::MethodCallExpression(primitive->srcInfo, member, args);

    auto block = new IR::BlockStatement;
    structure->localInstances.push_back(new IR::Declaration_Instance(randName, type, args));
    block->push_back(new IR::MethodCallStatement(primitive->srcInfo, call));
    return block;
}

const IR::BlockStatement *generate_recirc_port_assignment(const IR::Expression *port) {
    auto egress_spec =
        new IR::Member(new IR::PathExpression("ig_intr_md_for_tm"), "ucast_egress_port");
    auto ingress_port = new IR::Member(new IR::PathExpression("ig_intr_md"), "ingress_port");
    auto block = new IR::BlockStatement;
    block->push_back(
        new IR::AssignmentStatement(new IR::Slice(egress_spec, 6, 0), new IR::Slice(port, 6, 0)));
    block->push_back(new IR::AssignmentStatement(new IR::Slice(egress_spec, 8, 7),
                                                 new IR::Slice(ingress_port, 8, 7)));
    return block;
}

// used by p4-14 to tna conversion,
CONVERT_PRIMITIVE(recirculate, 8) {
    if (!using_tna_arch()) return nullptr;
    if (primitive->operands.size() != 1) return nullptr;
    ExpressionConverter conv(structure);
    auto operand = primitive->operands.at(0);
    if (operand->is<IR::Constant>() || operand->is<IR::ActionArg>()) {
        auto port = conv.convert(operand);
        port = new IR::Cast(IR::Type::Bits::get(9), port);
        return generate_recirc_port_assignment(port);
    } else if (auto path = operand->to<IR::PathExpression>()) {
        auto fl = structure->field_lists.get(path->path->name);
        if (fl == nullptr) return nullptr;
        auto right = structure->convertFieldList(operand);
        if (right == nullptr) return nullptr;
        computeDigestIndex(dynamic_cast<TnaProgramStructure *>(structure), primitive, right);
        auto block = new IR::BlockStatement;
        block->push_back(
            generate_recirc_port_assignment(new IR::Constant(IR::Type::Bits::get(9), 68)));
        // FIXME: init recirculated_header
        return block;
    }
    return nullptr;
}

CONVERT_PRIMITIVE(swap, 1) {
    if (!using_tna_arch()) return nullptr;
    if (primitive->operands.size() != 2) return nullptr;
    ExpressionConverter conv(structure);
    auto temp = IR::ID(structure->makeUniqueName("temp"_cs));
    auto v1 = primitive->operands.at(0);
    auto v2 = primitive->operands.at(1);
    auto type = v1->type;
    return new IR::BlockStatement(
        {new IR::Declaration_Variable(temp, type, conv.convert(v1)),
         structure->assign(primitive->srcInfo, conv.convert(v1), conv.convert(v2), type),
         structure->assign(primitive->srcInfo, conv.convert(v2), new IR::PathExpression(temp),
                           type)});
}

CONVERT_PRIMITIVE(execute_meter_with_or, 1) {
    if (!using_tna_arch()) return nullptr;
    if (primitive->operands.size() != 3 && primitive->operands.size() != 4) return nullptr;
    ExpressionConverter conv(structure);
    auto block = new IR::BlockStatement;
    auto dest = conv.convert(primitive->operands.at(2));
    cstring temp2 = structure->makeUniqueName("temp"_cs);
    block->push_back(new IR::Declaration_Variable(temp2, dest->type));
    if (primitive->operands.size() == 3) {
        if (!makeTnaMeterExecCall(primitive->srcInfo, structure, block,
                                  conv.convert(primitive->operands.at(0)),
                                  conv.convert(primitive->operands.at(1)),
                                  new IR::PathExpression(dest->type, new IR::Path(temp2))))
            return nullptr;
    } else {
        // has pre-color, so need TNA specific call
        block->push_back(new IR::MethodCallStatement(
            primitive->srcInfo, IR::ID(primitive->srcInfo, "execute_meter_with_color"_cs),
            {new IR::Argument(conv.convert(primitive->operands.at(0))),
             new IR::Argument(conv.convert(primitive->operands.at(1))),
             new IR::Argument(new IR::PathExpression(new IR::Path(temp2))),
             new IR::Argument(conv.convert(primitive->operands.at(3)))}));
    }
    block->push_back(new IR::AssignmentStatement(
        dest, new IR::BOr(dest, new IR::PathExpression(new IR::Path(temp2)))));
    return block;
}

CONVERT_PRIMITIVE(execute_meter_from_hash, 1) {
    if (!using_tna_arch()) return nullptr;
    if (primitive->operands.size() != 3 && primitive->operands.size() != 4) return nullptr;
    ExpressionConverter conv(structure);
    auto block = new IR::BlockStatement;
    cstring temp = structure->makeUniqueName("temp"_cs);
    block = P4V1::generate_hash_block_statement(structure, primitive, temp, conv, 3);
    if (primitive->operands.size() == 3) {
        if (!makeTnaMeterExecCall(primitive->srcInfo, structure, block,
                                  conv.convert(primitive->operands.at(0)),
                                  new IR::PathExpression(new IR::Path(temp)),
                                  conv.convert(primitive->operands.at(2))))
            return nullptr;
    } else {
        // has pre-color, so need TNA specific call
        block->push_back(new IR::MethodCallStatement(
            primitive->srcInfo, IR::ID(primitive->srcInfo, "execute_meter_with_color"_cs),
            {new IR::Argument(conv.convert(primitive->operands.at(0))),
             new IR::Argument(new IR::PathExpression(new IR::Path(temp))),
             new IR::Argument(conv.convert(primitive->operands.at(2))),
             new IR::Argument(conv.convert(primitive->operands.at(3)))}));
    }
    return block;
}

CONVERT_PRIMITIVE(execute_meter_from_hash_with_or, 1) {
    if (!using_tna_arch()) return nullptr;
    if (primitive->operands.size() != 3 && primitive->operands.size() != 4) return nullptr;
    ExpressionConverter conv(structure);
    cstring temp = structure->makeUniqueName("idx"_cs);
    auto block = P4V1::generate_hash_block_statement(structure, primitive, temp, conv, 3);
    auto dest = conv.convert(primitive->operands.at(2));
    cstring temp2 = structure->makeUniqueName("temp"_cs);
    block->push_back(new IR::Declaration_Variable(temp2, dest->type));
    if (primitive->operands.size() == 3) {
        if (!makeTnaMeterExecCall(primitive->srcInfo, structure, block,
                                  conv.convert(primitive->operands.at(0)),
                                  new IR::PathExpression(temp),
                                  new IR::PathExpression(dest->type, new IR::Path(temp2)))) {
            return nullptr;
        }
    } else {
        // has pre-color, so need TNA specific call
        block->push_back(new IR::MethodCallStatement(
            primitive->srcInfo, IR::ID(primitive->srcInfo, "execute_meter_with_color"_cs),
            {new IR::Argument(conv.convert(primitive->operands.at(0))),
             new IR::Argument(new IR::PathExpression(new IR::Path(temp))),
             new IR::Argument(new IR::PathExpression(new IR::Path(temp2))),
             new IR::Argument(conv.convert(primitive->operands.at(3)))}));
    }
    block->push_back(new IR::AssignmentStatement(
        dest, new IR::BOr(dest, new IR::PathExpression(new IR::Path(temp2)))));
    return block;
}

CONVERT_PRIMITIVE(sample_e2e, 1) {
    /* -- avoid compiler warnings */
    (void)structure;
    (void)primitive;

    if (using_tna_arch()) {
        error(ErrorType::ERR_UNSUPPORTED,
              "sample_e2e primitive is not currently supported by the TNA architecture");
    }
    return nullptr;
}

}  // namespace P4V1
}  // namespace P4
