/* Copyright 2022 Intel Corporation

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

#include "dpdkUtils.h"

namespace DPDK {
bool isSimpleExpression(const IR::Expression *e) {
    if (e->is<IR::Member>() || e->is<IR::PathExpression>() ||
        e->is<IR::Constant>() || e->is<IR::BoolLiteral>())
        return true;
    return false;
}

bool isNonConstantSimpleExpression(const IR::Expression *e) {
    if (e->is<IR::Member>() || e->is<IR::PathExpression>())
        return true;
    return false;
}

bool isCommutativeBinaryOperation(const IR::Operation_Binary *bin) {
    auto right = bin->right;
    if (right->is<IR::Add>() || right->is<IR::Equ>() || right->is<IR::LOr>() ||
        right->is<IR::LAnd>() || right->is<IR::BOr>() || right->is<IR::BAnd>() ||
        right->is<IR::BXor>())
        return true;
    return false;
}

bool isStandardMetadata(cstring name) {
    bool isStdMeta = name == "psa_ingress_parser_input_metadata_t" ||
                     name == "psa_ingress_input_metadata_t" ||
                     name == "psa_ingress_output_metadata_t" ||
                     name == "psa_egress_parser_input_metadata_t" ||
                     name == "psa_egress_input_metadata_t" ||
                     name == "psa_egress_output_metadata_t" ||
                     name == "psa_egress_deparser_input_metadata_t" ||
                     name == "pna_pre_input_metadata_t" ||
                     name == "pna_pre_output_metadata_t" ||
                     name == "pna_main_input_metadata_t" ||
                     name == "pna_main_output_metadata_t" ||
                     name == "pna_main_parser_input_metadata_t";
    return isStdMeta;
}

bool isMetadataStruct(const IR::Type_Struct *st) {
    for (auto anno : st->annotations->annotations) {
        if (anno->name == "__metadata__") {
            return true;
        }
    }
    return false;
}

bool isMetadataField(const IR::Expression *e) {
    if (!e->is<IR::Member>())
        return false;
    if (e->to<IR::Member>()->expr->type->is<IR::Type_Struct>())
        return isMetadataStruct(e->to<IR::Member>()->expr->type->to<IR::Type_Struct>());
    return false;
}

bool isEightBitAligned(const IR::Expression *e) {
    if (e->type->width_bits() % 8 != 0)
        return false;
    return true;
}

}  // namespace DPDK
