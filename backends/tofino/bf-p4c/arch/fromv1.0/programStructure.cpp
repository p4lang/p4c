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

#include "programStructure.h"

#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/arch/fromv1.0/meter.h"
#include "bf-p4c/arch/fromv1.0/phase0.h"
#include "bf-p4c/arch/intrinsic_metadata.h"
#include "bf-p4c/common/pragma/all_pragmas.h"
#include "bf-p4c/device.h"
#include "frontends/p4-14/fromv1.0/converters.h"
#include "frontends/p4-14/header_type.h"
#include "frontends/p4-14/typecheck.h"
#include "frontends/p4/cloner.h"
#include "frontends/p4/simplify.h"
#include "frontends/p4/unusedDeclarations.h"
#include "lib/error.h"

namespace P4 {
namespace P4V1 {

// TODO: older definition of ProgramStructure used by 14-to-v1model conversion path
// to be removed
// ** BEGIN **

const IR::Expression *P4V1::TNA_ProgramStructure::convertHashAlgorithm(Util::SourceInfo srcInfo,
                                                                       IR::ID algorithm) {
    include("tofino/p4_14_prim.p4"_cs, "-D_TRANSLATE_TO_V1MODEL"_cs);
    return IR::MAU::HashFunction::convertHashAlgorithmBFN(srcInfo, algorithm);
}

// This convertTable function calls the parent frontend convertTable function
// and appends additional info required for the backend. This is currently
// restricted to action selectors and pulls out information on hash calculation
// which is used by PD Generation.
// For future use, this can be expanded to include other info used by backend
// which is not extracted from the frontend function call.
const IR::P4Table *P4V1::TNA_ProgramStructure::convertTable(
    const IR::V1Table *table, cstring newName, IR::IndexedVector<IR::Declaration> &stateful,
    std::map<cstring, cstring> &mapNames) {
    // Generate p4Table from frontend call
    auto p4Table = ProgramStructure::convertTable(table, newName, stateful, mapNames);

    // Check for action selector and generate a new IR::P4Table with additional
    // annotations for hash info required for PD Generation
    cstring profile = table->action_profile.name;
    auto *action_profile = action_profiles.get(profile);
    auto *action_selector =
        action_profile ? action_selectors.get(action_profile->selector.name) : nullptr;

    if (action_selector) {
        auto flc = field_list_calculations.get(action_selector->key.name);
        if (flc) {
            auto annos = p4Table->getAnnotations();
            annos = annos->addAnnotation("action_selector_hash_field_calc_name"_cs,
                                         new IR::StringLiteral(flc->name));
            annos = annos->addAnnotation("action_selector_hash_field_list_name"_cs,
                                         new IR::StringLiteral(flc->input->names[0]));
            for (auto a : flc->algorithm->names) {
                annos = annos->addAnnotation("algorithm"_cs, new IR::StringLiteral(a));
            }
            annos = annos->addAnnotation("action_selector_hash_field_calc_output_width"_cs,
                                         new IR::Constant(flc->output_width));
            auto newP4Table =
                new IR::P4Table(p4Table->srcInfo, newName, annos, p4Table->properties);
            LOG4("Adding dynamic hash annotations: " << annos << " to table " << table->name);
            return newP4Table;
        }
    }
    return p4Table;
}

const IR::Declaration_Instance *P4V1::TNA_ProgramStructure::convertActionProfile(
    const IR::ActionProfile *action_profile, cstring newName) {
    // Generate decl from frontend call
    auto decl = ProgramStructure::convertActionProfile(action_profile, newName);

    auto mergedAnnotations = action_profile->annotations;
    for (auto annotation : decl->annotations->annotations)
        mergedAnnotations = mergedAnnotations->add(annotation);
    // Add annotations from action profile to the declaration
    auto newDecl = new IR::Declaration_Instance(decl->name, mergedAnnotations, decl->type,
                                                decl->arguments, nullptr);
    return newDecl;
}

// ** END **

const std::map<cstring, int> intrinsic_metadata = {{"ig_prsr_ctrl"_cs, 0},
                                                   {"ig_intr_md"_cs, 1},
                                                   {"ig_pg_md"_cs, 2},
                                                   {"ig_intr_md_for_tm"_cs, 3},
                                                   {"ig_intr_md_from_parser_aux"_cs, 4},
                                                   {"ig_intr_md_for_mb"_cs, 5},
                                                   {"ig_intr_md_for_dprsr"_cs, 6},
                                                   {"eg_intr_md"_cs, 7},
                                                   {"eg_intr_md_from_parser_aux"_cs, 8},
                                                   {"eg_intr_md_for_mb"_cs, 9},
                                                   {"eg_intr_md_for_oport"_cs, 10},
                                                   {"eg_intr_md_for_dprsr"_cs, 11}};

const std::vector<std::tuple<cstring, cstring, IR::Direction>> intrinsic_metadata_type = {
    std::make_tuple("ingress_parser_control_signals"_cs, "ig_prsr_ctrl"_cs, IR::Direction::In),
    std::make_tuple("ingress_intrinsic_metadata_t"_cs, "ig_intr_md"_cs, IR::Direction::In),
    std::make_tuple("generator_metadata_t"_cs, "ig_pg_md"_cs, IR::Direction::In),
    std::make_tuple("ingress_intrinsic_metadata_for_tm_t"_cs, "ig_intr_md_for_tm"_cs,
                    IR::Direction::InOut),
    std::make_tuple("ingress_intrinsic_metadata_from_parser_aux_t"_cs,
                    "ig_intr_md_from_parser_aux"_cs, IR::Direction::In),
    std::make_tuple("ingress_intrinsic_metadata_for_mirror_buffer_t"_cs, "ig_intr_md_for_mb"_cs,
                    IR::Direction::In),
    std::make_tuple("ingress_intrinsic_metadata_for_deparser_t"_cs, "ig_intr_md_for_dprsr"_cs,
                    IR::Direction::InOut),
    std::make_tuple("egress_intrinsic_metadata_t"_cs, "eg_intr_md"_cs, IR::Direction::In),
    std::make_tuple("egress_intrinsic_metadata_from_parser_aux_t"_cs,
                    "eg_intr_md_from_parser_aux"_cs, IR::Direction::In),
    std::make_tuple("egress_intrinsic_metadata_for_mirror_buffer_t"_cs, "eg_intr_md_for_mb"_cs,
                    IR::Direction::In),
    std::make_tuple("egress_intrinsic_metadata_for_output_port_t"_cs, "eg_intr_md_for_oport"_cs,
                    IR::Direction::InOut),
    // these metadata are new in tna
    std::make_tuple("egress_intrinsic_metadata_for_deparser_t"_cs, "eg_intr_md_for_dprsr"_cs,
                    IR::Direction::InOut),
};

const IR::Expression *TnaProgramStructure::convertHashAlgorithm(Util::SourceInfo srcInfo,
                                                                IR::ID algorithm) {
    auto algo = IR::MAU::HashFunction::convertHashAlgorithmBFN(srcInfo, algorithm);

    auto mce = algo->to<IR::MethodCallExpression>();
    if (!mce) FATAL_ERROR("hash algorithm %1% is not supported in tofino", algorithm);

    auto path = mce->method->to<IR::PathExpression>();
    BUG_CHECK(path != nullptr, "Unable to find path expression in hash algorithm");
    auto alg_name = path->path->name;
    if (alg_name == "crc_poly") {
        auto mc_name = "CRCPolynomial";
        auto typeArgs = new IR::Vector<IR::Type>();
        typeArgs->push_back(mce->arguments->at(2)->expression->type);
        auto arguments = new IR::Vector<IR::Argument>();
        arguments->push_back(mce->arguments->at(2));
        arguments->push_back(mce->arguments->at(5));
        arguments->push_back(mce->arguments->at(0));
        arguments->push_back(mce->arguments->at(1));
        arguments->push_back(mce->arguments->at(3));
        arguments->push_back(mce->arguments->at(4));
        return new IR::ConstructorCallExpression(
            srcInfo, new IR::Type_Specialized(new IR::Type_Name(mc_name), typeArgs), arguments);
    } else if (alg_name == "identity_hash") {
        return new IR::Member(new IR::TypeNameExpression("HashAlgorithm_t"), "IDENTITY");
    } else if (alg_name == "xor16") {
        return new IR::Member(new IR::TypeNameExpression("HashAlgorithm_t"), "XOR16");
    } else if (alg_name == "random_hash") {
        return new IR::Member(new IR::TypeNameExpression("HashAlgorithm_t"), "RANDOM");
    }
    return algo;
}

void TnaProgramStructure::checkHeaderType(const IR::Type_StructLike *hdr, bool metadata) {
    LOG3("Checking " << hdr << " " << (metadata ? "M" : "H"));
    for (auto f : hdr->fields) {
        if (f->type->is<IR::Type_Varbits>()) {
            if (metadata) error("%1%: varbit types illegal in metadata", f);
        } else if (f->type->is<IR::Type_Name>()) {
            // type name only comes from Type_StructLike
        } else if (f->type->is<IR::Type_StructLike>()) {
            // during the conversion to tna, header type is created from field
            // list, so this check needs to be recursive.
            checkHeaderType(f->type->to<IR::Type_StructLike>(), f->type->is<IR::Type_Struct>());
        } else if (!f->type->is<IR::Type_Bits>()) {
            // These come from P4-14, so they cannot be anything else
            BUG("%1%: unexpected type", f);
        }
    }
}

const IR::Declaration_Instance *TnaProgramStructure::convertDirectCounter(const IR::Counter *c,
                                                                          cstring newName) {
    LOG3("Synthesizing " << c);
    auto annos = addGlobalNameAnnotation(c->name, c->annotations);
    auto typeArgs = new IR::Vector<IR::Type>();
    if (c->min_width >= 0) {
        typeArgs->push_back(IR::Type::Bits::get(c->min_width));
    } else {
        // Do not impose LRT
        auto min_width = IR::Type::Bits::get(64);
        typeArgs->push_back(min_width);
        LOG1("WARNING: Could not infer min_width for counter %s, using bit<64>" << c);
    }

    // keep max_width as annotation
    if (c->max_width >= 0) {
        annos = annos->addAnnotation("max_width"_cs, new IR::Constant(c->max_width));
    }

    auto args = new IR::Vector<IR::Argument>();
    auto kindarg = counterType(c);
    args->push_back(new IR::Argument(kindarg));
    auto specializedType = new IR::Type_Specialized(new IR::Type_Name("DirectCounter"), typeArgs);
    return new IR::Declaration_Instance(newName, annos, specializedType, args);
}

const IR::Declaration_Instance *TnaProgramStructure::convertDirectMeter(const IR::Meter *c,
                                                                        cstring newName) {
    LOG3("Synthesizing " << c);
    auto annos = addGlobalNameAnnotation(c->name, c->annotations);
    auto args = new IR::Vector<IR::Argument>();
    auto kindarg = counterType(c);
    args->push_back(new IR::Argument(kindarg));
    auto meterType = new IR::Type_Name("DirectMeter");
    return new IR::Declaration_Instance(newName, annos, meterType, args);
}

const IR::Statement *TnaProgramStructure::convertMeterCall(const IR::Meter *meterToAccess) {
    ExpressionConverter conv(this);
    // add a writeback to the meter field
    auto decl = get(meterMap, meterToAccess);
    CHECK_NULL(decl);
    auto extObj = new IR::PathExpression(decl->name);
    auto method = new IR::Member(extObj, "execute");
    auto output = conv.convert(meterToAccess->result);
    IR::Expression *mc = new IR::MethodCallExpression(method, {});
    if (output->type->width_bits() > 8)
        mc = new IR::Cast(IR::Type::Bits::get(output->type->width_bits()), mc);
    else if (output->type->width_bits() < 8)
        mc = new IR::Slice(mc, output->type->width_bits() - 1, 0);
    auto stat = new IR::AssignmentStatement(mc->srcInfo, output, mc);
    return stat;
}

const IR::Statement *TnaProgramStructure::convertCounterCall(cstring counterToAccess) {
    ExpressionConverter conv(this);
    auto decl = get(counterMap, counterToAccess);
    CHECK_NULL(decl);
    auto extObj = new IR::PathExpression(decl->name);
    auto method = new IR::Member(extObj, "count");
    auto mc = new IR::MethodCallExpression(method, new IR::Vector<IR::Argument>());
    auto stat = new IR::MethodCallStatement(mc->srcInfo, mc);
    return stat;
}

const IR::Declaration_Instance *TnaProgramStructure::convert(const IR::Register *reg,
                                                             cstring newName,
                                                             const IR::Type *regElementType) {
    LOG3("Synthesizing " << reg);
    if (regElementType) {
        if (auto str = regElementType->to<IR::Type_StructLike>())
            regElementType = new IR::Type_Name(new IR::Path(str->name));
        // use provided type
    } else if (reg->width > 0) {
        regElementType = IR::Type_Bits::get(reg->width);
    } else if (reg->layout) {
        cstring newName = ::get(registerLayoutType, reg->layout);
        if (newName.isNullOrEmpty()) newName = reg->layout;
        regElementType = new IR::Type_Name(new IR::Path(newName));
    } else {
        ::warning(ErrorType::WARN_MISSING, "%1%: Register width unspecified; using %2%", reg,
                  defaultRegisterWidth);
        regElementType = IR::Type_Bits::get(defaultRegisterWidth);
    }

    auto type = reg->direct ? new IR::Type_Name("DirectRegister") : new IR::Type_Name("Register");
    auto typeArgs = new IR::Vector<IR::Type>();
    typeArgs->push_back(regElementType);
    if (!reg->direct) typeArgs->push_back(IR::Type::Bits::get(32));
    auto spectype = new IR::Type_Specialized(type, typeArgs);
    auto args = new IR::Vector<IR::Argument>();

    if (!reg->direct) {
        args->push_back(
            new IR::Argument(new IR::Constant(v1model.registers.size_type, reg->instance_count)));
    }

    auto annos = addGlobalNameAnnotation(reg->name, reg->annotations);
    auto decl = new IR::Declaration_Instance(newName, annos, spectype, args, nullptr);
    return decl;
}

/**
 * Generate code template that bypasses architecture definition and uses
 * internal architecture independent definition for hashes.
 */
IR::BlockStatement *generate_arch_neutral_hash_block_statement(P4V1::ProgramStructure *structure,
                                                               const IR::Primitive *prim,
                                                               const cstring temp,
                                                               ExpressionConverter &conv,
                                                               unsigned num_ops) {
    BUG_CHECK(prim->operands.size() == num_ops, "Wrong number of operands to %s", prim->name);
    auto pe = prim->operands.at(num_ops - 1)->to<IR::PathExpression>();
    auto flc = pe ? structure->field_list_calculations.get(pe->path->name) : nullptr;
    if (!flc) {
        error("%s: Expected a field_list_calculation", prim->operands.at(1));
        return nullptr;
    }
    IR::BlockStatement *block = new IR::BlockStatement;
    auto ttype = IR::Type_Bits::get(flc->output_width);
    block->push_back(new IR::Declaration_Variable(temp, ttype));

    auto fl = structure->getFieldLists(flc);
    if (fl == nullptr) return nullptr;
    const IR::ListExpression *listExp = conv.convert(fl)->to<IR::ListExpression>();
    auto list =
        new IR::HashListExpression(flc->srcInfo, listExp->components, flc->name, flc->output_width);
    list->fieldListNames = flc->input;
    if (flc->algorithm->names.size() > 0) list->algorithms = flc->algorithm;

    auto block_args = new IR::Vector<IR::Argument>();
    block_args->push_back(new IR::Argument(new IR::PathExpression(new IR::Path(temp))));
    block_args->push_back(new IR::Argument(structure->convertHashAlgorithms(flc->algorithm)));
    block_args->push_back(new IR::Argument(new IR::Constant(ttype, 0)));
    block_args->push_back(new IR::Argument(list));
    block_args->push_back(new IR::Argument(
        new IR::Constant(IR::Type_Bits::get(flc->output_width + 1), 1 << flc->output_width)));
    block->push_back(new IR::MethodCallStatement(new IR::MethodCallExpression(
        new IR::PathExpression(structure->v1model.hash.Id()), block_args)));
    return block;
}

/**
 * TNA hash extern has two constructors:
 * Hash(HashAlgorithm_t)
 * Hash(HashAlgorithm_t, CRCPolynomial)
 *
 * The second constructor could be Hash(crc_poly) if the frontend implements a
 * more intelligent name lookup that is not only based on number of arguments.
 * For now, we will insert an HashAlgorithm_t.CUSTOM.
 */
void add_custom_enum_if_crc_poly(const IR::Expression *expr, IR::Vector<IR::Argument> *declArgs) {
    if (!expr->is<IR::ConstructorCallExpression>()) return;
    auto cce = expr->to<IR::ConstructorCallExpression>();
    auto path = cce->constructedType->to<IR::Type_Specialized>();
    if (!path) return;
    if (path->baseType->path->name == "CRCPolynomial") {
        auto mem = new IR::Member(new IR::TypeNameExpression("HashAlgorithm_t"), "CUSTOM");
        declArgs->push_back(new IR::Argument(mem));
    }
}

/**
 * Generate code template that matches to TNA architecture.
 */
IR::BlockStatement *generate_tna_hash_block_statement(P4V1::TnaProgramStructure *structure,
                                                      const IR::Primitive *prim, const cstring temp,
                                                      ExpressionConverter &conv, unsigned num_ops,
                                                      const IR::Expression *dest = nullptr) {
    BUG_CHECK(prim->operands.size() == num_ops, "Wrong number of operands to %s", prim->name);
    unsigned offset = 0;
    LOG3("prim " << prim);
    if (prim->name == "execute_with_pre_color_from_hash") {
        offset = 1;
    } else if (prim->name == "execute_stateful_alu_from_hash") {
        offset = 1;
    } else if (prim->name == "modify_field_with_hash_based_offset") {
        offset = 2;
    } else if (prim->name == "count_from_hash") {
        offset = 1;
    } else if (prim->name == "execute_meter_from_hash_with_or") {
        offset = 2;
    } else if (prim->name == "execute_meter_from_hash") {
        offset = 2;
    } else if (prim->name == "execute_from_hash") {
        offset = 1;
    } else {
        BUG("Unhandled primitive %1%", prim->name);
    }
    auto flc_offset = num_ops - offset;
    // TODO: workaround for a v1 typeCheck error when a P4-14 table shares
    // the same name with the field-list-calculation, typeChecker incorrect infers
    // the primitive parameter with the type of the table, instead of the type
    // of the field-list-calculation.
    std::optional<cstring> name = std::nullopt;
    if (auto pe = prim->operands.at(flc_offset)->to<IR::PathExpression>()) {
        name = pe->path->name;
    } else if (auto tbl = prim->operands.at(flc_offset)->to<IR::V1Table>()) {
        name = tbl->name;
    } else if (auto gref = prim->operands.at(flc_offset)->to<IR::GlobalRef>()) {
        if (auto tbl = gref->obj->to<IR::V1Table>()) {
            name = tbl->name;
        }
    }
    auto flc = (name != std::nullopt) ? structure->field_list_calculations.get(*name) : nullptr;
    if (!flc) {
        error("%s: Expected a field_list_calculation", prim->operands.at(flc_offset));
        return nullptr;
    }
    IR::BlockStatement *block = new IR::BlockStatement;
    auto ttype = IR::Type_Bits::get(flc->output_width);
    if (!dest) block->push_back(new IR::Declaration_Variable(temp, ttype));

    auto fl = structure->getFieldLists(flc);
    if (fl == nullptr) return nullptr;
    const IR::ListExpression *listExp = conv.convert(fl)->to<IR::ListExpression>();
    auto list =
        new IR::HashListExpression(flc->srcInfo, listExp->components, flc->name, flc->output_width);
    list->fieldListNames = flc->input;
    if (flc->algorithm->names.size() > 0) list->algorithms = flc->algorithm;

    // Hash() hash_0;
    cstring hash_inst = structure->makeUniqueName("hash"_cs);
    auto declArgs = new IR::Vector<IR::Argument>();
    auto algo = structure->convertHashAlgorithms(flc->algorithm);
    add_custom_enum_if_crc_poly(algo, declArgs);
    declArgs->push_back(new IR::Argument(algo));
    auto declType =
        new IR::Type_Specialized(new IR::Type_Name("Hash"), new IR::Vector<IR::Type>({ttype}));
    const IR::Annotations *annos = IR::Annotations::empty;
    auto symmetricAnnotation = fl->annotations->getSingle("symmetric"_cs);
    if (symmetricAnnotation) {
        if (symmetricAnnotation->expr.size() == 2) {
            // auto name1 = symmetricAnnotation->expr[0];
            // auto name2 = symmetricAnnotation->expr[1];
            annos->addAnnotation("symmetric"_cs, new IR::ListExpression(symmetricAnnotation->expr));
        } else {
            error("Invalid pragma usage: symmetric");
        }
    }
    auto decl = new IR::Declaration_Instance(hash_inst, declType, declArgs);
    structure->localInstances.push_back(decl);

    auto block_args = new IR::Vector<IR::Argument>();
    block_args->push_back(new IR::Argument(list));
    auto hash_get = new IR::Member(new IR::PathExpression(hash_inst), IR::ID("get"));
    auto methodCall = new IR::MethodCallExpression(hash_get, block_args);
    if (!dest) {
        block->push_back(new IR::AssignmentStatement(new IR::PathExpression(temp), methodCall));
    } else {
        if (dest->type->width_bits() > ttype->width_bits()) {
            int padding_width = dest->type->width_bits() - ttype->width_bits();
            block->push_back(new IR::AssignmentStatement(
                dest, new IR::Concat(new IR::Constant(IR::Type::Bits::get(padding_width), 0),
                                     methodCall)));
        } else if (dest->type->width_bits() < ttype->width_bits()) {
            block->push_back(new IR::AssignmentStatement(
                dest, new IR::Slice(methodCall, dest->type->width_bits() - 1, 0)));
        } else {
            block->push_back(new IR::AssignmentStatement(dest, methodCall));
        }
    }
    return block;
}

IR::BlockStatement *generate_hash_block_statement(P4V1::ProgramStructure *structure,
                                                  const IR::Primitive *prim, const cstring temp,
                                                  ExpressionConverter &conv, unsigned num_ops) {
    if (BackendOptions().arch == "tna" || BackendOptions().arch == "t2na") {
        auto st = dynamic_cast<P4V1::TnaProgramStructure *>(structure);
        BUG_CHECK(st != nullptr, "Unable to cast structure to tna programStructure");
        return generate_tna_hash_block_statement(st, prim, temp, conv, num_ops);
    }
    return generate_arch_neutral_hash_block_statement(structure, prim, temp, conv, num_ops);
}

// This convertTable function calls the parent frontend convertTable function
// and appends additional info required for the backend. This is currently
// restricted to action selectors and pulls out information on hash calculation
// which is used by PD Generation.
// For future use, this can be expanded to include other info used by backend
// which is not extracted from the frontend function call.
const IR::P4Table *TnaProgramStructure::convertTable(const IR::V1Table *table, cstring newName,
                                                     IR::IndexedVector<IR::Declaration> &stateful,
                                                     std::map<cstring, cstring> &mapNames) {
    // Generate p4Table from frontend call
    auto p4Table = ProgramStructure::convertTable(table, newName, stateful, mapNames);

    // Check for action selector and generate a new IR::P4Table with additional
    // annotations for hash info required for PD Generation
    cstring profile = table->action_profile.name;
    auto *action_profile = action_profiles.get(profile);
    auto *action_selector =
        action_profile ? action_selectors.get(action_profile->selector.name) : nullptr;

    if (action_selector) {
        auto flc = field_list_calculations.get(action_selector->key.name);
        if (flc) {
            auto annos = p4Table->getAnnotations();
            annos = annos->addAnnotation("action_selector_hash_field_calc_name"_cs,
                                         new IR::StringLiteral(flc->name));
            annos = annos->addAnnotation("action_selector_hash_field_list_name"_cs,
                                         new IR::StringLiteral(flc->input->names[0]));
            for (auto a : flc->algorithm->names) {
                annos = annos->addAnnotation("algorithm"_cs, new IR::StringLiteral(a));
            }
            annos = annos->addAnnotation("action_selector_hash_field_calc_output_width"_cs,
                                         new IR::Constant(flc->output_width));
            auto newP4Table =
                new IR::P4Table(p4Table->srcInfo, newName, annos, p4Table->properties);
            LOG4("Adding dynamic hash annotations: " << annos << " to table " << table->name);
            return newP4Table;
        }
    }

    LOG3("p4 table " << p4Table);
    return p4Table;
}

IR::ConstructorCallExpression *TnaProgramStructure::createHashExtern(const IR::Expression *expr,
                                                                     const IR::Type *ttype) {
    if (auto cce = expr->to<IR::ConstructorCallExpression>()) {
        auto declArgs = new IR::Vector<IR::Argument>();
        auto path = cce->constructedType->to<IR::Type_Specialized>();
        if (!path) return nullptr;
        if (path->baseType->path->name == "CRCPolynomial") {
            auto mem = new IR::Member(new IR::TypeNameExpression("HashAlgorithm_t"), "CUSTOM");
            declArgs->push_back(new IR::Argument(mem));
        }
        declArgs->push_back(new IR::Argument(cce));
        auto declType =
            new IR::Type_Specialized(new IR::Type_Name("Hash"), new IR::Vector<IR::Type>({ttype}));
        return new IR::ConstructorCallExpression(declType, declArgs);
    } else {
        auto declArgs = new IR::Vector<IR::Argument>();
        declArgs->push_back(new IR::Argument(expr));
        auto declType =
            new IR::Type_Specialized(new IR::Type_Name("Hash"), new IR::Vector<IR::Type>({ttype}));
        return new IR::ConstructorCallExpression(declType, declArgs);
    }
}

const IR::Declaration_Instance *TnaProgramStructure::convertActionProfile(
    const IR::ActionProfile *action_profile, cstring newName) {
    auto *action_selector = action_selectors.get(action_profile->selector.name);
    if (!action_profile->selector.name.isNullOrEmpty() && !action_selector)
        error("Cannot locate action selector %1%", action_profile->selector);
    const IR::Type *type = nullptr;
    auto args = new IR::Vector<IR::Argument>();
    auto annos = addGlobalNameAnnotation(action_profile->name);
    if (action_selector) {
        type = new IR::Type_Name(new IR::Path("ActionSelector"));
        if (action_profile->size == 0)
            LOG1("WARNING: action_profile " << action_profile->name
                                            << "is specified with 0 size, "
                                               "setting default value to 1024.");
        auto size = new IR::Constant(action_profile->srcInfo, IR::Type_Bits::get(32),
                                     action_profile->size == 0 ? 1024 : action_profile->size);
        args->push_back(new IR::Argument(size));
        auto flc = field_list_calculations.get(action_selector->key.name);
        auto ttype = IR::Type_Bits::get(flc->output_width);
        auto algorithm = createHashExtern(convertHashAlgorithms(flc->algorithm), ttype);
        args->push_back(new IR::Argument(algorithm));
        // TODO: tna action_selector does not support specifying width
        // auto width = new IR::Constant(IR::Type_Bits::get(32), flc->output_width);
        // args->push_back(new IR::Argument(width));
        if (action_selector->mode == "resilient") {
            args->push_back(new IR::Argument(
                new IR::Member(new IR::TypeNameExpression("SelectorMode_t"), "RESILIENT")));
        } else {
            args->push_back(new IR::Argument(
                new IR::Member(new IR::TypeNameExpression("SelectorMode_t"), "FAIR")));
        }
        auto fl = getFieldLists(flc);
        for (auto annot : fl->annotations->annotations) {
            annos = annos->add(annot);
        }
    } else {
        type = new IR::Type_Name(new IR::Path("ActionProfile"));
        if (action_profile->size == 0)
            LOG1("WARNING: action_profile " << action_profile->name
                                            << "is specified with 0 size, "
                                               "setting default value to 1024.");
        auto size = new IR::Constant(action_profile->srcInfo, IR::Type_Bits::get(32),
                                     action_profile->size == 0 ? 1024 : action_profile->size);
        args->push_back(new IR::Argument(size));
        // annos = annos->addAnnotation(action_profile->annotations);
    }
    auto decl = new IR::Declaration_Instance(newName, annos, type, args);

    auto mergedAnnotations = action_profile->annotations;
    for (auto annotation : decl->annotations->annotations)
        mergedAnnotations = mergedAnnotations->add(annotation);
    // Add annotations from action profile to the declaration
    auto newDecl = new IR::Declaration_Instance(decl->name, mergedAnnotations, decl->type,
                                                decl->arguments, nullptr);
    return newDecl;
}

void TnaProgramStructure::addIngressParams(IR::ParameterList *paramList) {
    // add ig_intr_md
    auto path = new IR::Path("ingress_intrinsic_metadata_t");
    auto type = new IR::Type_Name(path);
    auto param = new IR::Parameter("ig_intr_md"_cs, IR::Direction::In, type);
    paramList->push_back(param);

    // add ig_intr_md_from_prsr
    path = new IR::Path("ingress_intrinsic_metadata_from_parser_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md_from_parser_aux"_cs, IR::Direction::In, type);
    paramList->push_back(param);

    // add ig_intr_md_for_dprsr
    path = new IR::Path("ingress_intrinsic_metadata_for_deparser_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md_for_dprsr"_cs, IR::Direction::InOut, type);
    paramList->push_back(param);

    // add ig_intr_md_for_tm
    path = new IR::Path("ingress_intrinsic_metadata_for_tm_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md_for_tm"_cs, IR::Direction::InOut, type);
    paramList->push_back(param);
}

void TnaProgramStructure::addEgressParams(IR::ParameterList *paramList) {
    auto path = new IR::Path("egress_intrinsic_metadata_t");
    auto type = new IR::Type_Name(path);
    IR::Parameter *param = nullptr;
    if (BackendOptions().backward_compatible)
        param = new IR::Parameter("eg_intr_md"_cs, IR::Direction::In, type);
    else
        param = new IR::Parameter("eg_intr_md"_cs, IR::Direction::In, type);
    paramList->push_back(param);

    // add eg_intr_md_from_prsr
    path = new IR::Path("egress_intrinsic_metadata_from_parser_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("eg_intr_md_from_parser_aux"_cs, IR::Direction::In, type);
    paramList->push_back(param);

    // add eg_intr_md_for_dprsr
    path = new IR::Path("egress_intrinsic_metadata_for_deparser_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("eg_intr_md_for_dprsr"_cs, IR::Direction::InOut, type);
    paramList->push_back(param);

    // add eg_intr_md_for_oport
    path = new IR::Path("egress_intrinsic_metadata_for_output_port_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("eg_intr_md_for_oport"_cs, IR::Direction::InOut, type);
    paramList->push_back(param);
}

// Reference to P4-14 standard metadata is converted to reference to
// meta.standard_metadata in tna.
const IR::Expression *TnaProgramStructure::stdMetaReference(const IR::Parameter *param) {
    auto result = new IR::Member(new IR::PathExpression(param->name), IR::ID("standard_metadata"));
    auto type = types.get("standard_metadata_t"_cs);
    if (type != nullptr) result->type = type;
    return result;
}

const IR::Type_Control *TnaProgramStructure::controlType(IR::ID name) {
    /// look up the map build for control type
    auto params = new IR::ParameterList;

    auto headpath = new IR::Path(v1model.headersType.Id());
    auto headtype = new IR::Type_Name(headpath);
    // we can use ingress, since all control blocks have the same signature
    auto headers =
        new IR::Parameter(v1model.ingress.headersParam.Id(), IR::Direction::InOut, headtype);
    params->push_back(headers);
    conversionContext->header = paramReference(headers);

    auto metapath = new IR::Path(v1model.metadataType.Id());
    auto metatype = new IR::Type_Name(metapath);
    auto meta =
        new IR::Parameter(v1model.ingress.metadataParam.Id(), IR::Direction::InOut, metatype);
    params->push_back(meta);
    conversionContext->userMetadata = paramReference(meta);

    // tna does not use standard metadata as parameter
    conversionContext->standardMetadata = stdMetaReference(meta);

    if (name == "ingress") {
        addIngressParams(params);
    } else if (name == "egress") {
        addEgressParams(params);
    } else {
        if (controlParams.count(name.name)) {
            auto extraParams = controlParams.at(name.name);
            for (auto param : extraParams) {
                auto p = param->apply(FixParamType())->to<IR::Parameter>();
                params->push_back(p);
            }
        }
    }

    auto type = new IR::Type_Control(name, new IR::TypeParameters(), params);
    return type;
}

const IR::Expression *TnaProgramStructure::counterType(const IR::CounterOrMeter *cm) {
    IR::ID kind;
    IR::ID enumName;
    if (cm->is<IR::Counter>()) {
        enumName = "CounterType_t";
        if (cm->type == IR::CounterType::PACKETS)
            kind = "PACKETS";
        else if (cm->type == IR::CounterType::BYTES)
            kind = "BYTES";
        else if (cm->type == IR::CounterType::BOTH)
            kind = "PACKETS_AND_BYTES";
        else
            BUG("%1%: unsupported", cm);
    } else {
        BUG_CHECK(cm->is<IR::Meter>(), "%1%: expected a meter", cm);
        enumName = "MeterType_t";
        if (cm->type == IR::CounterType::PACKETS)
            kind = "PACKETS";
        else if (cm->type == IR::CounterType::BYTES)
            kind = "BYTES";
        else
            BUG("%1%: unsupported", cm);
    }
    auto enumref = new IR::TypeNameExpression(new IR::Type_Name(new IR::Path(enumName)));
    return new IR::Member(cm->srcInfo, enumref, kind);
}

const IR::Declaration_Instance *TnaProgramStructure::convert(const IR::CounterOrMeter *cm,
                                                             cstring newName) {
    LOG3("Synthesizing " << cm);
    IR::ID ext;
    if (cm->is<IR::Counter>())
        ext = "Counter";
    else
        ext = "Meter";
    auto typepath = new IR::Path(ext);
    auto type_name = new IR::Type_Name(typepath);
    auto args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(cm->srcInfo,
                                     new IR::Constant(IR::Type_Bits::get(32), cm->instance_count)));
    auto kindarg = counterType(cm);
    args->push_back(new IR::Argument(kindarg));
    auto type_args = new IR::Vector<IR::Type>();
    auto annos = addGlobalNameAnnotation(cm->name, cm->annotations);
    if (auto *c = cm->to<IR::Counter>()) {
        if (c->min_width >= 0) {
            annos = annos->addAnnotation("min_width"_cs, new IR::Constant(c->min_width));
            type_args->push_back(IR::Type_Bits::get(c->min_width));
        } else {
            type_args->push_back(IR::Type_Bits::get(64));  // by default, use 64
        }
        if (c->max_width >= 0)
            annos = annos->addAnnotation("max_width"_cs, new IR::Constant(c->max_width));
    }
    type_args->push_back(IR::Type_Bits::get(cm->index_width()));
    auto type = new IR::Type_Specialized(type_name, type_args);
    auto decl = new IR::Declaration_Instance(newName, annos, type, args, nullptr);
    return decl;
}

// Collect reference to global header from each action for generating control
// block constructor parameter list.
class FindHeaderReference : public Inspector {
    TnaProgramStructure *structure;
    cstring control;
    gress_t gress;
    bitvec &use;

 public:
    FindHeaderReference(TnaProgramStructure *s, cstring c, gress_t g, bitvec &use)
        : structure(s), control(c), gress(g), use(use) {
        CHECK_NULL(structure);
    }

    void postorder(const IR::Member *mem) override {
        if (auto ref = mem->expr->to<IR::ConcreteHeaderRef>()) {
            auto path = ref->ref->name + "." + mem->member;
            if (path == "ig_intr_md_for_tm.drop_ctl") {
                use.setbit(intrinsic_metadata.at("ig_intr_md_for_dprsr"_cs));
            } else if (path == "standard_metadata.egress_spec") {
                use.setbit(intrinsic_metadata.at("ig_intr_md_for_tm"_cs));
            } else if (path == "standard_metadata.ingress_port") {
                use.setbit(intrinsic_metadata.at("ig_intr_md"_cs));
            } else if (path == "ig_intr_md_for_tm.packet_color") {
                use.setbit(intrinsic_metadata.at("ig_intr_md_for_tm"_cs));
            }
        }
    }

    void postorder(const IR::ConcreteHeaderRef *hdr) override {
        if (intrinsic_metadata.count(hdr->ref->name) != 0) {
            use.setbit(intrinsic_metadata.at(hdr->ref->name));
        }
    }

    void postorder(const IR::PathExpression *p) override {
        if (intrinsic_metadata.count(p->path->name) != 0) {
            use.setbit(intrinsic_metadata.at(p->path->name));
        }

        // path could be a reference to field list calculation.
        auto flc = structure->getFieldListCalculation(p);
        if (flc) {
            flc->apply(FindHeaderReference(structure, control, gress, use));
        }
    }

    // action may invoke another action.
    void postorder(const IR::Primitive *p) override {
        if (auto act = structure->actions.get(p->name)) {
            act->apply(FindHeaderReference(structure, control, gress, use));
        } else if (structure->controls.get(p->name)) {
            LOG4("\tadd param use from control " << p->name);
            if (structure->controlParamUse.count(p->name)) {
                use |= structure->controlParamUse.at(p->name);
            }
        } else if (p->name == "drop" || p->name == "mark_for_drop" || p->name == "mark_to_drop") {
            if (structure->getGress(control) == INGRESS)
                use.setbit(intrinsic_metadata.at("ig_intr_md_for_dprsr"_cs));
            else
                use.setbit(intrinsic_metadata.at("eg_intr_md_for_dprsr"_cs));
        } else if (p->name == "clone_ingress_pkt_to_egress") {
            use.setbit(intrinsic_metadata.at("ig_intr_md_for_dprsr"_cs));
        } else if (p->name == "clone_egress_pkt_to_egress") {
            use.setbit(intrinsic_metadata.at("eg_intr_md_for_dprsr"_cs));
        } else if (p->name == "generate_digest") {
            use.setbit(intrinsic_metadata.at("ig_intr_md_for_dprsr"_cs));
        } else if (p->name == "recirculate" || p->name == "recirculate_preserving_field_list") {
            use.setbit(intrinsic_metadata.at("ig_intr_md_for_tm"_cs));
        } else if (p->name == "resubmit" || p->name == "resubmit_preserving_field_list") {
            use.setbit(intrinsic_metadata.at("ig_intr_md_for_dprsr"_cs));
        }
    }

    void postorder(const IR::V1Table *tbl) override {
        if (tbl->reads) {
            for (auto key : *tbl->reads) {
                if (auto hdr = key->to<IR::ConcreteHeaderRef>()) {
                    if (intrinsic_metadata.count(hdr->ref->name) != 0)
                        use.setbit(intrinsic_metadata.at(hdr->ref->name));
                } else if (auto mem = key->to<IR::Member>()) {
                    if (auto hdr = mem->expr->to<IR::ConcreteHeaderRef>()) {
                        if (intrinsic_metadata.count(hdr->ref->name) != 0)
                            use.setbit(intrinsic_metadata.at(hdr->ref->name));
                    }
                }
            }
        }

        // selector key could also refer to intrinsic metadata
        cstring profile = tbl->action_profile.name;
        auto *action_profile = structure->action_profiles.get(profile);
        auto *action_selector = action_profile
                                    ? structure->action_selectors.get(action_profile->selector.name)
                                    : nullptr;

        if (action_selector != nullptr) {
            auto flc = structure->field_list_calculations.get(action_selector->key.name);
            if (flc == nullptr) {
                error("Cannot locate field list %1%", action_selector->key);
                return;
            }
            auto fl = structure->getFieldLists(flc);
            if (fl != nullptr) {
                for (auto f : fl->fields) {
                    if (auto mem = f->to<IR::Member>()) {
                        if (auto hdr = mem->expr->to<IR::ConcreteHeaderRef>()) {
                            if (intrinsic_metadata.count(hdr->ref->name) != 0)
                                use.setbit(intrinsic_metadata.at(hdr->ref->name));
                        }
                    }
                }
            }
        }

        // direct meter result can refer to intrinsic metadata
        if (structure->directMeters.count(tbl->name)) {
            auto meter = structure->directMeters.at(tbl->name);
            if (meter) {
                if (auto result = meter->result->to<IR::Member>()) {
                    if (auto hdr = result->expr->to<IR::ConcreteHeaderRef>()) {
                        if (intrinsic_metadata.count(hdr->ref->name) != 0)
                            use.setbit(intrinsic_metadata.at(hdr->ref->name));
                    }
                }
            }
        }
    }
};

const IR::Node *FixApplyStatement::postorder(IR::MethodCallExpression *mce) {
    if (auto mem = mce->method->to<IR::Member>()) {
        if (auto path = mem->expr->to<IR::PathExpression>()) {
            LOG3(" path " << path);
            if (structure->controls.get(path->path->name)) {
                LOG3("found control apply " << mce);
            }
        }
    }
    return mce;
}

IR::Vector<IR::Argument> *TnaProgramStructure::createApplyArguments(cstring name) {
    auto args = new IR::Vector<IR::Argument>();
    args->push_back(new IR::Argument(conversionContext->header->clone()));
    args->push_back(new IR::Argument(conversionContext->userMetadata->clone()));

    if (controlParams.count(name)) {
        for (auto p : controlParams.at(name)) {
            auto arg = new IR::Argument(new IR::PathExpression(p->name));
            args->push_back(arg);
        }
    }
    return args;
}

std::vector<cstring> TnaProgramStructure::findActionInTables(const IR::V1Control *control) {
    std::vector<cstring> actionsInTables;
    std::vector<const IR::V1Table *> usedTables;
    tablesReferred(control, usedTables);
    for (auto t : usedTables) {
        for (auto a : t->actions) actionsInTables.push_back(a.name);
        if (!t->default_action.name.isNullOrEmpty())
            actionsInTables.push_back(t->default_action.name);
        if (!t->action_profile.name.isNullOrEmpty()) {
            auto ap = action_profiles.get(t->action_profile.name);
            for (auto a : ap->actions) actionsInTables.push_back(a.name);
        }
    }
    return actionsInTables;
}

void TnaProgramStructure::collectHeaderReference(const IR::V1Control *control) {
    bitvec use;
    auto visitor = new FindHeaderReference(this, control->name, currentGress, use);

    auto actionsInTables = findActionInTables(control);
    for (auto name : actionsInTables) {
        auto act = actions.get(name);
        LOG3("\taction: " << act);
        act->apply(*visitor);
    }

    std::vector<const IR::V1Table *> usedTables;
    tablesReferred(control, usedTables);
    for (auto t : usedTables) {
        t->apply(*visitor);
    }

    // collect reference to global header in control flow
    control->apply(*visitor);

    // cache use for faster lookup
    LOG4("\tsave param use for control " << control->name);
    controlParamUse.emplace(control->name, use);

    std::vector<const IR::Parameter *> params;
    for (unsigned long index = 0; index < intrinsic_metadata.size(); index++) {
        if (use[index]) {
            LOG3("\treference to intrinsic " << std::get<0>(intrinsic_metadata_type[index]));
            auto path = new IR::Path(std::get<0>(intrinsic_metadata_type[index]));
            auto type = new IR::Type_Name(path);
            auto meta = new IR::Parameter(IR::ID(std::get<1>(intrinsic_metadata_type[index])),
                                          std::get<2>(intrinsic_metadata_type[index]), type);
            params.push_back(meta);
        }
    }
    controlParams[control->name] = params;
}

gress_t TnaProgramStructure::getGress(cstring name) {
    if (name == "ingress")
        currentGress = INGRESS;
    else if (name == "egress")
        currentGress = EGRESS;
    else if (mapControlToGress.count(name))
        currentGress = mapControlToGress.at(name);
    return currentGress;
}

const IR::P4Control *TnaProgramStructure::convertControl(const IR::V1Control *control,
                                                         cstring newName) {
    LOG3("Converting backend " << control->name);
    collectHeaderReference(control);
    auto ctrl = ProgramStructure::convertControl(control, newName);

    // convert P4-14 metadata to P4-16 tna metadata
    ctrl = ctrl->apply(ConvertMetadata(this));

    auto locals = new IR::IndexedVector<IR::Declaration>();
    locals->append(localInstances);
    locals->append(ctrl->controlLocals);
    localInstances.clear();

    // add bridged metadata to ingress
    auto *body = ctrl->body->clone();
    if (control->name == "ingress") {
        P4::CloneExpressions cloner;

#if 1
        if (useBridgeMetadata()) {
            body->components.push_back(BFN::createSetMetadata(
                "meta"_cs, "bridged_header"_cs, "bridged_metadata_indicator"_cs, 8, 0));
            body->components.push_back(
                BFN::createSetValid(control->srcInfo, "meta"_cs, "bridged_header"_cs));
        }
#endif

        for (auto f : bridgedFields) {
            if (bridgedFieldInfo.count(f)) {
                auto info = bridgedFieldInfo.at(f);
                auto fn = convertLinearPathToStructFieldName(&info);
                auto *member = new IR::Member(
                    new IR::Member(new IR::PathExpression("meta"), IR::ID("bridged_header")),
                    IR::ID(fn));
                auto *bridgedMember = info.components.back()->apply(cloner);
                bridgedMember = bridgedMember->apply(ConvertConcreteHeaderRefToPathExpression());
                auto *assignment = new IR::AssignmentStatement(member, bridgedMember);
                body->components.push_back(assignment);
            }
        }
    }

    return new IR::P4Control(ctrl->name, ctrl->type, *locals, body);
}

// Adapted from frontend with changes on how name annotation is generated.
// Specifically, this function generates local name annotation for pvs and
// parser state instead of global name annotation.
const IR::ParserState *TnaProgramStructure::convertParser(
    const IR::V1Parser *parser, IR::IndexedVector<IR::Declaration> *stateful) {
    ExpressionConverter conv(this);

    latest = nullptr;
    IR::IndexedVector<IR::StatOrDecl> components;
    for (auto e : parser->stmts) {
        auto stmt = convertParserStatement(e);
        if (stmt) components.push_back(stmt);
    }
    const IR::Expression *select = nullptr;
    if (parser->select != nullptr) {
        auto list = new IR::ListExpression(parser->select->srcInfo, {});
        std::vector<const IR::Type::Bits *> fieldTypes;
        for (auto e : *parser->select) {
            auto c = conv.convert(e);
            list->components.push_back(c);
            if (auto *t = c->type->to<IR::Type::Bits>()) {
                fieldTypes.push_back(t);
            } else {
                auto w = c->type->width_bits();
                BUG_CHECK(w > 0, "Unknown width for expression %1%", e);
                fieldTypes.push_back(IR::Type::Bits::get(w));
            }
        }
        BUG_CHECK(list->components.size() > 0, "No select expression in %1%", parser);
        // select always expects a ListExpression
        IR::Vector<IR::SelectCase> cases;
        for (auto c : *parser->cases) {
            IR::ID state = c->action;
            auto deststate = getState(state);
            if (deststate == nullptr) return nullptr;

            // discover all parser value_sets in the current parser state.
            for (auto v : c->values) {
                auto first = v.first->to<IR::PathExpression>();
                if (!first) continue;
                auto value_set = value_sets.get(first->path->name);
                if (!value_set) {
                    error("Unable to find declaration for parser_value_set %s", first->path->name);
                    return nullptr;
                }

                auto type = ProgramStructure::explodeType(fieldTypes);
                auto sizeAnnotation = value_set->annotations->getSingle("parser_value_set_size"_cs);
                const IR::Constant *sizeConstant;
                if (sizeAnnotation) {
                    if (sizeAnnotation->expr.size() != 1) {
                        error("@size should be an integer for declaration %1%", value_set);
                        return nullptr;
                    }
                    sizeConstant = sizeAnnotation->expr[0]->to<IR::Constant>();
                    if (sizeConstant == nullptr || !sizeConstant->fitsInt()) {
                        error("@size should be an integer for declaration %1%", value_set);
                        return nullptr;
                    }
                } else {
                    ::warning(ErrorType::WARN_MISSING,
                              "%1%: parser_value_set has no @parser_value_set_size annotation."
                              "Using default size 4.",
                              c);
                    sizeConstant = new IR::Constant(4);
                }
                auto annos = addNameAnnotation(value_set->name, value_set->annotations);
                // TODO: work around for the semantic difference bwteen
                // P4-14 value_set and P4-16 value_set. In p4-14, a value_set
                // can be global and share the same name in ingress add egress
                // parser. In p4-16, a value_set is local to a parser and can
                // not have the same global name.
                // This annotation forces the pvs in ingress parser and egress
                // parser to have the same name in context json to generate the
                // correct pd.h api to be consistent with existing p4-14 pd
                // tests.
                annos =
                    annos->addAnnotation("pd_pvs_name"_cs, new IR::StringLiteral(value_set->name));
                auto decl = new IR::P4ValueSet(value_set->name, annos, type, sizeConstant);
                stateful->push_back(decl);
            }
            for (auto v : c->values) {
                if (auto first = v.first->to<IR::Constant>()) {
                    auto expr = ProgramStructure::explodeLabel(first, v.second, fieldTypes);
                    auto sc = new IR::SelectCase(c->srcInfo, expr, deststate);
                    cases.push_back(sc);
                } else {
                    auto sc = new IR::SelectCase(c->srcInfo, v.first, deststate);
                    cases.push_back(sc);
                }
            }
        }
        select = new IR::SelectExpression(parser->select->srcInfo, list, std::move(cases));
    } else if (!parser->default_return.name.isNullOrEmpty()) {
        IR::ID id = parser->default_return;
        select = getState(id);
        if (select == nullptr) return nullptr;
    } else {
        BUG("No select or default_return %1%", parser);
    }
    latest = nullptr;
    auto annos = addNameAnnotation(parser->name, parser->annotations);
    auto result = new IR::ParserState(parser->srcInfo, parser->name, annos, components, select);
    return result;
}

void TnaProgramStructure::createIngressParser() {
    auto *paramList = new IR::ParameterList;
    ordered_map<cstring, cstring> tnaParams;

    auto pinpath = new IR::Path("packet_in");
    auto pintype = new IR::Type_Name(pinpath);
    parserPacketIn = new IR::Parameter("pkt"_cs, IR::Direction::None, pintype);
    tnaParams.emplace("pkt"_cs, "pkt"_cs);
    paramList->push_back(parserPacketIn);

    auto headpath = new IR::Path("headers");
    auto headtype = new IR::Type_Name(headpath);
    parserHeadersOut = new IR::Parameter("hdr"_cs, IR::Direction::Out, headtype);
    tnaParams.emplace("hdr"_cs, "hdr"_cs);
    paramList->push_back(parserHeadersOut);
    conversionContext->header = paramReference(parserHeadersOut);

    auto metapath = new IR::Path("metadata");
    auto metatype = new IR::Type_Name(metapath);
    auto meta = new IR::Parameter("meta"_cs, IR::Direction::Out, metatype);
    tnaParams.emplace("meta"_cs, "meta"_cs);
    paramList->push_back(meta);
    conversionContext->userMetadata = paramReference(meta);

    // add ig_intr_md
    auto md_intr_path = new IR::Path("ingress_intrinsic_metadata_t");
    auto md_intr_type = new IR::Type_Name(md_intr_path);
    auto md_intr = new IR::Parameter("ig_intr_md"_cs, IR::Direction::Out, md_intr_type);
    tnaParams.emplace("ig_intr_md"_cs, "ig_intr_md"_cs);
    paramList->push_back(md_intr);

    // add ig_intr_md_for_tm
    auto intr_md_for_tm_path = new IR::Path("ingress_intrinsic_metadata_for_tm_t");
    auto intr_md_for_tm_type = new IR::Type_Name(intr_md_for_tm_path);
    auto intr_md_for_tm =
        new IR::Parameter("ig_intr_md_for_tm"_cs, IR::Direction::Out, intr_md_for_tm_type);
    tnaParams.emplace("ig_intr_md_for_tm"_cs, "ig_intr_md_for_tm"_cs);
    paramList->push_back(intr_md_for_tm);

    // add ig_intr_md_from_prsr
    auto intr_md_from_prsr_path = new IR::Path("ingress_intrinsic_metadata_from_parser_t");
    auto intr_md_from_prsr_type = new IR::Type_Name(intr_md_from_prsr_path);
    auto intr_md_from_prsr =
        new IR::Parameter("ig_intr_md_from_prsr"_cs, IR::Direction::Out, intr_md_from_prsr_type);
    tnaParams.emplace("ig_intr_md_from_prsr"_cs, "ig_intr_md_from_prsr"_cs);
    paramList->push_back(intr_md_from_prsr);

    auto parser_type = new IR::Type_Parser("IngressParserImpl"_cs, paramList);

    IR::IndexedVector<IR::Declaration> stateful;
    auto states = new IR::IndexedVector<IR::ParserState>();

    ordered_map<cstring, ordered_set<cstring>> transitions;
    for (auto p : parserStates) {
        forAllMatching<IR::CaseEntry>(p.first, [p, &transitions](const IR::CaseEntry *sc) {
            transitions[p.first->name.name].insert(sc->action);
        });
        if (p.first->default_return.name != nullptr) {
            transitions[p.first->name].insert(p.first->default_return);
        }
    }

    ordered_map<cstring, ordered_set<cstring>> reachable_states;
    // compute all states reachable from 'state'
    auto compute_reachable_states = [&](cstring state) {
        ordered_set<cstring> worklist;
        for (auto t : transitions[state]) worklist.insert(t);
        while (!worklist.empty()) {
            auto w = worklist.front();
            reachable_states[state].insert(w);
            auto next = transitions[w];
            for (auto n : next) reachable_states[state].insert(n);
            worklist.erase(w);
        }
    };

    compute_reachable_states("start"_cs);
    compute_reachable_states("start_i2e_mirrored"_cs);
    compute_reachable_states("start_e2e_mirrored"_cs);

    for (auto p : parserStates) {
        auto state = convertParser(p.first, &stateful);
        if (state == nullptr) return;
        if (state->name == "start") {
            state = BFN::convertStartStateToNormalState(state, "ingress_p4_entry_point"_cs);
        }
        if (state->name == "start_i2e_mirrored" || state->name == "start_e2e_mirrored") continue;

        if (reachable_states["start_i2e_mirrored"_cs].count(state->name) &&
            !reachable_states["start"_cs].count(state->name)) {
            continue;
        }

        if (reachable_states["start_e2e_mirrored"_cs].count(state->name) &&
            !reachable_states["start"_cs].count(state->name))
            continue;

        states->push_back(state);
    }

    if (states->empty()) error("No parsers specified");

    states->append(*createIngressParserStates());

    // create csum and csum.add statements
    createChecksumVerifyStatements(INGRESS);

    auto ip = new IR::P4Parser("IngressParserImpl"_cs, parser_type, stateful, *states);
    declarations->push_back(ip);

    if (ingressReference.name.isNullOrEmpty())
        error("No transition from a parser to ingress pipeline found");
}

void TnaProgramStructure::createEgressParser() {
    auto *paramList = new IR::ParameterList;
    ordered_map<cstring, cstring> tnaParams;

    auto pinpath = new IR::Path("packet_in");
    auto pintype = new IR::Type_Name(pinpath);
    parserPacketIn = new IR::Parameter("pkt"_cs, IR::Direction::None, pintype);
    tnaParams.emplace("pkt"_cs, "pkt"_cs);
    paramList->push_back(parserPacketIn);

    auto headpath = new IR::Path("headers");
    auto headtype = new IR::Type_Name(headpath);
    parserHeadersOut = new IR::Parameter("hdr"_cs, IR::Direction::Out, headtype);
    tnaParams.emplace("hdr"_cs, "hdr"_cs);
    paramList->push_back(parserHeadersOut);
    conversionContext->header = paramReference(parserHeadersOut);

    auto metapath = new IR::Path("metadata");
    auto metatype = new IR::Type_Name(metapath);
    auto meta = new IR::Parameter("meta"_cs, IR::Direction::Out, metatype);
    tnaParams.emplace("meta"_cs, "meta"_cs);
    paramList->push_back(meta);
    conversionContext->userMetadata = paramReference(meta);

    // add eg_intr_md
    auto md_intr_path = new IR::Path("egress_intrinsic_metadata_t");
    auto md_intr_type = new IR::Type_Name(md_intr_path);
    auto md_intr = new IR::Parameter("eg_intr_md"_cs, IR::Direction::Out, md_intr_type);
    tnaParams.emplace("eg_intr_md"_cs, "eg_intr_md"_cs);
    paramList->push_back(md_intr);

    // add eg_intr_md_from_prsr
    auto md_path = new IR::Path("egress_intrinsic_metadata_from_parser_t");
    auto md_type = new IR::Type_Name(md_path);
    auto md_param = new IR::Parameter("eg_intr_md_from_parser_aux"_cs, IR::Direction::Out, md_type);
    tnaParams.emplace("eg_intr_md_from_parser_aux"_cs, md_param->name);
    paramList->push_back(md_param);

    auto parser_type = new IR::Type_Parser("EgressParserImpl"_cs, paramList);

    IR::IndexedVector<IR::Declaration> stateful;
    auto states = new IR::IndexedVector<IR::ParserState>();
    P4::CloneExpressions cloner;
    for (auto p : parserStates) {
        auto state = convertParser(p.first, &stateful);
        if (state == nullptr) return;
        if (state->name == "start")
            state = BFN::convertStartStateToNormalState(state, "egress_p4_entry_point"_cs);
        states->push_back(state);
    }
    if (states->empty()) error("No parsers specified");

    states->append(*createEgressParserStates());

    auto ep = new IR::P4Parser("EgressParserImpl"_cs, parser_type, stateful, *states);
    declarations->push_back(ep);

    if (ingressReference.name.isNullOrEmpty())
        error("No transition from a parser to ingress pipeline found");
}

IR::IndexedVector<IR::ParserState> *TnaProgramStructure::createIngressParserStates() {
    auto states = new IR::IndexedVector<IR::ParserState>();
    // Add a state that skips over any padding between the phase 0 data and the
    // beginning of the packet.
    const auto bitSkip = Device::pardeSpec().bitIngressPrePacketPaddingSize();
    auto *skipToPacketState = BFN::createGeneratedParserState(
        "skip_to_packet"_cs, {BFN::createAdvanceCall("pkt"_cs, bitSkip)},
        "__ingress_p4_entry_point"_cs);
    parsers.add("__skip_to_packet"_cs);
    parsers.calls("__skip_to_packet"_cs, "__ingress_p4_entry_point"_cs);
    states->push_back(skipToPacketState);

    const auto bitPhase0Size = Device::pardeSpec().bitPhase0Size();
    auto *phase0State = BFN::createGeneratedParserState(
        "phase0"_cs, {BFN::createAdvanceCall("pkt"_cs, bitPhase0Size)}, skipToPacketState->name);
    parsers.add("__phase0"_cs);
    parsers.calls("__phase0"_cs, "__skip_to_packet"_cs);
    states->push_back(phase0State);

    auto resubmitStates = createResubmitStates();
    states->append(*resubmitStates);

    // If this is a resubmitted packet, the initial intrinsic metadata will be
    // followed by the resubmit data; otherwise, it's followed by the phase 0
    // data. This state checks the resubmit flag and branches accordingly.
    IR::Vector<IR::Expression> selectOn = {
        new IR::Member(new IR::PathExpression("ig_intr_md"), "resubmit_flag"_cs)};
    auto *checkResubmitState = BFN::createGeneratedParserState(
        "check_resubmit"_cs, {},
        new IR::SelectExpression(new IR::ListExpression(selectOn),
                                 {BFN::createSelectCase(1, 0x0, 0x1, phase0State),
                                  BFN::createSelectCase(1, 0x1, 0x1, resubmitStates->back())}));
    parsers.add("__check_resubmit"_cs);
    parsers.calls("__check_resubmit"_cs, "__phase0"_cs);
    parsers.calls("__check_resubmit"_cs, "__resubmit"_cs);
    states->push_back(checkResubmitState);

    // This state handles the extraction of ingress intrinsic metadata.
    IR::IndexedVector<IR::StatOrDecl> statements;
    statements.push_back(
        BFN::createExtractCall("pkt"_cs, "ingress_intrinsic_metadata_t"_cs, "ig_intr_md"_cs));

    auto *igMetadataState = BFN::createGeneratedParserState(
        "ingress_metadata"_cs, std::move(statements), checkResubmitState->name);
    parsers.add("__ingress_metadata"_cs);
    parsers.calls("__ingress_metadata"_cs, "__check_resubmit"_cs);
    states->push_back(igMetadataState);

    states->push_back(BFN::addNewStartState("ingress_tna_entry_point"_cs, "__ingress_metadata"_cs));
    parsers.add("start"_cs);
    parsers.calls("start"_cs, "__ingress_metadata"_cs);
    return states;
}

const IR::ParserState *TnaProgramStructure::createEmptyMirrorState(cstring nextState) {
    // Add a state that skips over compiler generated byte
    auto *skipToPacketState =
        BFN::createGeneratedParserState("mirrored"_cs, {BFN::createAdvanceCall("pkt"_cs, 8)},
                                        new IR::PathExpression(IR::ID(nextState)));
    return skipToPacketState;
}

const IR::ParserState *TnaProgramStructure::createMirrorState(gress_t gress, unsigned index,
                                                              const IR::Expression *expr,
                                                              cstring headerType,
                                                              cstring nextState) {
    auto statements = new IR::IndexedVector<IR::StatOrDecl>();
    auto tmp = cstring::to_cstring(gress) + "_mirror_" + std::to_string(index);
    auto decl = new IR::Declaration_Variable(IR::ID(tmp), new IR::Type_Name(headerType));
    statements->push_back(decl);
    statements->push_back(BFN::createExtractCall("pkt"_cs, headerType, tmp));

    // Construct a `clone_src` value. The first bit indicates that the
    // packet is mirrored; the second bit indicates whether it originates
    // from ingress or egress. See `constants.p4` for the details of the
    // format.
    unsigned source = 1 << 0;
    if (gress == EGRESS) source |= 1 << 1;

    statements->push_back(
        BFN::createSetMetadata("meta"_cs, COMPILER_META, "clone_src"_cs, 4, source));

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
    statements->push_back(
        BFN::createSetMetadata("meta"_cs, COMPILER_META, "mirror_source"_cs, 8, mirror_source));

    P4::CloneExpressions cloner;
    for (auto e : expr->to<IR::ListExpression>()->components) {
        if (e->to<IR::ListExpression>()) {
            BUG("\tnested field list");
            // FIXME: handle nested field list.
        } else {
            auto linearizer = new BFN::PathLinearizer();
            e->apply(*linearizer);
            auto path = *linearizer->linearPath;
            auto fn = convertLinearPathToStructFieldName(&path);
            statements->push_back(BFN::createSetMetadata(e->apply(cloner), tmp, fn));
        }
    }

    // Create a state that extracts the fields in this field list.
    cstring name = "parse_"_cs + cstring::to_cstring(gress) + "_mirror_header_"_cs +
                   cstring::to_cstring(index);

    auto select = new IR::PathExpression(IR::ID(nextState));
    auto newStateName = IR::ID(cstring("__") + name);
    auto *newState = new IR::ParserState(newStateName, *statements, select);
    newState->annotations = newState->annotations->addAnnotationIfNew(
        IR::Annotation::nameAnnotation, new IR::StringLiteral(cstring(cstring("$") + name)));
    return newState;
}

const IR::SelectCase *TnaProgramStructure::createSelectCase(gress_t gress, unsigned digestId,
                                                            const IR::ParserState *newState) {
    unsigned source = 1 << 0;
    if (gress == EGRESS) source |= 1 << 1;

    const unsigned fieldListId = (source << 3) | digestId;
    auto selectCase = BFN::createSelectCase(8, fieldListId, 0x1f, newState);
    return selectCase;
}

IR::IndexedVector<IR::ParserState> *TnaProgramStructure::createMirrorStates() {
    auto states = new IR::IndexedVector<IR::ParserState>();
    auto selectCases = new IR::Vector<IR::SelectCase>();

    // if mirror metadata is needed
    for (auto d : digestFieldLists["clone_ingress_pkt_to_egress"_cs]) {
        auto hdrName = "ig_mirror_header_" + cstring::to_cstring(d.first) + "_t";
        auto nextState = createMirrorState(INGRESS, d.first, d.second.second, hdrName,
                                           "__egress_p4_entry_point"_cs);
        states->push_back(nextState);
        auto selectCase = createSelectCase(INGRESS, d.first, nextState);
        selectCases->push_back(selectCase);
    }

    for (auto d : digestFieldLists["clone_egress_pkt_to_egress"_cs]) {
        auto hdrName = "eg_mirror_header_" + cstring::to_cstring(d.first) + "_t";
        auto nextState = createMirrorState(EGRESS, d.first, d.second.second, hdrName,
                                           "__egress_p4_entry_point"_cs);
        states->push_back(nextState);
        auto selectCase = createSelectCase(EGRESS, d.first, nextState);
        selectCases->push_back(selectCase);
    }

    if (!selectCases->empty()) {
        auto selectOn = new IR::ListExpression({BFN::createLookaheadExpr("pkt"_cs, 8)});
        auto *mirroredState = BFN::createGeneratedParserState(
            "mirrored"_cs, {}, new IR::SelectExpression(selectOn, *selectCases));
        states->push_back(mirroredState);
    } else {
        auto *mirroredState =
            BFN::createGeneratedParserState("mirrored"_cs, {}, "__egress_p4_entry_point"_cs);
        states->push_back(mirroredState);
    }
    return states;
}

const IR::ParserState *TnaProgramStructure::createResubmitState(gress_t gress, unsigned index,
                                                                const IR::Expression *expr,
                                                                cstring headerType,
                                                                cstring nextState) {
    auto statements = new IR::IndexedVector<IR::StatOrDecl>();
    auto tmp = cstring::to_cstring(gress) + "_resubmit_" + std::to_string(index);
    auto decl = new IR::Declaration_Variable(IR::ID(tmp), new IR::Type_Name(headerType));
    statements->push_back(decl);
    statements->push_back(BFN::createExtractCall("pkt"_cs, headerType, tmp));

    // Create a state that extracts the fields in this field list.
    cstring name = "parse_"_cs + cstring::to_cstring(gress) + "_resubmit_header_"_cs +
                   cstring::to_cstring(index);

    // Construct a value for `resubmit_source`, which is
    // compiler-generated metadata that's prepended to the user field
    // list. Its layout (in network order) is:
    //   [  0    1       2          3         4      5     6     7 ]
    //      [                 unused          ]     [resubmit_type]
    //
    // TODO: PHV should be able to overlay this field
    // with resubmit_type.
    unsigned resubmit_source = index;
    statements->push_back(
        BFN::createSetMetadata("meta"_cs, COMPILER_META, "resubmit_source"_cs, 8, resubmit_source));

    P4::CloneExpressions cloner;
    for (auto e : expr->to<IR::ListExpression>()->components) {
        if (e->to<IR::ListExpression>()) {
            BUG("\tnested field list");
            // FIXME: handle nested field list.
        } else {
            auto linearizer = new BFN::PathLinearizer();
            e->apply(*linearizer);
            auto path = *linearizer->linearPath;
            auto fn = convertLinearPathToStructFieldName(&path);
            statements->push_back(BFN::createSetMetadata(e->apply(cloner), tmp, fn));
        }
    }

    auto select = new IR::PathExpression(IR::ID(nextState));
    auto newStateName = IR::ID(cstring("__") + name);
    auto *newState = new IR::ParserState(newStateName, *statements, select);
    newState->annotations = newState->annotations->addAnnotationIfNew(
        IR::Annotation::nameAnnotation, new IR::StringLiteral(cstring(cstring("$") + name)));
    return newState;
}

IR::IndexedVector<IR::ParserState> *TnaProgramStructure::createResubmitStates() {
    auto states = new IR::IndexedVector<IR::ParserState>();
    auto selectCases = new IR::Vector<IR::SelectCase>();

    for (auto d : digestFieldLists["resubmit"_cs]) {
        auto hdrName = "resubmit_header_" + cstring::to_cstring(d.first) + "_t";
        auto nextState = createResubmitState(INGRESS, d.first, d.second.second, hdrName,
                                             "__ingress_p4_entry_point"_cs);
        states->push_back(nextState);
        auto selectCase = BFN::createSelectCase(8, d.first, 0x07, nextState);
        selectCases->push_back(selectCase);
    }

    if (!selectCases->empty()) {
        auto selectOn = new IR::ListExpression({BFN::createLookaheadExpr("pkt"_cs, 8)});
        auto *resubmitState = BFN::createGeneratedParserState(
            "resubmit"_cs, {}, new IR::SelectExpression(selectOn, *selectCases));
        states->push_back(resubmitState);
    } else {
        auto *resubmitState =
            BFN::createGeneratedParserState("resubmit"_cs, {}, "__ingress_p4_entry_point"_cs);
        states->push_back(resubmitState);
    }

    return states;
}

IR::IndexedVector<IR::ParserState> *TnaProgramStructure::createEgressParserStates() {
    auto states = new IR::IndexedVector<IR::ParserState>();
    P4::CloneExpressions cloner;

    // if bridge metadata is needed
    IR::IndexedVector<IR::StatOrDecl> bridgeStateStmts;
    if (useBridgeMetadata()) {
        auto member = new IR::Member(new IR::PathExpression("meta"), IR::ID("bridged_header"));
        bridgeStateStmts.push_back(BFN::createExtractCall("pkt"_cs, "bridged_header_t"_cs, member));

        for (auto f : bridgedFields) {
            if (!bridgedFieldInfo.count(f)) continue;
            auto info = bridgedFieldInfo.at(f);
            auto fn = convertLinearPathToStructFieldName(&info);
            auto *member = new IR::Member(
                new IR::Member(new IR::PathExpression("meta"), IR::ID("bridged_header")),
                IR::ID(fn));
            auto *bridgedMember = info.components.back()->apply(cloner);
            bridgedMember = bridgedMember->apply(ConvertConcreteHeaderRefToPathExpression());
            auto *assignment = new IR::AssignmentStatement(bridgedMember, member);
            bridgeStateStmts.push_back(assignment);
        }
    }

    auto *bridgedMetadataState =
        BFN::createGeneratedParserState("bridged_metadata"_cs, std::move(bridgeStateStmts),
                                        "__egress_p4_entry_point"_cs);  // FIXME: start_egress
    parsers.add("__bridged_metadata"_cs);
    parsers.calls("__bridged_metadata"_cs, "__egress_p4_entry_point"_cs);
    states->push_back(bridgedMetadataState);

    auto mirroredStates = createMirrorStates();
    states->append(*mirroredStates);

    // check_mirrored
    auto selectOn = new IR::ListExpression({BFN::createLookaheadExpr("pkt"_cs, 8)});
    auto *checkMirroredState = BFN::createGeneratedParserState(
        "check_mirrored"_cs, {},
        new IR::SelectExpression(
            selectOn, {BFN::createSelectCase(8, 0, 1 << 3, bridgedMetadataState),
                       BFN::createSelectCase(8, 1 << 3, 1 << 3, mirroredStates->back())}));
    parsers.add("__check_mirrored"_cs);
    parsers.calls("__check_mirrored"_cs, "__bridged_metadata"_cs);
    parsers.calls("__check_mirrored"_cs, "__mirrored"_cs);
    states->push_back(checkMirroredState);

    auto *egMetadataState = BFN::createGeneratedParserState(
        "egress_metadata"_cs,
        {BFN::createExtractCall("pkt"_cs, "egress_intrinsic_metadata_t"_cs, "eg_intr_md"_cs)},
        "__check_mirrored"_cs);
    parsers.add("__egress_metadata"_cs);
    parsers.calls("__egress_metadata"_cs, "__check_mirrored"_cs);
    states->push_back(egMetadataState);

    states->push_back(BFN::addNewStartState("egress_tna_entry_point"_cs, "__egress_metadata"_cs));
    parsers.add("start"_cs);
    parsers.calls("start"_cs, "__egress_metadata"_cs);
    return states;
}

void TnaProgramStructure::createParser() {
    createIngressParser();
    createEgressParser();
}

// add 'mirror_source' to the beginning of the field list
const IR::StatOrDecl *TnaProgramStructure::createDigestEmit(
    cstring headerType, unsigned index,
    std::pair<std::optional<cstring>, const IR::Expression *> field_list, cstring intr_md,
    cstring ext_name, cstring selector) {
    const IR::Member *member;
    auto typeArgs = new IR::Vector<IR::Type>();
    if (ext_name == "digest") {
        auto pathExpr = new IR::PathExpression(
            new IR::Path((ext_name == "digest") ? *field_list.first : ext_name));
        member = new IR::Member(pathExpr, (ext_name == "digest") ? "pack" : "emit");
    } else {
        auto pathExpr = new IR::PathExpression(new IR::Path(ext_name));
        member = new IR::Member(pathExpr, "emit");
        auto typeName = headerType + cstring::to_cstring(index) + "_t";
        typeArgs->push_back(new IR::Type_Name(IR::ID(typeName)));
    }

    auto args = new IR::Vector<IR::Argument>();
    if (ext_name == "mirror")
        args->push_back(new IR::Argument(new IR::Member(
            new IR::Member(new IR::PathExpression("meta"), COMPILER_META), "mirror_id")));
    auto expr = field_list.second;
    BUG_CHECK(expr->is<IR::ListExpression>(), "expect digest field list to be a list expression");
    auto list = expr->to<IR::ListExpression>();
    auto components = new IR::Vector<IR::Expression>();
    if (ext_name == "mirror")
        components->push_back(new IR::Member(
            new IR::Member(new IR::PathExpression("meta"), COMPILER_META), "mirror_source"_cs));
    else if (ext_name == "resubmit")
        components->push_back(new IR::Member(
            new IR::Member(new IR::PathExpression("meta"), COMPILER_META), "resubmit_source"_cs));
    components->append(list->components);
    auto fl = new IR::ListExpression(*components);
    args->push_back(new IR::Argument(fl));

    auto mcs = new IR::MethodCallStatement(new IR::MethodCallExpression(member, typeArgs, args));
    auto condExprPath = new IR::Member(new IR::PathExpression(IR::ID(intr_md)), selector);
    auto condExpr = new IR::Equ(condExprPath, new IR::Constant(index));
    auto cond = new IR::IfStatement(condExpr, mcs, nullptr);

    return cond;
}

const IR::StatOrDecl *TnaProgramStructure::createBridgeEmit() {
    auto pathExpr = new IR::PathExpression(new IR::Path("pkt"));
    auto member = new IR::Member(pathExpr, "emit");
    auto args = new IR::Vector<IR::Argument>();
    args->push_back(
        new IR::Argument(new IR::Member(new IR::PathExpression("meta"), "bridged_header"_cs)));
    auto stmt = new IR::MethodCallStatement(new IR::MethodCallExpression(member, args));
    return stmt;
}

/**
 */
void TnaProgramStructure::createIngressDeparser() {
    auto poutpath = new IR::Path(p4lib.packetOut.Id());
    auto pouttype = new IR::Type_Name(poutpath);
    auto packetOut = new IR::Parameter("pkt"_cs, IR::Direction::None, pouttype);

    auto headpath = new IR::Path("headers");
    auto headtype = new IR::Type_Name(headpath);
    auto headers = new IR::Parameter("hdr"_cs, IR::Direction::InOut, headtype);
    conversionContext->header = paramReference(headers);

    std::vector<IR::Parameter *> paramList;
    auto metapath = new IR::Path("metadata");
    auto metatype = new IR::Type_Name(metapath);
    auto meta = new IR::Parameter("meta"_cs, IR::Direction::In, metatype);
    paramList.push_back(meta);

    conversionContext->userMetadata = paramReference(meta);

    // add deparser intrinsic metadata
    auto path = new IR::Path("ingress_intrinsic_metadata_for_deparser_t");
    auto type = new IR::Type_Name(path);
    auto param = new IR::Parameter("ig_intr_md_for_dprsr"_cs, IR::Direction::In, type);
    paramList.push_back(param);

    // add intrinsic metadata
    auto md_path = new IR::Path("ingress_intrinsic_metadata_t");
    auto md_type = new IR::Type_Name(md_path);
    auto md_param = new IR::Parameter("ig_intr_md"_cs, IR::Direction::In, md_type);
    paramList.push_back(md_param);

    IR::IndexedVector<IR::Declaration> controlLocals;
    if (digestFieldLists["resubmit"_cs].size() != 0) {
        auto declArgs = new IR::Vector<IR::Argument>({});
        auto declType = new IR::Type_Name("Resubmit");
        auto decl = new IR::Declaration_Instance("resubmit"_cs, declType, declArgs);
        controlLocals.push_back(decl);
    }
    if (digestFieldLists["clone_ingress_pkt_to_egress"_cs].size() != 0) {
        auto declArgs = new IR::Vector<IR::Argument>({});
        auto declType = new IR::Type_Name("Mirror");
        auto decl = new IR::Declaration_Instance("mirror"_cs, declType, declArgs);
        controlLocals.push_back(decl);
    }
    for (auto digest : digestFieldLists["generate_digest"_cs]) {
        auto declArgs = new IR::Vector<IR::Argument>({});
        auto typeName = "digest_header_" + cstring::to_cstring(digest.first) + "_t";
        auto typeArgs = new IR::Vector<IR::Type>();
        typeArgs->push_back(new IR::Type_Name(IR::ID(typeName)));
        auto declType = new IR::Type_Specialized(new IR::Type_Name("Digest"), typeArgs);
        BUG_CHECK(digest.second.first != std::nullopt, "digest field list name cannot be empty");
        auto fl_name = *digest.second.first;
        auto annot = new IR::Annotations();
        auto annos = addGlobalNameAnnotation(fl_name, annot);
        auto decl = new IR::Declaration_Instance(fl_name, annos, declType, declArgs);
        controlLocals.push_back(decl);
    }

    // create csum and csum.update statements;
    std::vector<ChecksumUpdateInfo> checksumUpdates;
    createChecksumUpdateStatements(INGRESS, controlLocals, checksumUpdates);

    auto addAllEmits = [this, checksumUpdates](IR::BlockStatement *blk) {
        auto headerEmits = blk->components;
        auto allEmits = new IR::IndexedVector<IR::StatOrDecl>();
        for (const auto &d : digestFieldLists["clone_ingress_pkt_to_egress"_cs]) {
            auto cond = createDigestEmit("ig_mirror_header_"_cs, d.first, d.second,
                                         "ig_intr_md_for_dprsr"_cs, "mirror"_cs, "mirror_type"_cs);
            allEmits->push_back(cond);
        }
        for (const auto &d : digestFieldLists["resubmit"_cs]) {
            auto cond =
                createDigestEmit("resubmit_header_"_cs, d.first, d.second,
                                 "ig_intr_md_for_dprsr"_cs, "resubmit"_cs, "resubmit_type"_cs);
            allEmits->push_back(cond);
        }
        for (const auto &d : digestFieldLists["generate_digest"_cs]) {
            auto cond = createDigestEmit("digest_header_"_cs, d.first, d.second,
                                         "ig_intr_md_for_dprsr"_cs, "digest"_cs, "digest_type"_cs);
            allEmits->push_back(cond);
        }
        // insert checksum update statement if deparserUpdateLocations contains ingress
        for (auto csum : checksumUpdates) {
            if (csum.second.deparserUpdateLocations.count(INGRESS)) {
                allEmits->push_back(csum.first);
            }
        }
        if (useBridgeMetadata()) {
            allEmits->push_back(createBridgeEmit());
        }
        allEmits->append(headerEmits);
        return new IR::BlockStatement(*allEmits);
    };

    createDeparserInternal("IngressDeparserImpl"_cs, packetOut, headers, paramList, controlLocals,
                           addAllEmits);
}

void TnaProgramStructure::parseUpdateLocationAnnotation(std::set<gress_t> &updateLocations,
                                                        const IR::Annotation *annot,
                                                        cstring pragma) {
    if (annot->name.name == pragma) {
        auto &exprs = annot->expr;
        auto gress = exprs[0]->to<IR::StringLiteral>();

        if (gress->value == "ingress")
            updateLocations = {INGRESS};
        else if (gress->value == "egress")
            updateLocations = {EGRESS};
        else if (gress->value == "ingress_and_egress")
            updateLocations = {INGRESS, EGRESS};
        else
            error(
                "Invalid use of @pragma %1%, valid value "
                " is ingress/egress/ingress_and_egress",
                pragma);
    }
}

void TnaProgramStructure::createChecksumUpdateStatements(
    gress_t gress, IR::IndexedVector<IR::Declaration> &controlLocals,
    std::vector<ChecksumUpdateInfo> &checksumUpdateInfo) {
    ExpressionConverter conv(this);

    forAllChecksums(calculated_fields, [&](const IR::CalculatedField *cf,
                                           const IR::FieldListCalculation *flc,
                                           const IR::CalculatedField::update_or_verify uov,
                                           const IR::FieldList *fl) {
        auto dest = conv.convert(cf->field);
        auto le = conv.convert(fl);

        auto checksumInstName = makeUniqueName("checksum"_cs);
        auto checksumInst = new IR::Declaration_Instance(
            checksumInstName, new IR::Type_Name("Checksum"), new IR::Vector<IR::Argument>());
        auto method = new IR::Member(new IR::PathExpression(checksumInstName), IR::ID("update"));
        auto args = new IR::Vector<IR::Argument>();
        args->push_back(new IR::Argument(le));

        for (auto name : flc->algorithm->names)
            if (name == "csum16_udp") args->push_back(new IR::Argument(new IR::BoolLiteral(1)));

        IR::Statement *mc = nullptr;
        mc = new IR::AssignmentStatement(dest, new IR::MethodCallExpression(method, args));

        const IR::Expression *condition = nullptr;
        if (uov.cond != nullptr) condition = conv.convert(uov.cond);
        if (condition) mc = new IR::IfStatement(condition, mc, nullptr);

        ChecksumInfo cinfo;
        cinfo.cond = condition;
        cinfo.fieldList = le;
        cinfo.destField = dest;
        cinfo.parserUpdateLocations.insert(INGRESS);
        cinfo.deparserUpdateLocations.insert(EGRESS);

        // annotation may override the default updateLocations
        for (auto annot : cf->annotations->annotations) {
            parseUpdateLocationAnnotation(cinfo.parserUpdateLocations, annot,
                                          "residual_checksum_parser_update_location"_cs);
            parseUpdateLocationAnnotation(cinfo.deparserUpdateLocations, annot,
                                          "calculated_field_update_location"_cs);
        }

        for (auto annot : flc->annotations->annotations) {
            parseUpdateLocationAnnotation(cinfo.parserUpdateLocations, annot,
                                          "residual_checksum_parser_update_location"_cs);
            parseUpdateLocationAnnotation(cinfo.deparserUpdateLocations, annot,
                                          "calculated_field_update_location"_cs);
        }
        if (fl->payload) {
            if (cinfo.parserUpdateLocations.count(gress)) {
                cinfo.with_payload = fl->payload;
                if (residualChecksumNames.count(fl))
                    cinfo.residulChecksumName = residualChecksumNames.at(fl);
                residualChecksums.push_back(cinfo);
            }
        }

        if (cinfo.deparserUpdateLocations.count(gress)) controlLocals.push_back(checksumInst);
        checksumUpdateInfo.push_back(std::make_pair(mc, cinfo));
    });
}

void TnaProgramStructure::createChecksumVerifyStatements(gress_t) {
    ExpressionConverter conv(this);
    for (auto cf : calculated_fields) {
        LOG3("Converting " << cf);
        auto dest = conv.convert(cf->field);

        for (auto uov : cf->specs) {
            if (uov.update) continue;
            auto flc = field_list_calculations.get(uov.name.name);

            auto fl = getFieldLists(flc);
            if (fl == nullptr) continue;
            auto le = conv.convert(fl);
            const IR::Expression *condition;
            if (uov.cond != nullptr)
                condition = conv.convert(uov.cond);
            else
                condition = new IR::BoolLiteral(true);

            ChecksumInfo cinfo;
            cinfo.with_payload = fl->payload;
            cinfo.cond = condition;
            cinfo.fieldList = le;
            cinfo.destField = dest;
            cinfo.parserUpdateLocations.insert(INGRESS);
            cinfo.deparserUpdateLocations.insert(EGRESS);

            // annotation may override the default updateLocations
            for (auto annot : cf->annotations->annotations) {
                parseUpdateLocationAnnotation(cinfo.parserUpdateLocations, annot,
                                              "residual_checksum_parser_update_location"_cs);
                parseUpdateLocationAnnotation(cinfo.deparserUpdateLocations, annot,
                                              "calculated_field_update_location"_cs);
            }

            for (auto annot : flc->annotations->annotations) {
                parseUpdateLocationAnnotation(cinfo.parserUpdateLocations, annot,
                                              "residual_checksum_parser_update_location"_cs);
                parseUpdateLocationAnnotation(cinfo.deparserUpdateLocations, annot,
                                              "calculated_field_update_location"_cs);
            }

            verifyChecksums.push_back(cinfo);
        }
    }
    LOG3("created " << verifyChecksums.size() << " verifyChecksums");
}

void TnaProgramStructure::createEgressDeparser() {
    auto poutpath = new IR::Path(p4lib.packetOut.Id());
    auto pouttype = new IR::Type_Name(poutpath);
    auto packetOut = new IR::Parameter("pkt"_cs, IR::Direction::None, pouttype);

    auto headpath = new IR::Path("headers");
    auto headtype = new IR::Type_Name(headpath);
    auto headers = new IR::Parameter("hdr"_cs, IR::Direction::InOut, headtype);
    conversionContext->header = paramReference(headers);

    std::vector<IR::Parameter *> paramList;
    auto metapath = new IR::Path("metadata");
    auto metatype = new IR::Type_Name(metapath);
    auto meta = new IR::Parameter("meta"_cs, IR::Direction::In, metatype);
    paramList.push_back(meta);

    conversionContext->userMetadata = paramReference(meta);

    // add eg_intr_md_for_dprsr
    auto path = new IR::Path("egress_intrinsic_metadata_for_deparser_t");
    auto type = new IR::Type_Name(path);
    auto param = new IR::Parameter("eg_intr_md_for_dprsr"_cs, IR::Direction::In, type);
    paramList.push_back(param);

    // add eg_intr_md
    path = new IR::Path("egress_intrinsic_metadata_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("eg_intr_md"_cs, IR::Direction::In, type);
    paramList.push_back(param);

    // add eg_intr_md_from_prsr
    path = new IR::Path("egress_intrinsic_metadata_from_parser_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("eg_intr_md_from_parser_aux"_cs, IR::Direction::In, type);
    paramList.push_back(param);

    IR::IndexedVector<IR::Declaration> controlLocals;
    if (digestFieldLists["clone_egress_pkt_to_egress"_cs].size() != 0) {
        auto declArgs = new IR::Vector<IR::Argument>({});
        auto declType = new IR::Type_Name("Mirror");
        auto decl = new IR::Declaration_Instance("mirror"_cs, declType, declArgs);
        controlLocals.push_back(decl);
    }

    // create csum and csum.update statements;
    std::vector<ChecksumUpdateInfo> checksumUpdates;
    createChecksumUpdateStatements(EGRESS, controlLocals, checksumUpdates);

    auto addAllEmits = [this, checksumUpdates](IR::BlockStatement *blk) {
        auto headerEmits = blk->components;
        auto allEmits = new IR::IndexedVector<IR::StatOrDecl>();
        for (const auto &d : digestFieldLists["clone_egress_pkt_to_egress"_cs]) {
            LOG3("digest field list " << d.first << d.second.second);
            auto cond = createDigestEmit("eg_mirror_header_"_cs, d.first, d.second,
                                         "eg_intr_md_for_dprsr"_cs, "mirror"_cs, "mirror_type"_cs);
            allEmits->push_back(cond);
        }
        // insert checksum update statement if deparserUpdateLocations contains egress
        for (auto csum : checksumUpdates) {
            if (csum.second.deparserUpdateLocations.count(EGRESS)) {
                allEmits->push_back(csum.first);
            }
        }
        allEmits->append(headerEmits);
        return new IR::BlockStatement(*allEmits);
    };

    createDeparserInternal("EgressDeparserImpl"_cs, packetOut, headers, paramList, controlLocals,
                           addAllEmits);
}

void TnaProgramStructure::createDeparser() {
    createIngressDeparser();
    createEgressDeparser();
}

void TnaProgramStructure::collectControlGressMapping() {
    std::set<cstring> reachableFromIngress;
    calledControls.reachable("ingress"_cs, reachableFromIngress);
    for (cstring s : reachableFromIngress) {
        mapControlToGress.emplace(s, INGRESS);
        LOG5("\tcontrol " << s << " reachable in ingress");
    }
    std::set<cstring> reachableFromEgress;
    calledControls.reachable("egress"_cs, reachableFromEgress);
    for (cstring s : reachableFromEgress) {
        mapControlToGress.emplace(s, EGRESS);
        LOG5("\tcontrol " << s << " reachable in egress");
    }
}

cstring TnaProgramStructure::convertLinearPathToStructFieldName(BFN::LinearPath *path) {
    auto field = path->components.back()->to<IR::Member>();
    BUG_CHECK(field != nullptr, "Unexpected non-member field %1%", path->components.back());
    bool skip_path_expression = false;
    if (auto hdr = path->components.front()->to<IR::PathExpression>()) {
        if (hdr->path->name == "meta") skip_path_expression = true;
    }
    auto fn = path->to_cstring("_"_cs, skip_path_expression);
    return fn;
}

void TnaProgramStructure::collectBridgedFieldsInControls() {
    // collect bridge info in controls
    ordered_set<cstring> mayRead[2];
    ordered_set<cstring> mayWrite[2];

    std::set<cstring> reachableFromIngress;
    calledControls.reachable("ingress"_cs, reachableFromIngress);
    for (auto s : reachableFromIngress) {
        if (auto v1ctrl = controls.get(s)) {
            // convert to p4-16 because the WriteContext used in
            // CollectBridgedFields is implemented with p4-16 IR nodes.
            auto p4ctrl = ProgramStructure::convertControl(v1ctrl, s);
            p4ctrl = p4ctrl->apply(ConvertMetadata(this));
            p4ctrl->apply(CollectBridgedFields(INGRESS, mayWrite[INGRESS], mayRead[INGRESS],
                                               bridgedFieldInfo));
        } else {
            error("No ingress control in program");
            return;
        }
    }

    std::set<cstring> reachableFromEgress;
    calledControls.reachable("egress"_cs, reachableFromEgress);
    for (auto s : reachableFromEgress) {
        auto v1ctrl = controls.get(s);
        // p4-14 program may not have a 'egress' control
        if (!v1ctrl) continue;
        // convert to p4-16 because the WriteContext used in
        // CollectBridgedFields is implemented with p4-16 IR nodes.
        auto p4ctrl = ProgramStructure::convertControl(v1ctrl, s);
        p4ctrl = p4ctrl->apply(ConvertMetadata(this));
        p4ctrl->apply(
            CollectBridgedFields(EGRESS, mayWrite[EGRESS], mayRead[EGRESS], bridgedFieldInfo));
    }

    // A field must be bridged if it may be read uninitialized in egress and
    // it may be written in ingress.
    for (auto &field : mayRead[EGRESS]) {
        // mayWrite contains write in control
        if (mayWrite[INGRESS].count(field)) {
            // ignore field that comes from packet header.
            if (field.startsWith("hdr")) continue;
            bridgedFields.insert(field);
        }
    }
}

//
// This function is NOT idempotent. It depends on if the function is invoked
// after the collectBridgedFieldsInControls() and collectBridgedFieldsInParsers() call.
bool TnaProgramStructure::useBridgeMetadata() {
    bool useBridgedFields = !bridgedFields.empty();

    bool useResidualChecksum = false;
    forAllResidualChecksums(calculated_fields,
                            [&](const IR::FieldList *) { useResidualChecksum = true; });

    return useBridgedFields || useResidualChecksum;
}

// Egress parser is generated from ingress parser. Since the ingress parser
// could select on ig_intr_md, the generated egress parser must select on the
// copy of the same field that is bridged from ingress. Therefore, any read
// access to ig_intr_md in ingress parser will result in a bridged field.
void TnaProgramStructure::collectBridgedFieldsInParsers() {
    // collect bridge info in parsers
    ordered_set<cstring> mayRead[2];
    ordered_set<cstring> mayWrite[2];
    std::set<cstring> reachableFromStartState;
    parsers.reachable("start"_cs, reachableFromStartState);
    IR::IndexedVector<IR::Declaration> stateful;
    for (auto s : reachableFromStartState) {
        auto v1parser = parserStates.get(s);
        v1parser->apply(
            CollectBridgedFields(EGRESS, mayWrite[EGRESS], mayRead[EGRESS], bridgedFieldInfo));
    }

    for (auto &field : mayRead[EGRESS]) {
        if (field.startsWith("ig_intr_md")) bridgedFields.insert(field);
    }

    // standard metadata is special
    for (auto &field : mayRead[EGRESS]) {
        if (field.startsWith("standard_metadata")) {
            bridgedFields.insert(field);
        }
    }
}

void TnaProgramStructure::addBridgedFieldsFromChecksumUpdate(
    IR::IndexedVector<IR::StructField> *fields) {
    ExpressionConverter conv(this);
    auto headpath = new IR::Path(v1model.headersType.Id());
    auto headtype = new IR::Type_Name(headpath);
    // we can use ingress, since all control blocks have the same signature
    auto headers =
        new IR::Parameter(v1model.ingress.headersParam.Id(), IR::Direction::InOut, headtype);
    conversionContext->header = paramReference(headers);

    auto metapath = new IR::Path(v1model.metadataType.Id());
    auto metatype = new IR::Type_Name(metapath);
    auto meta =
        new IR::Parameter(v1model.ingress.metadataParam.Id(), IR::Direction::InOut, metatype);
    conversionContext->userMetadata = paramReference(meta);
    // corner case: checksum update condition is set in ingress and read in egress deparser
    // the condition must be bridged to egress.
    for (auto cf : calculated_fields) {
        for (auto uov : cf->specs) {
            if (!uov.update) continue;
            if (!uov.cond) continue;
            auto condition = conv.convert(uov.cond);
            if (auto op = condition->to<IR::Operation_Binary>()) {
                const IR::Expression *expr = nullptr;
                if (auto mem = op->left->to<IR::Member>())
                    expr = mem;
                else if (auto mem = op->right->to<IR::Member>())
                    expr = mem;
                BUG_CHECK(expr, "Invalid condition %s", uov.cond);
                auto linearizer = new BFN::PathLinearizer();
                expr->apply(*linearizer);
                auto path = *linearizer->linearPath;
                auto fn = convertLinearPathToStructFieldName(&path);
                auto type = expr->type;
                if (!bridgedFields.count(fn)) {
                    bridgedFields.insert(fn);
                    auto annot = new IR::Annotations({new IR::Annotation(IR::ID("flexible"), {})});
                    fields->push_back(new IR::StructField(IR::ID(fn), annot, type));
                    bridgedFieldInfo.emplace(fn, path);
                }
            }
        }
    }
}

// visit all control blocks, collect fields that are
// written in ingress and read in egress.
void TnaProgramStructure::collectBridgedFields() {
    LOG3("Collecting bridge info");
    collectBridgedFieldsInControls();
    collectBridgedFieldsInParsers();

    // handle pa_do_not_bridge
    for (auto it : headers) {
        auto do_not_bridge = it.first->type->annotations->getSingle("pa_do_not_bridge"_cs);
        if (do_not_bridge) {
            if (do_not_bridge->expr.size() != 2) continue;
            if (auto expr = do_not_bridge->expr.at(1)->to<IR::StringLiteral>()) {
                auto meta = "meta." + expr->value;
                if (bridgedFields.count(meta)) {
                    bridgedFields.erase(meta);
                }
            }
        }
    }

    if (!useBridgeMetadata()) return;

    auto fields = new IR::IndexedVector<IR::StructField>();
    fields->push_back(new IR::StructField("bridged_metadata_indicator"_cs, IR::Type::Bits::get(8)));

    int count = 0;
    forAllResidualChecksums(calculated_fields, [&](const IR::FieldList *fl) {
        LOG3("residual checksum " << fl);
        auto residualFieldName = "residual_checksum_" + cstring::to_cstring(count++);
        bridgedFields.insert(residualFieldName);
        fields->push_back(new IR::StructField(IR::ID(residualFieldName), IR::Type::Bits::get(16)));
        residualChecksumNames.emplace(fl, residualFieldName);
    });

    auto annot = new IR::Annotations({new IR::Annotation(IR::ID("flexible"), {})});
    for (auto f : bridgedFields) {
        if (bridgedFieldInfo.count(f)) {
            auto info = bridgedFieldInfo.at(f);
            auto fn = convertLinearPathToStructFieldName(&info);
            auto field = info.components.back()->to<IR::Member>();
            fields->push_back(new IR::StructField(fn, annot, field->type));
        }
    }

    addBridgedFieldsFromChecksumUpdate(fields);

    auto type = new IR::Type_Header("bridged_header_t", *fields);
    auto inst = new IR::Header(IR::ID("bridged_header"), type);
    metadata.emplace(inst);
}

void TnaProgramStructure::getStructFieldsFromFieldList(IR::IndexedVector<IR::StructField> *fields,
                                                       const IR::ListExpression *expr) {
    for (auto f : expr->components) {
        if (auto list = f->to<IR::ListExpression>()) {
            getStructFieldsFromFieldList(fields, list);
        } else {
            auto linearizer = new BFN::PathLinearizer();
            f->apply(*linearizer);
            auto path = *linearizer->linearPath;
            auto fn = convertLinearPathToStructFieldName(&path);
            auto type = f->type;
            if (type->is<IR::Type_StructLike>())
                type = new IR::Type_Name(type->to<IR::Type_StructLike>()->name);
            auto annot = new IR::Annotations({new IR::Annotation(IR::ID("flexible"), {})});
            fields->push_back(new IR::StructField(fn, annot, type));
        }
    }
}

void TnaProgramStructure::createDigestHeaderTypeAndInstance(
    unsigned index, const IR::Expression *expr, cstring name,
    std::optional<IR::IndexedVector<IR::StructField>> tagFields) {
    auto fields = new IR::IndexedVector<IR::StructField>();
    if (tagFields != std::nullopt) fields->append(*tagFields);
    auto list = expr->to<IR::ListExpression>();
    BUG_CHECK(list != nullptr, "expected list for digest field list, not %1%", expr);
    getStructFieldsFromFieldList(fields, list);
    auto typeName = name + cstring::to_cstring(index) + cstring::to_cstring("_t");
    auto type = new IR::Type_Header(IR::ID(typeName), *fields);
    auto instName = name + cstring::to_cstring(index);
    auto inst = new IR::Header(IR::ID(instName), type);
    headers.emplace(inst);
}

// visit all control blocks, collect fields that are
// used in resubmit, clone primitives.
void TnaProgramStructure::collectDigestFields() {
    std::set<cstring> reachableFromIngress;
    calledControls.reachable("ingress"_cs, reachableFromIngress);
    for (auto s : reachableFromIngress) {
        if (auto ctrl = controls.get(s)) {
            // needed by CollectDigestFields to initalize conversionContext
            controlType(ctrl->name);
            auto actionsInTables = findActionInTables(ctrl);
            for (auto name : actionsInTables) {
                auto act = actions.get(name);
                act->apply(CollectDigestFields(this, INGRESS, digestFieldLists));
            }
        }
    }

    std::set<cstring> reachableFromEgress;
    calledControls.reachable("egress"_cs, reachableFromEgress);
    for (auto s : reachableFromEgress) {
        auto ctrl = controls.get(s);
        if (!ctrl) continue;
        // needed by CollectDigestFields to initalize conversionContext
        controlType(ctrl->name);
        auto actionsInTables = findActionInTables(ctrl);
        for (auto name : actionsInTables) {
            auto act = actions.get(name);
            act->apply(CollectDigestFields(this, EGRESS, digestFieldLists));
        }
    }

    for (auto d : digestFieldLists["resubmit"_cs]) {
        LOG3("generate resub " << d.first << " " << d.second.second);
        auto tagFields = new IR::IndexedVector<IR::StructField>();
        tagFields->push_back(
            new IR::StructField(IR::ID("resubmit_source"), IR::Type::Bits::get(8)));
        createDigestHeaderTypeAndInstance(d.first, d.second.second, "resubmit_header_"_cs,
                                          *tagFields);
    }

    for (auto d : digestFieldLists["clone_ingress_pkt_to_egress"_cs]) {
        LOG3("generate i2e " << d.first << " " << d.second.second);
        auto tagFields = new IR::IndexedVector<IR::StructField>();
        tagFields->push_back(new IR::StructField(IR::ID("mirror_source"), IR::Type::Bits::get(8)));
        createDigestHeaderTypeAndInstance(d.first, d.second.second, "ig_mirror_header_"_cs,
                                          *tagFields);
    }

    for (auto d : digestFieldLists["clone_egress_pkt_to_egress"_cs]) {
        LOG3("generate e2e " << d.first << " " << d.second.second);
        auto tagFields = new IR::IndexedVector<IR::StructField>();
        tagFields->push_back(new IR::StructField(IR::ID("mirror_source"), IR::Type::Bits::get(8)));
        createDigestHeaderTypeAndInstance(d.first, d.second.second, "eg_mirror_header_"_cs,
                                          *tagFields);
    }

    for (auto d : digestFieldLists["generate_digest"_cs]) {
        LOG3("generate digest " << d.first << " " << d.second.second);
        createDigestHeaderTypeAndInstance(d.first, d.second.second, "digest_header_"_cs,
                                          std::nullopt);
    }
}

void TnaProgramStructure::createPipeline() {
    auto name = IR::ID("pipe");
    auto typepath = new IR::Path("Pipeline");
    auto type = new IR::Type_Name(typepath);
    auto args = new IR::Vector<IR::Argument>();
    auto emptyArgs = new IR::Vector<IR::Argument>();

    auto ipp = new IR::Path("IngressParserImpl");
    auto ipt = new IR::Type_Name(ipp);
    auto ipc = new IR::ConstructorCallExpression(ipt, emptyArgs);
    args->push_back(new IR::Argument(ipc));

    auto igp = new IR::Path("ingress");
    auto igt = new IR::Type_Name(igp);
    auto igc = new IR::ConstructorCallExpression(igt, emptyArgs);
    args->push_back(new IR::Argument(igc));

    auto idp = new IR::Path("IngressDeparserImpl");
    auto idt = new IR::Type_Name(idp);
    auto idc = new IR::ConstructorCallExpression(idt, emptyArgs);
    args->push_back(new IR::Argument(idc));

    auto epp = new IR::Path("EgressParserImpl");
    auto ept = new IR::Type_Name(epp);
    auto epc = new IR::ConstructorCallExpression(ept, emptyArgs);
    args->push_back(new IR::Argument(epc));

    auto egp = new IR::Path("egress");
    auto egt = new IR::Type_Name(egp);
    auto egc = new IR::ConstructorCallExpression(egt, emptyArgs);
    args->push_back(new IR::Argument(egc));

    auto edp = new IR::Path("EgressDeparserImpl");
    auto edt = new IR::Type_Name(edp);
    auto edc = new IR::ConstructorCallExpression(edt, emptyArgs);
    args->push_back(new IR::Argument(edc));

    auto result = new IR::Declaration_Instance(name, type, args, nullptr);
    declarations->push_back(result);
}

void TnaProgramStructure::createMain() {
    createPipeline();

    auto name = IR::ID(IR::P4Program::main);
    auto typepath = new IR::Path("Switch");
    auto type = new IR::Type_Name(typepath);
    auto args = new IR::Vector<IR::Argument>();
    auto pipe0 = new IR::Path("pipe");
    auto expr = new IR::PathExpression(pipe0);
    args->push_back(new IR::Argument(expr));

    auto *annotations = new IR::Annotations();
    annotations->annotations.push_back(
        new IR::Annotation(IR::ID(PragmaAutoInitMetadata::name), {}));

    auto result = new IR::Declaration_Instance(name, annotations, type, args, nullptr);
    declarations->push_back(result);
}

void TnaProgramStructure::createCompilerGeneratedTypes() {
    // add 'compiler_generated_metadata_t'
    auto cgm = new IR::Type_Struct("compiler_generated_metadata_t");

    // Inject new fields for mirroring.
    cgm->fields.push_back(
        new IR::StructField("mirror_id"_cs, IR::Type::Bits::get(Device::cloneSessionIdWidth())));
    cgm->fields.push_back(new IR::StructField("mirror_source"_cs, IR::Type::Bits::get(8)));

    cgm->fields.push_back(new IR::StructField("resubmit_source"_cs, IR::Type::Bits::get(8)));

    // FIXME(hanw): we can probably remove these two fields.
    cgm->fields.push_back(new IR::StructField("clone_src"_cs, IR::Type::Bits::get(4)));
    cgm->fields.push_back(new IR::StructField("clone_digest_id"_cs, IR::Type::Bits::get(4)));
    // Used to support separate egress_start parser state.
    cgm->fields.push_back(new IR::StructField("instance_type"_cs, IR::Type::Bits::get(32)));

    auto meta = new IR::Metadata(COMPILER_META, cgm);
    metadata.emplace(meta);
}

/* If a P4-14 program uses ingress intrinsic metadata in egress, the intrinsic
 * metadata will be bridged.
 *
 * In P4-14, the ingress intrinsic metadata are defined as headers. In Tna,
 * intrinsic metadata that are not parsed are defined as structs. This function
 * fixes the staled information in programStructure which originates from
 * parsing the P4-14 source.
 *
 */

void TnaProgramStructure::createType(cstring typeName, cstring headerName) {
    LOG3("creating type " << typeName);
    if (tna_intr_md_types.count(typeName)) {
        if (types.get(typeName)) BUG("Cannot create type %1%, already exist!", typeName);
        auto type = tna_intr_md_types.at(typeName);
        if (type->is<IR::Type_Header>()) {
            auto hdr = new IR::Header(headerName, type->to<IR::Type_Header>());
            types.emplace(type);
            metadata.emplace(hdr);
            LOG3("Added header " << hdr);
        } else if (type->is<IR::Type_Struct>()) {
            auto md = new IR::Metadata(headerName, type->to<IR::Type_Struct>());
            types.emplace(type);
            metadata.emplace(md);
            LOG3("Added metadata " << md);
        }
    }
}

// add some P4-16 tna types into 'metadata' or 'headers' to help with
// bridging implementation
void TnaProgramStructure::createIntrinsicMetadataTypes() {
    createType("ingress_intrinsic_metadata_t"_cs, "ig_intr_md"_cs);
    createType("ingress_intrinsic_metadata_for_tm_t"_cs, "ig_intr_md_for_tm"_cs);
    createType("ingress_intrinsic_metadata_from_parser_t"_cs, "ig_intr_md_from_parser_aux"_cs);
    createType("ingress_intrinsic_metadata_for_mirror_buffer_t"_cs, "ig_intr_md_for_mb"_cs);

    createType("egress_intrinsic_metadata_t"_cs, "eg_intr_md"_cs);
    createType("egress_intrinsic_metadata_from_parser_t"_cs, "eg_intr_md_from_parser_aux"_cs);
    createType("egress_intrinsic_metadata_for_deparser_t"_cs, "eg_intr_md_for_dprsr"_cs);
    createType("egress_intrinsic_metadata_for_output_port_t"_cs, "eg_intr_md_for_oport"_cs);
    createType("egress_intrinsic_metadata_for_mirror_buffer_t"_cs, "eg_intr_md_for_mb"_cs);

    auto stdmeta = types.get("standard_metadata_t"_cs);
    if (stdmeta) {
        auto meta = new IR::Metadata("standard_metadata"_cs, stdmeta);
        metadata.emplace(meta);
    }
}

void TnaProgramStructure::removeType(cstring name, cstring headerName) {
    LOG3("removing type " << name);
    if (types.get(name)) {
        LOG3("Removed " << types.get(name));
        types.erase(name);
    }
    if (headers.get(headerName)) {
        LOG3("Removed " << headers.get(headerName));
        headers.erase(headerName);
    }
    if (metadata.get(headerName)) {
        LOG3("Removed " << metadata.get(headerName));
        metadata.erase(headerName);
    }
}

// remove types from parsing p4-14 intrinsic_metadata.p4
void TnaProgramStructure::removeP14IntrinsicMetadataTypes() {
    removeType("ingress_intrinsic_metadata_t"_cs, "ig_intr_md"_cs);
    removeType("ingress_intrinsic_metadata_for_tm_t"_cs, "ig_intr_md_for_tm"_cs);
    removeType("ingress_intrinsic_metadata_from_parser_aux_t"_cs, "ig_intr_md_from_parser_aux"_cs);
    removeType("ingress_intrinsic_metadata_for_mirror_buffer_t"_cs, "ig_intr_md_for_mb"_cs);
    removeType("egress_intrinsic_metadata_t"_cs, "eg_intr_md"_cs);
    removeType("egress_intrinsic_metadata_from_parser_aux_t"_cs, "eg_intr_md_from_parser_aux"_cs);
    removeType("egress_intrinsic_metadata_for_mirror_buffer_t"_cs, "eg_intr_md_for_mb"_cs);
    removeType("egress_intrinsic_metadata_for_output_port_t"_cs, "eg_intr_md_for_oport"_cs);
}

void TnaProgramStructure::loadModel() {
    if (BackendOptions().arch == "tna")
        include("tna.p4"_cs, "-D__TARGET_TOFINO__=1"_cs);
    else if (BackendOptions().arch == "t2na")
        include("t2na.p4"_cs, "-D__TARGET_TOFINO__=2"_cs);
    else
        error(
            "Must specify either --arch tna or --arch t2na"
            "");

    // iterate over tna declarations, find the type for intrinsic metadata
    for (auto decl : *declarations) {
        if (decl->getAnnotation("__intrinsic_metadata"_cs)) {
            auto st = decl->to<IR::Type_StructLike>();
            tna_intr_md_types.emplace(st->name.toString(), st);
        }
    }

    // iterate over tna declarations, insert header type declaration to
    // finalHeaderType
    for (auto decl : *declarations) {
        if (auto type = decl->to<IR::Type_Header>()) {
            finalHeaderType.emplace(type->externalName(), type);
        }
    }

    // make sure the P4-16 types are not added again as declarations.
    parameterTypes.insert("ingress_intrinsic_metadata_t"_cs);
    parameterTypes.insert("egress_intrinsic_metadata_t"_cs);
    parameterTypes.insert("ingress_intrinsic_metadata_for_tm_t"_cs);
    parameterTypes.insert("ingress_intrinsic_metadata_from_parser_t"_cs);
    parameterTypes.insert("ingress_intrinsic_metadata_for_mirror_buffer_t"_cs);
    parameterTypes.insert("egress_intrinsic_metadata_from_parser_t"_cs);
    parameterTypes.insert("egress_intrinsic_metadata_for_deparser_t"_cs);
    parameterTypes.insert("egress_intrinsic_metadata_for_output_port_t"_cs);
    parameterTypes.insert("egress_intrinsic_metadata_for_mirror_buffer_t"_cs);
    parameterTypes.insert("ingress_intrinsic_metadata_t"_cs);
    parameterTypes.insert("egress_intrinsic_metadata_t"_cs);
    parameterTypes.insert("ingress_intrinsic_metadata_for_tm_t"_cs);
    parameterTypes.insert("ingress_intrinsic_metadata_from_parser_t"_cs);
    parameterTypes.insert("ingress_intrinsic_metadata_for_mirror_buffer_t"_cs);
    parameterTypes.insert("egress_intrinsic_metadata_from_parser_t"_cs);
    parameterTypes.insert("egress_intrinsic_metadata_for_deparser_t"_cs);
    parameterTypes.insert("egress_intrinsic_metadata_for_output_port_t"_cs);
    parameterTypes.insert("egress_intrinsic_metadata_for_mirror_buffer_t"_cs);
    headerTypes.insert("pktgen_timer_header_t"_cs);
    headerTypes.insert("pktgen_port_down_header_t"_cs);
    headerTypes.insert("pktgen_recirc_header_t"_cs);
    headerTypes.insert("ptp_metadata_t"_cs);

    // following type names are from p4-14/tofino/intrinsic_metadata.p4
    // we would like to filter them out of generated toplevel header type.
    parameterTypes.insert("ingress_intrinsic_metadata_from_parser_aux_t"_cs);
    parameterTypes.insert("ingress_parser_control_signals"_cs);
    parameterTypes.insert("egress_intrinsic_metadata_from_parser_aux_t"_cs);
    headerInstances.insert("ingress_parser_control_signals"_cs);
    headerTypes.insert("generator_metadata_t_0"_cs);
    headerInstances.insert("generator_metadata_t_0"_cs);
}

bool CollectBridgedFields::preorder(const IR::MethodCallExpression *) {
    // skip
    return false;
}

bool CollectBridgedFields::preorder(const IR::Member *expr) {
    auto context = findContext<IR::Function>();
    if (context) return false;
    const bool exprIsRead = isRead();
    const bool exprIsWrite = isWrite();
    if (exprIsRead) LOG3("expr " << expr << " is read");
    if (exprIsWrite) LOG3("expr " << expr << " is written");

    auto linearizer = new BFN::PathLinearizer();
    expr->apply(*linearizer);
    if (!linearizer->linearPath) {
        LOG2("Won't bridge complex or invalid expression: " << expr);
        return false;
    }
    // we want to know about the metadata that is written in ingress and read
    // in egress.
    auto path = linearizer->linearPath->to_cstring();
    if (gress == INGRESS && exprIsWrite) {
        mayWrite.insert(path);
    } else if (gress == EGRESS && exprIsRead) {
        mayRead.insert(path);
        fieldInfo.emplace(path, *linearizer->linearPath);
    }
    return false;
}

unsigned long computeHashOverFieldList(const IR::Expression *expr,
                                       std::map<unsigned long, unsigned> &hashIndexMap,
                                       bool reserve_entry_zero) {
    std::string fieldsString;
    auto liste = expr->to<IR::ListExpression>();
    for (auto comp : liste->components) {
        std::ostringstream fieldListString;
        fieldListString << comp;
        fieldsString += fieldListString.str();
    }
    std::hash<std::string> hashFn;
    auto fieldsStringHash = hashFn(fieldsString);
    auto offset = reserve_entry_zero ? 1 : 0;
    if (hashIndexMap.count(fieldsStringHash) == 0)
        hashIndexMap.emplace(fieldsStringHash, hashIndexMap.size() + offset);
    return hashIndexMap[fieldsStringHash];
}

IR::Expression *CollectDigestFields::flatten(const IR::ListExpression *args) {
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

void CollectDigestFields::convertFieldList(const IR::Primitive *prim, size_t fieldListIndex,
                                           std::map<unsigned long, unsigned> &indexHashes,
                                           bool reserve_entry_zero = false) {
    const IR::Expression *list;
    std::optional<cstring> name = std::nullopt;
    BUG_CHECK(prim->operands.size() <= fieldListIndex, "Unexpected number of operands for %1%",
              prim);
    if (prim->operands.size() == fieldListIndex) {
        auto fl = prim->operands.at(fieldListIndex - 1);
        list = structure->convertFieldList(fl);
        if (auto liste = list->to<IR::ListExpression>()) list = flatten(liste);
        if (fl->is<IR::PathExpression>()) {
            auto nr = fl->to<IR::PathExpression>();
            name = nr->path->name;
        }
    } else {
        list = new IR::ListExpression({});
        name = std::nullopt;
    }

    LOG3("Assigned " << prim->name << " field list " << list);
    auto index = computeHashOverFieldList(list, indexHashes, reserve_entry_zero);
    digestFieldLists[prim->name][index] = std::make_pair(name, list);
}

bool CollectDigestFields::preorder(const IR::Primitive *prim) {
    if (prim->name == "resubmit" || prim->name == "resubmit_preserving_field_list") {
        convertFieldList(prim, 1, structure->resubmitIndexHashes);
    }
    if (prim->name == "clone_ingress_pkt_to_egress") {
        convertFieldList(prim, 2, structure->cloneIndexHashes[INGRESS], true);
    }
    if (prim->name == "clone_egress_pkt_to_egress") {
        convertFieldList(prim, 2, structure->cloneIndexHashes[EGRESS]);
    }
    if (prim->name == "generate_digest") {
        convertFieldList(prim, 2, structure->digestIndexHashes);
    }
    if (prim->name == "recirculate" || prim->name == "recirculate_preserving_field_list") {
        // exclude recirculate(68) and recirculate(local_port)
        auto operand = prim->operands.at(0);
        if (operand->is<IR::Constant>() || operand->is<IR::ActionArg>()) return true;
        convertFieldList(prim, 1, structure->recirculateIndexHashes);
    }
    return true;
}

IR::Node *FixParserPriority::preorder(IR::AssignmentStatement *assign) {
    auto left = assign->left;
    while (auto slice = left->to<IR::Slice>()) left = slice->e0;
    while (auto cast = left->to<IR::Cast>()) left = cast->expr;
    BFN::PathLinearizer linearizer;
    left->apply(linearizer);
    auto path = linearizer.linearPath;
    if (!path) return assign;
    if (path->to_cstring() == "ig_prsr_ctrl.priority") return nullptr;
    return assign;
}

const IR::P4Program *TnaProgramStructure::create(Util::SourceInfo info) {
    ////////////////////////////////////////////////////////////////////
    // the collect* passes has side-effect on 'declarations' due to the use of
    // convertControl functions. This is an undesirable behavior. We will make
    // a copy of the the declarations before it gets modified and restore the
    // declarations after these passes.
    auto snapshot = declarations->clone();
    collectControlGressMapping();
    collectBridgedFields();
    collectDigestFields();
    declarations->clear();
    declarations->append(*snapshot);
    globalInstances.clear();
    localInstances.clear();
    ////////////////////////////////////////////////////////////////////
    removeP14IntrinsicMetadataTypes();
    createIntrinsicMetadataTypes();
    createCompilerGeneratedTypes();
    createTypes();
    createStructures();
    createExterns();
    createParser();
    if (errorCount()) return nullptr;
    createControls();
    if (errorCount()) return nullptr;
    createDeparser();
    createMain();
    if (errorCount()) return nullptr;
    auto result = new IR::P4Program(info, *declarations);
    return result;
}

FixChecksum::FixChecksum(TnaProgramStructure *structure) {
    CHECK_NULL(structure);
    refMap.setIsV1(true);
    auto parserGraphs = new P4ParserGraphs(&refMap, cstring());
    addPasses({
        new P4::CreateBuiltins(),
        new P4::ResolveReferences(&refMap, true),  // check shadowing
        new P4::ConstantFolding(&typeMap),
        new P4::ResolveReferences(&refMap),
        new P4::TypeInference(&typeMap, false),
        parserGraphs,
        new ModifyParserForChecksum(&refMap, &typeMap, structure, parserGraphs),
        new InsertChecksumError(structure, parserGraphs),
        new InsertChecksumDeposit(structure),
        new P4::ClearTypeMap(&typeMap),
        new P4::TypeChecking(&refMap, &typeMap),  // useful to infer type on phase0 table
        new BFN::TranslatePhase0(&refMap, &typeMap, structure),
        new RemoveBuiltins(),
    });
}

TnaConverter::TnaConverter() {
    setStopOnError(true);
    setName("TnaConverter");

    structure = P4V1::TnaProgramStructure::create();
    structure->conversionContext = new ConversionContext();
    structure->conversionContext->clear();
    structure->populateOutputNames();

    // Discover types using P4-14 type-checker
    passes.emplace_back(new DetectDuplicates());
    passes.emplace_back(new P4::DoConstantFolding(nullptr, nullptr));
    passes.emplace_back(new CheckHeaderTypes());
    passes.emplace_back(new AdjustLengths());
    passes.emplace_back(new TypeCheck());
    // Convert
    passes.emplace_back(new DiscoverStructure(structure));
    passes.emplace_back(new ComputeCallGraph(structure));
    passes.emplace_back(new ComputeTableCallGraph(structure));
    passes.emplace_back(new Rewriter(structure));
    passes.emplace_back(new PostTranslationFix(structure));
}

Visitor::profile_t TnaConverter::init_apply(const IR::Node *node) {
    if (!node->is<IR::V1Program>()) BUG("Converter only accepts IR::Globals, not %1%", node);
    return PassManager::init_apply(node);
}

bool use_v1model() {
    return (BackendOptions().arch == "v1model" &&
            BackendOptions().langVersion == CompilerOptions::FrontendVersion::P4_14);
}

}  // namespace P4V1
}  // namespace P4
