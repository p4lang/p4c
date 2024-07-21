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

#include <cstdio>
#include <map>

#include "ir/id.h"
#include "ir/indexed_vector.h"
#include "ir/ir.h"
#include "ir/namemap.h"
#include "ir/node.h"
#include "ir/vector.h"
#include "lib/bitops.h"
#include "lib/cstring.h"
#include "lib/error.h"
#include "lib/error_catalog.h"
#include "lib/source_file.h"

#define SINGLETON_TYPE(NAME)                                               \
    const IR::Type_##NAME *IR::Type_##NAME::get() {                        \
        static const Type_##NAME *singleton;                               \
        if (!singleton) singleton = (new Type_##NAME(Util::SourceInfo())); \
        return singleton;                                                  \
    }
SINGLETON_TYPE(Block)
SINGLETON_TYPE(Counter)
SINGLETON_TYPE(Expression)
SINGLETON_TYPE(FieldListCalculation)
SINGLETON_TYPE(Meter)
SINGLETON_TYPE(Register)
SINGLETON_TYPE(AnyTable)

cstring IR::NamedCond::unique_name() {
    static int unique_counter = 0;
    char buf[16];
    snprintf(buf, sizeof(buf), "cond-%d", unique_counter++);
    return cstring(buf);
}

struct primitive_info_t {
    unsigned min_operands, max_operands;
    unsigned out_operands;         // bitset -- 1 bit per operand
    unsigned type_match_operands;  // bitset -- 1 bit per operand
};

using namespace p4c::P4::literals;

static const std::map<cstring, primitive_info_t> prim_info = {
    {"add"_cs, {3, 3, 0x1, 0x7}},
    {"add_header"_cs, {1, 1, 0x1, 0x0}},
    {"add_to_field"_cs, {2, 2, 0x1, 0x3}},
    {"bit_and"_cs, {3, 3, 0x1, 0x7}},
    {"bit_andca"_cs, {3, 3, 0x1, 0x7}},
    {"bit_andcb"_cs, {3, 3, 0x1, 0x7}},
    {"bit_nand"_cs, {3, 3, 0x1, 0x7}},
    {"bit_nor"_cs, {3, 3, 0x1, 0x7}},
    {"bit_not"_cs, {2, 2, 0x1, 0x3}},
    {"bit_or"_cs, {3, 3, 0x1, 0x7}},
    {"bit_orca"_cs, {3, 3, 0x1, 0x7}},
    {"bit_orcb"_cs, {3, 3, 0x1, 0x7}},
    {"bit_xnor"_cs, {3, 3, 0x1, 0x7}},
    {"bit_xor"_cs, {3, 3, 0x1, 0x7}},
    {"bypass_egress"_cs, {0, 0, 0x0, 0x0}},
    {"clone_egress_pkt_to_egress"_cs, {1, 2, 0x0, 0x0}},
    {"clone_ingress_pkt_to_egress"_cs, {1, 2, 0x0, 0x0}},
    {"copy_header"_cs, {2, 2, 0x1, 0x3}},
    {"copy_to_cpu"_cs, {1, 1, 0x0, 0x0}},
    {"count"_cs, {2, 2, 0x1, 0x0}},
    {"drop"_cs, {0, 0, 0x0, 0x0}},
    {"emit"_cs, {1, 1, 0x0, 0x0}},
    {"execute_meter"_cs, {3, 4, 0x5, 0x0}},
    {"execute_stateful_alu"_cs, {1, 2, 0x0, 0x0}},
    {"execute_stateful_alu_from_hash"_cs, {1, 2, 0x0, 0x0}},
    {"execute_stateful_log"_cs, {1, 1, 0x0, 0x0}},
    {"exit"_cs, {0, 0, 0x0, 0x0}},
    {"extract"_cs, {1, 1, 0x1, 0x0}},
    {"funnel_shift_right"_cs, {4, 4, 0x1, 0x0}},
    {"generate_digest"_cs, {2, 2, 0x0, 0x0}},
    {"invalidate"_cs, {1, 1, 0x0, 0x0}},
    {"mark_for_drop"_cs, {0, 0, 0x0, 0x0}},
    {"max"_cs, {3, 3, 0x1, 0x7}},
    {"min"_cs, {3, 3, 0x1, 0x7}},
    {"modify_field"_cs, {2, 3, 0x1, 0x7}},
    {"modify_field_conditionally"_cs, {3, 3, 0x1, 0x5}},
    {"modify_field_from_rng"_cs, {2, 3, 0x1, 0x5}},
    {"modify_field_rng_uniform"_cs, {3, 3, 0x1, 0x5}},
    {"modify_field_with_hash_based_offset"_cs, {4, 4, 0x1, 0x0}},
    {"no_op"_cs, {0, 0, 0x0, 0x0}},
    {"pop"_cs, {1, 2, 0x1, 0x0}},
    {"push"_cs, {1, 2, 0x1, 0x0}},
    {"recirculate"_cs, {1, 1, 0x0, 0x0}},
    {"register_read"_cs, {3, 3, 0x1, 0x0}},
    {"register_write"_cs, {3, 3, 0x0, 0x0}},
    {"remove_header"_cs, {1, 1, 0x1, 0x0}},
    {"resubmit"_cs, {0, 1, 0x0, 0x0}},
    {"sample_e2e"_cs, {2, 3, 0x0, 0x0}},
    {"set_metadata"_cs, {2, 2, 0x1, 0x3}},
    {"shift_left"_cs, {3, 3, 0x1, 0x0}},
    {"shift_right"_cs, {3, 3, 0x1, 0x0}},
    {"subtract"_cs, {3, 3, 0x1, 0x7}},
    {"subtract_from_field"_cs, {2, 2, 0x1, 0x3}},
    {"truncate"_cs, {1, 1, 0x0, 0x0}},
    {"valid"_cs, {1, 1, 0x0, 0x0}},
};

void IR::Primitive::typecheck() const {
    if (prim_info.count(name)) {
        auto &info = prim_info.at(name);
        if (operands.size() < info.min_operands)
            error(ErrorType::ERR_INSUFFICIENT, "%s: not enough operands for primitive %s", srcInfo,
                  name);
        if (operands.size() > info.max_operands)
            error(ErrorType::ERR_OVERLIMIT, "%s: too many operands for primitive %s", srcInfo,
                  name);
    } else {
        /*error("%s: unknown primitive %s", srcInfo, name);*/ }
}

bool IR::Primitive::isOutput(int operand_index) const {
    if (prim_info.count(name)) return (prim_info.at(name).out_operands >> operand_index) & 1;
    return false;
}

unsigned IR::Primitive::inferOperandTypes() const {
    if (prim_info.count(name)) return prim_info.at(name).type_match_operands;
    return 0;
}

// infer the index width of a meter/counter/register based on the instance count
// default to 32 bits if we can't figure it out
int IR::Stateful::index_width() const {
    return instance_count > 1 ? ceil_log2(instance_count) : 32;
}

static int inferIndexWidth(const IR::Expression *obj) {
    if (auto *glob = obj->to<IR::GlobalRef>())
        if (auto *sful = glob->obj->to<IR::Stateful>()) return sful->index_width();
    return 32;
}

const IR::Type *IR::Primitive::inferOperandType(int operand) const {
    const IR::Type *rv = IR::Type::Unknown::get();
    unsigned infer = 0;

    if (prim_info.count(name)) infer = prim_info.at(name).type_match_operands;

    if ((infer >> operand) & 1) {
        for (auto o : operands) {
            if ((infer & 1) && o->type != rv) {
                rv = o->type;
                break;
            }
            infer >>= 1;
        }
        return rv;
    }
    if (name == "truncate") return IR::Type::Bits::get(32);
    if ((name == "count" || name == "execute_meter") && operand == 1)
        return IR::Type::Bits::get(inferIndexWidth(operands.at(0)));
    if (name.startsWith("execute_stateful") && operand == 1) return IR::Type::Bits::get(32);
    if ((name == "clone_ingress_pkt_to_egress" || name == "clone_i2e" ||
         name == "clone_egress_pkt_to_egress" || name == "clone_e2e") &&
        operand == 0) {
        return IR::Type::Bits::get(32);
    }
    if ((name == "execute") && operand == 2)
        return IR::Type::Bits::get(inferIndexWidth(operands.at(0)));
    if (name == "modify_field_conditionally" && operand == 1) return IR::Type::Bits::get(1);
    if (name == "register_read" && operand == 2)
        return IR::Type::Bits::get(inferIndexWidth(operands.at(1)));
    if (name == "register_write" && operand == 1)
        return IR::Type::Bits::get(inferIndexWidth(operands.at(0)));
    if (name == "shift_left" && operand == 1) {
        if (operands.at(0)->type->width_bits() > operands.at(1)->type->width_bits())
            return operands.at(0)->type;
        return IR::Type::Unknown::get();
    }
    if (name == "shift_right" && operand == 1) {
        if (operands.at(0)->type->width_bits() > operands.at(1)->type->width_bits())
            return operands.at(0)->type;
        return IR::Type::Unknown::get();
    }
    return rv;
}

IR::V1Program::V1Program() {
    // This is used to typecheck P4-14 programs
    auto *standard_metadata_t =
        new IR::Type_Struct("standard_metadata_t",
                            {new IR::StructField("ingress_port", IR::Type::Bits::get(9)),
                             new IR::StructField("packet_length", IR::Type::Bits::get(32)),
                             new IR::StructField("egress_spec", IR::Type::Bits::get(9)),
                             new IR::StructField("egress_port", IR::Type::Bits::get(9)),
                             new IR::StructField("egress_instance", IR::Type::Bits::get(16)),
                             new IR::StructField("instance_type", IR::Type::Bits::get(32)),
                             new IR::StructField("parser_status", IR::Type::Bits::get(8)),
                             new IR::StructField("parser_error_location", IR::Type::Bits::get(8))});
    scope.add("standard_metadata_t"_cs, new IR::v1HeaderType(standard_metadata_t));
    scope.add("standard_metadata"_cs, new IR::Metadata("standard_metadata", standard_metadata_t));
}
