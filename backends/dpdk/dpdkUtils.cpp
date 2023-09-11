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
    if (e->is<IR::Member>() || e->is<IR::PathExpression>() || e->is<IR::Constant>() ||
        e->is<IR::BoolLiteral>())
        return true;
    return false;
}

bool isNonConstantSimpleExpression(const IR::Expression *e) {
    if (e->is<IR::Member>() || e->is<IR::PathExpression>()) return true;
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
    bool isStdMeta =
        name == "psa_ingress_parser_input_metadata_t" || name == "psa_ingress_input_metadata_t" ||
        name == "psa_ingress_output_metadata_t" || name == "psa_egress_parser_input_metadata_t" ||
        name == "psa_egress_input_metadata_t" || name == "psa_egress_output_metadata_t" ||
        name == "psa_egress_deparser_input_metadata_t" || name == "pna_pre_input_metadata_t" ||
        name == "pna_pre_output_metadata_t" || name == "pna_main_input_metadata_t" ||
        name == "pna_main_output_metadata_t" || name == "pna_main_parser_input_metadata_t";
    return isStdMeta;
}

bool isHeadersStruct(const IR::Type_Struct *st) {
    if (!st) return false;
    auto annon = st->getAnnotation("__packet_data__");
    if (annon) return true;
    return false;
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
    if (!e->is<IR::Member>()) return false;
    if (e->to<IR::Member>()->expr->type->is<IR::Type_Struct>())
        return isMetadataStruct(e->to<IR::Member>()->expr->type->to<IR::Type_Struct>());
    return false;
}

bool isEightBitAligned(const IR::Expression *e) {
    if (e->type->width_bits() % 8 != 0) return false;
    return true;
}

bool isLargeFieldOperand(const IR::Expression *e) {
    auto expr = e;
    if (auto base = e->to<IR::Cast>()) expr = base->expr;
    if (auto type = expr->type->to<IR::Type_Bits>()) {
        auto size = type->width_bits();
        if (size > 64) return true;
    }
    return false;
}

bool isInsideHeader(const IR::Expression *expr) {
    auto e = expr;
    if (auto base = expr->to<IR::Cast>()) e = base->expr;
    if (!e->is<IR::Member>()) return false;
    if (e->to<IR::Member>()->expr->type->is<IR::Type_Header>()) return true;
    return false;
}

const IR::Type_Bits *getEightBitAlignedType(const IR::Type_Bits *tb) {
    auto width = (tb->width_bits() + 7) & (~7);
    return IR::Type_Bits::get(width);
}

bool isDirection(const IR::Member *m) {
    if (m == nullptr) return false;
    return m->member.name == "pna_main_input_metadata_direction" ||
           m->member.name == "pna_pre_input_metadata_direction" ||
           m->member.name == "pna_main_parser_input_metadata_direction";
}

// Creates Register extern declaration for holding persistent information
IR::Declaration_Instance *createRegDeclarationInstance(cstring instanceName, int regSize,
                                                       int indexBitWidth, int initValBitWidth) {
    auto typepath = new IR::Path("Register");
    auto type = new IR::Type_Name(typepath);
    auto typeargs = new IR::Vector<IR::Type>(
        {IR::Type::Bits::get(indexBitWidth), IR::Type::Bits::get(initValBitWidth)});
    auto spectype = new IR::Type_Specialized(type, typeargs);
    auto args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(new IR::Constant(IR::Type::Bits::get(32), regSize)));
    auto annot = IR::Annotations::empty;
    annot->addAnnotationIfNew(IR::Annotation::nameAnnotation, new IR::StringLiteral(instanceName));
    auto decl = new IR::Declaration_Instance(instanceName, annot, spectype, args, nullptr);
    return decl;
}

// Check for reserved names for DPDK target
bool reservedNames(P4::ReferenceMap *refMap, std::vector<cstring> names, cstring &resName) {
    for (auto name : names) {
        auto newname = refMap->newName(name);
        if (newname != name) {
            resName = name;
            return false;
        }
    }
    return true;
}

// Update bitwidth of Metadata fields to 32 or 64 bits if it 8-bit aligned.
int getMetadataFieldWidth(int width) {
    if (width % 8 != 0) {
        BUG_CHECK(width <= 64, "Metadata bit-width expected to be within 64-bits, found %1%",
                  width);
        if (width < 32)
            return 32;
        else
            return 64;
    }
    return width;
}

}  // namespace DPDK
