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

#include "bf-p4c/phv/pragma/pa_byte_pack.h"

#include <sstream>

#include <boost/range/adaptor/reversed.hpp>

#include "bf-p4c/ir/bitrange.h"
#include "bf-p4c/phv/phv_fields.h"
#include "bf-p4c/phv/pragma/phv_pragmas.h"
#include "lib/source_file.h"

/// BFN::Pragma interface
const char *PragmaBytePack::name = "pa_byte_pack";
const char *PragmaBytePack::description =
    "Force PHV allocation to allocate metadata fields in the specified layout.";
const char *PragmaBytePack::help = R"(@pragma pa_byte_pack [pipe] gress ["field_name"|integer]+,
where field names refer to metadata or pov fields, and integers represent the number of bits as padding.
+ attached to P4 header instances.

For example, assume that f1 is 4-bit, f2 is 6-bit and f3 is 3-bit, and we have
@pa_byte_pack("ingress", 2, "f1", 1, "f2", "f3").
Then PHV allocation must allocate following 2 bytes to byte-aligned positions of containers.
byte1 = f2[4:0] ++ f3;
byte2 = 2w0 ++ f1 ++ 1w0 ++ f2[0:0];
One way to understand this pramga is to think of it as `making a pseudo header of metadata`, and
this header only introduces co-pack constraints within bytes, i.e., relative order between bytes
is not constrainted. For example, assume that byte1 and byte2 are allocated to one 16-bit
container H1, both H1 = byte1 ++ byte2 and H1 = byte2 ++ byte1 are valid.

This pragma is useful in tuning table placement and power consumption, by proposing
optimal packing layouts of metadata fields which are used as match keys.

*Constraints*
1. The total number of bits in all fields and padding must be divisible by 8.
2. Each padding element must be greater than 0 and less than 8 bits.
3. Fields used in this pragma must be metadata fields.
4. If a metadata or pov field is written in the parser, then the parser will also write to other
   fields packed in the same byte as the target field. Users of this pragma must either:
     <1> Never pack two parsed fields in the same byte.
     <2> Explicitly initialize non-parsed fields after the parser,
         even if @auto-init-metadata is enabled.
5. Compilation will fail if the pramga cannot be satisfied.)";

bool PragmaBytePack::preorder(const IR::BFN::Pipe *pipe) {
    auto global_pragmas = pipe->global_pragmas;

    for (const auto *annotation : global_pragmas) {
        if (annotation->name.name != PragmaBytePack::name) continue;

        auto &exprs = annotation->expr;

        const unsigned min_required_arguments = 2;  // gress, field1....
        unsigned required_arguments = min_required_arguments;
        unsigned expr_index = 0;
        const IR::StringLiteral *pipe_arg = nullptr;
        const IR::StringLiteral *gress_arg = nullptr;

        if (!PHV::Pragmas::determinePipeGressArgs(exprs, expr_index, required_arguments, pipe_arg,
                                                  gress_arg)) {
            continue;
        }

        if (!PHV::Pragmas::checkNumberArgs(annotation, required_arguments, min_required_arguments,
                                           false, cstring(PragmaBytePack::name),
                                           "gress, [\"field_name\"|integer]+"_cs)) {
            continue;
        }

        if (!PHV::Pragmas::checkPipeApplication(annotation, pipe, pipe_arg)) {
            continue;
        }

        // Extract the rest of the arguments
        bool ignore = false;
        int n_bits = 0;
        PackConstraint pack;
        pack.compiler_added = false;
        pack.src_info = annotation->getSourceInfo();
        gress_arg->value >> pack.packing.gress;
        for (; expr_index < exprs.size(); ++expr_index) {
            if (const auto *field_ir = exprs[expr_index]->to<IR::StringLiteral>()) {
                cstring field_name = gress_arg->value + "::"_cs + field_ir->value;
                const auto *field = phv_i.field(field_name);
                if (!field) {
                    PHV::Pragmas::reportNoMatchingPHV(pipe, field_ir, field_name);
                    ignore = true;
                    break;
                }
                if (!field->metadata && !field->pov) {
                    error(
                        "@pa_byte_pack pragma can only be applied on metadata or pov fields, "
                        "but %1% is not.",
                        field->name);
                    ignore = true;
                    break;
                }
                if (field->parsed()) {
                    warning(
                        "Applying @pa_byte_pack on parsed field %1%. Please make sure that other "
                        "fields in the same byte will be explicitly initialized after parser.",
                        field->name);
                }
                pack.packing.layout.push_back(PHV::PackingLayout::FieldRangeOrPadding(
                    {field, le_bitrange(StartLen(0, field->size))}));
            } else if (const auto *padding_int = exprs[expr_index]->to<IR::Constant>()) {
                int n_bits = padding_int->asInt();
                if (n_bits <= 0 || n_bits >= 8) {
                    error("Invalid size of padding in @pa_byte_pack pragma: %1%", n_bits);
                    ignore = true;
                    break;
                }
                pack.packing.layout.push_back(PHV::PackingLayout::FieldRangeOrPadding(n_bits));
            } else {
                error("Invalid parameter in @pa_byte_pack pragma: %1%",
                      exprs[expr_index]->toString());
                ignore = true;
                break;
            }
            n_bits += pack.packing.layout.back().size();
        }

        if (!ignore) {
            auto rst = add_packing_constraint(pack);
            if (!rst.ok()) {
                error(cstring(*rst.error + ", %1%"), *pack.src_info);
            }
        }
    }
    return true;
}

PragmaBytePack::AddConstraintResult PragmaBytePack::add_packing_constraint(
    const PackConstraint &packing) {
    std::stringstream ss;
    PragmaBytePack::AddConstraintResult rv;
    int offset = 0;
    for (const auto &slice : boost::adaptors::reverse(packing.packing.layout)) {
        if (slice.is_fs()) {
            const auto &fs = slice.fs();
            auto *field = phv_i.field(fs.first->id);
            CHECK_NULL(field);
            const auto alignment =
                FieldAlignment(le_bitrange(StartLen(offset + fs.second.lo, fs.second.size())));
            if (field->alignment) {
                if (*field->alignment != alignment) {
                    ss << "Cannot add @pa_byte_pack because: conflicting alignment on field "
                       << field << " original: " << *field->alignment
                       << ", added by byte pack constraint: " << alignment << "\n";
                    rv.error = ss.str();
                    break;
                }
            } else {
                field->updateAlignment(PHV::AlignmentReason::PA_BYTE_PACK, alignment,
                                       packing.src_info ? *packing.src_info : Util::SourceInfo());
                rv.alignment_added.insert(field);
            }
        }
        offset += slice.size();
    }
    if (rv.ok() && offset % 8 != 0) {
        ss << "invalid @pa_byte_pack pragma: the number of bits (" << offset << ") mod 8 != 0.";
        rv.error = ss.str();
    }
    if (rv.ok()) {
        packing_layouts_i.push_back(packing);
    } else {
        // revert partially updated alignment
        for (const auto &field : rv.alignment_added) {
            auto *fld = phv_i.field(field->id);
            CHECK_NULL(fld);
            fld->eraseAlignment();
        }
    }
    return rv;
}

PragmaBytePack::AddConstraintResult PragmaBytePack::add_compiler_added_packing(
    const PHV::PackingLayout &packing) {
    return add_packing_constraint(PackConstraint{true, std::nullopt, packing});
}
