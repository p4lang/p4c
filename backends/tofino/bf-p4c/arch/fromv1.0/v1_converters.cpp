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

#include "v1_converters.h"

#include <boost/range/adaptor/reversed.hpp>

#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/common/utils.h"
#include "bf-p4c/device.h"
#include "lib/ordered_map.h"
#include "v1_program_structure.h"

namespace BFN {

namespace V1 {
//////////////////////////////////////////////////////////////////////////////////////////////

// Removes control local standard_metadata_t variable declarations and assignments that use them
const IR::Node *ControlConverter::postorder(IR::BFN::TnaControl *node) {
    std::unordered_set<cstring> varsToRemove;
    // Create new TnaControl controlLocals
    IR::IndexedVector<IR::Declaration> newTnaControlLocals;
    for (const auto *decl : node->controlLocals) {
        // Check all Declaration_Variable and P4Action nodes in TnaControl
        if (const auto *declVar = decl->to<IR::Declaration_Variable>()) {
            // Save names of standard_metadata_t variables and skip/remove their declarations
            const auto *declVarTypeName = declVar->type->to<IR::Type_Name>();
            if (declVarTypeName && declVarTypeName->path->name.name == "standard_metadata_t") {
                LOG3("Removing standard_metadata_t variable declaration: " << decl);
                varsToRemove.emplace(declVar->name.name);
                continue;
            }
        } else if (const auto *action = decl->to<IR::P4Action>()) {
            // Create new P4Action body components
            IR::IndexedVector<IR::StatOrDecl> newP4ActionBodyComponents;
            for (const auto *stmt : action->body->components) {
                // Check all AssignmentStatement nodes in P4Action
                if (const auto *asgnStmt = stmt->to<IR::AssignmentStatement>()) {
                    // Skip/remove all assignments using variables with saved names
                    const auto *asgnStmtLPathExpr = asgnStmt->left->to<IR::PathExpression>();
                    const auto *asgnStmtRPathExpr = asgnStmt->right->to<IR::PathExpression>();
                    if ((asgnStmtLPathExpr &&
                         varsToRemove.count(asgnStmtLPathExpr->path->name.name)) ||
                        (asgnStmtRPathExpr &&
                         varsToRemove.count(asgnStmtRPathExpr->path->name.name))) {
                        LOG3("Removing assignment statement using standard_metadata_t variable: "
                             << stmt);
                        continue;
                    }
                }
                // Else add original StatOrDecl to new P4Action body components
                newP4ActionBodyComponents.push_back(stmt);
            }
            // Create and add new P4Action (with new body) to new TnaControl controlLocals
            const auto *newP4ActionBody = new IR::BlockStatement(
                action->body->srcInfo, action->body->annotations, newP4ActionBodyComponents);
            const auto *newP4Action =
                new IR::P4Action(action->srcInfo, action->name, action->annotations,
                                 action->parameters, newP4ActionBody);
            newTnaControlLocals.push_back(newP4Action);
            continue;
        }
        // Else add original Declaration to new TnaControl controlLocals
        newTnaControlLocals.push_back(decl);
    }
    // Create and return new TnaControl (with new controlLocals)
    return new IR::BFN::TnaControl(node->srcInfo, node->name, node->type, node->constructorParams,
                                   newTnaControlLocals, node->body, node->tnaParams, node->thread,
                                   node->pipeName);
}

const IR::Node *IngressControlConverter::preorder(IR::P4Control *node) {
    auto params = node->type->getApplyParameters();
    BUG_CHECK(params->size() == 3, "%1% Expected 3 parameters for ingress", node);

    auto *paramList = new IR::ParameterList;
    ordered_map<cstring, cstring> tnaParams;

    auto *headers = params->parameters.at(0);
    tnaParams.emplace("hdr"_cs, headers->name);
    paramList->push_back(headers);

    auto *meta = params->parameters.at(1);
    tnaParams.emplace("md"_cs, meta->name);
    paramList->push_back(meta);

    // add ig_intr_md
    auto path = new IR::Path("ingress_intrinsic_metadata_t");
    auto type = new IR::Type_Name(path);
    auto param = new IR::Parameter("ig_intr_md", IR::Direction::In, type);
    tnaParams.emplace("ig_intr_md"_cs, param->name);
    paramList->push_back(param);

    // add ig_intr_md_from_prsr
    path = new IR::Path("ingress_intrinsic_metadata_from_parser_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md_from_prsr", IR::Direction::In, type);
    tnaParams.emplace("ig_intr_md_from_prsr"_cs, param->name);
    paramList->push_back(param);

    // add ig_intr_md_for_dprsr
    path = new IR::Path("ingress_intrinsic_metadata_for_deparser_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md_for_dprsr", IR::Direction::InOut, type);
    tnaParams.emplace("ig_intr_md_for_dprsr"_cs, param->name);
    paramList->push_back(param);

    // add ig_intr_md_for_tm
    path = new IR::Path("ingress_intrinsic_metadata_for_tm_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md_for_tm", IR::Direction::InOut, type);
    tnaParams.emplace("ig_intr_md_for_tm"_cs, param->name);
    paramList->push_back(param);

    // add compiler generated struct
    path = new IR::Path("compiler_generated_metadata_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter(BFN::COMPILER_META, IR::Direction::InOut, type);
    tnaParams.emplace(BFN::COMPILER_META, param->name);
    paramList->push_back(param);

    auto controlType = new IR::Type_Control("ingress", paramList);

    auto controlLocals = new IR::IndexedVector<IR::Declaration>();
    controlLocals->append(structure->ingressDeclarations);
    controlLocals->append(node->controlLocals);

    auto result =
        new IR::BFN::TnaControl(node->srcInfo, "ingress"_cs, controlType, node->constructorParams,
                                *controlLocals, node->body, tnaParams, INGRESS);
    return result;
}

const IR::Node *EgressControlConverter::preorder(IR::P4Control *node) {
    auto params = node->type->getApplyParameters();
    BUG_CHECK(params->size() == 3, "%1% Expected 3 parameters for egress", node);

    auto *paramList = new IR::ParameterList;
    ordered_map<cstring, cstring> tnaParams;

    auto *headers = params->parameters.at(0);
    tnaParams.emplace("hdr"_cs, headers->name);
    paramList->push_back(headers);

    auto *meta = params->parameters.at(1);
    tnaParams.emplace("md"_cs, meta->name);
    paramList->push_back(meta);

    // add eg_intr_md
    auto path = new IR::Path("egress_intrinsic_metadata_t");
    auto type = new IR::Type_Name(path);
    IR::Parameter *param = nullptr;
    if (structure->backward_compatible)
        param = new IR::Parameter("eg_intr_md", IR::Direction::InOut, type);
    else
        param = new IR::Parameter("eg_intr_md", IR::Direction::In, type);
    tnaParams.emplace("eg_intr_md"_cs, param->name);
    paramList->push_back(param);

    // add eg_intr_md_from_prsr
    path = new IR::Path("egress_intrinsic_metadata_from_parser_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("eg_intr_md_from_prsr", IR::Direction::In, type);
    tnaParams.emplace("eg_intr_md_from_prsr"_cs, param->name);
    paramList->push_back(param);

    // add eg_intr_md_for_dprsr
    path = new IR::Path("egress_intrinsic_metadata_for_deparser_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("eg_intr_md_for_dprsr", IR::Direction::InOut, type);
    tnaParams.emplace("eg_intr_md_for_dprsr"_cs, param->name);
    paramList->push_back(param);

    // add eg_intr_md_for_oport
    path = new IR::Path("egress_intrinsic_metadata_for_output_port_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("eg_intr_md_for_oport", IR::Direction::InOut, type);
    tnaParams.emplace("eg_intr_md_for_oport"_cs, param->name);
    paramList->push_back(param);

    /// TODO following two parameters are added and should be removed after
    /// the defuse analysis is moved to the midend and bridge metadata is implemented.

    // add ig_intr_md_from_prsr
    path = new IR::Path("ingress_intrinsic_metadata_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md", IR::Direction::InOut, type);
    tnaParams.emplace("ig_intr_md"_cs, param->name);
    paramList->push_back(param);

    // add ig_intr_md_for_tm
    path = new IR::Path("ingress_intrinsic_metadata_for_tm_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md_for_tm", IR::Direction::InOut, type);
    tnaParams.emplace("ig_intr_md_for_tm"_cs, param->name);
    paramList->push_back(param);

    // add ig_intr_md_from_prsr
    path = new IR::Path("ingress_intrinsic_metadata_from_parser_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md_from_prsr", IR::Direction::InOut, type);
    tnaParams.emplace("ig_intr_md_from_prsr"_cs, param->name);
    paramList->push_back(param);

    // add compiler generated struct
    path = new IR::Path("compiler_generated_metadata_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter(BFN::COMPILER_META, IR::Direction::InOut, type);
    tnaParams.emplace(BFN::COMPILER_META, param->name);
    paramList->push_back(param);

    auto controlType = new IR::Type_Control("egress", paramList);

    auto controlLocals = new IR::IndexedVector<IR::Declaration>();
    controlLocals->append(structure->egressDeclarations);
    controlLocals->append(node->controlLocals);

    auto result =
        new IR::BFN::TnaControl(node->srcInfo, "egress"_cs, controlType, node->constructorParams,
                                *controlLocals, node->body, tnaParams, EGRESS);
    return result;
}

const IR::Node *IngressDeparserConverter::preorder(IR::P4Control *node) {
    auto deparser = node->apply(cloner);
    auto params = deparser->type->getApplyParameters();
    BUG_CHECK(params->size() == 2, "%1% Expected 2 parameters for deparser", deparser);

    auto *paramList = new IR::ParameterList;
    ordered_map<cstring, cstring> tnaParams;

    auto *packetOut = params->parameters.at(0);
    tnaParams.emplace("pkt"_cs, packetOut->name);
    paramList->push_back(packetOut);

    // add header
    auto hdr = deparser->getApplyParameters()->parameters.at(1);
    auto param = new IR::Parameter(hdr->name, hdr->annotations, IR::Direction::InOut, hdr->type);
    tnaParams.emplace("hdr"_cs, param->name);
    paramList->push_back(param);

    // add metadata
    CHECK_NULL(structure->user_metadata);
    auto meta = structure->user_metadata;
    param = new IR::Parameter(meta->name, meta->annotations, IR::Direction::In, meta->type);
    tnaParams.emplace("md"_cs, param->name);
    paramList->push_back(param);

    // add deparser intrinsic metadata
    auto path = new IR::Path("ingress_intrinsic_metadata_for_deparser_t");
    auto type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md_for_dprsr", IR::Direction::In, type);
    tnaParams.emplace("ig_intr_md_for_dprsr"_cs, param->name);
    paramList->push_back(param);

    // add intrinsic metadata
    path = new IR::Path("ingress_intrinsic_metadata_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md", IR::Direction::In, type);
    tnaParams.emplace("ig_intr_md"_cs, param->name);
    paramList->push_back(param);

    // add compiler generated struct
    path = new IR::Path("compiler_generated_metadata_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter(BFN::COMPILER_META, IR::Direction::InOut, type);
    tnaParams.emplace(BFN::COMPILER_META, param->name);
    paramList->push_back(param);

    auto controlType = new IR::Type_Control("ingressDeparserImpl", paramList);

    auto controlLocals = new IR::IndexedVector<IR::Declaration>();
    controlLocals->append(structure->ingressDeparserDeclarations);
    controlLocals->append(node->controlLocals);

    auto statements = new IR::IndexedVector<IR::StatOrDecl>();
    statements->append(structure->ingressDeparserStatements);
    statements->append(deparser->body->components);
    auto body = new IR::BlockStatement(deparser->body->srcInfo, *statements);

    auto result = new IR::BFN::TnaDeparser(deparser->srcInfo, "ingressDeparserImpl"_cs, controlType,
                                           deparser->constructorParams, *controlLocals, body,
                                           tnaParams, INGRESS);
    return result;
}

const IR::Node *EgressDeparserConverter::preorder(IR::P4Control *node) {
    /**
     * create clone for all path expressions in the egress deparser.
     * Otherwise, resolveReference will fail because the path expression may reference
     * to a type in another control block.
     */
    auto deparser = node->apply(cloner);

    auto params = deparser->type->getApplyParameters();
    BUG_CHECK(params->size() == 2, "%1% Expected 2 parameters for deparser", deparser);

    auto *paramList = new IR::ParameterList;
    ordered_map<cstring, cstring> tnaParams;

    auto *packetOut = params->parameters.at(0);
    tnaParams.emplace("pkt"_cs, packetOut->name);
    paramList->push_back(packetOut);

    // add header
    auto hdr = deparser->getApplyParameters()->parameters.at(1);
    auto param = new IR::Parameter(hdr->name, hdr->annotations, IR::Direction::InOut, hdr->type);
    tnaParams.emplace("hdr"_cs, param->name);
    paramList->push_back(param);

    // add metadata
    CHECK_NULL(structure->user_metadata);
    auto meta = structure->user_metadata;
    param = new IR::Parameter(meta->name, meta->annotations, IR::Direction::In, meta->type);
    tnaParams.emplace("md"_cs, param->name);
    paramList->push_back(param);

    // add eg_intr_md_for_dprsr
    auto path = new IR::Path("egress_intrinsic_metadata_for_deparser_t");
    auto type = new IR::Type_Name(path);
    param = new IR::Parameter("eg_intr_md_for_dprsr", IR::Direction::In, type);
    tnaParams.emplace("eg_intr_md_for_dprsr"_cs, param->name);
    paramList->push_back(param);

    // add eg_intr_md_from_prsr
    path = new IR::Path("egress_intrinsic_metadata_from_parser_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("eg_intr_md_from_prsr", IR::Direction::In, type);
    tnaParams.emplace("eg_intr_md_from_prsr"_cs, param->name);
    paramList->push_back(param);

    // add eg_intr_md
    path = new IR::Path("egress_intrinsic_metadata_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("eg_intr_md", IR::Direction::In, type);
    tnaParams.emplace("eg_intr_md"_cs, param->name);
    paramList->push_back(param);

    // add ig_intr_md
    path = new IR::Path("ingress_intrinsic_metadata_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md", IR::Direction::In, type);
    tnaParams.emplace("ig_intr_md"_cs, param->name);
    paramList->push_back(param);

    // add ig_intr_md_for_tm
    path = new IR::Path("ingress_intrinsic_metadata_for_tm_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md_for_tm", IR::Direction::In, type);
    tnaParams.emplace("ig_intr_md_for_tm"_cs, param->name);
    paramList->push_back(param);

    // add compiler generated struct
    path = new IR::Path("compiler_generated_metadata_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter(BFN::COMPILER_META, IR::Direction::InOut, type);
    tnaParams.emplace(BFN::COMPILER_META, param->name);
    paramList->push_back(param);

    auto controlType = new IR::Type_Control("egressDeparserImpl", paramList);

    auto controlLocals = new IR::IndexedVector<IR::Declaration>();
    controlLocals->append(structure->egressDeparserDeclarations);
    controlLocals->append(node->controlLocals);

    auto statements = new IR::IndexedVector<IR::StatOrDecl>();
    statements->append(structure->egressDeparserStatements);
    statements->append(deparser->body->components);
    auto body = new IR::BlockStatement(deparser->body->srcInfo, *statements);

    auto result = new IR::BFN::TnaDeparser(deparser->srcInfo, "egressDeparserImpl"_cs, controlType,
                                           deparser->constructorParams, *controlLocals, body,
                                           tnaParams, EGRESS);
    return result;
}

const IR::Node *IngressParserConverter::postorder(IR::P4Parser *node) {
    auto parser = node->apply(cloner);
    auto params = parser->type->getApplyParameters();
    BUG_CHECK(params->size() == 4, "%1%: Expected 4 parameters for parser", parser);

    auto *paramList = new IR::ParameterList;
    ordered_map<cstring, cstring> tnaParams;

    auto *packetIn = params->parameters.at(0);
    tnaParams.emplace("pkt"_cs, packetIn->name);
    paramList->push_back(packetIn);

    auto *headers = params->parameters.at(1);
    tnaParams.emplace("hdr"_cs, headers->name);
    paramList->push_back(headers);

    auto *meta = parser->getApplyParameters()->parameters.at(2);
    auto *param = new IR::Parameter(meta->name, meta->annotations, IR::Direction::Out, meta->type);
    tnaParams.emplace("md"_cs, meta->name);
    paramList->push_back(param);

    // add ig_intr_md
    auto path = new IR::Path("ingress_intrinsic_metadata_t");
    auto type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md", IR::Direction::Out, type);
    tnaParams.emplace("ig_intr_md"_cs, param->name);
    paramList->push_back(param);

    // add ig_intr_md_from_prsr
    path = new IR::Path("ingress_intrinsic_metadata_from_parser_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md_from_prsr", IR::Direction::Out, type);
    tnaParams.emplace("ig_intr_md_from_prsr"_cs, param->name);
    paramList->push_back(param);

    // add ig_intr_md_for_tm
    path = new IR::Path("ingress_intrinsic_metadata_for_tm_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md_for_tm", IR::Direction::Out, type);
    tnaParams.emplace("ig_intr_md_for_tm"_cs, param->name);
    paramList->push_back(param);

    // add compiler generated struct
    path = new IR::Path("compiler_generated_metadata_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter(BFN::COMPILER_META, IR::Direction::InOut, type);
    tnaParams.emplace(BFN::COMPILER_META, param->name);
    paramList->push_back(param);

    auto parser_type = new IR::Type_Parser("ingressParserImpl", paramList);

    auto parserLocals = new IR::IndexedVector<IR::Declaration>();
    parserLocals->append(structure->ingressParserDeclarations);
    parserLocals->append(node->parserLocals);

    for (auto *state : parser->states) {
        auto it = structure->ingressParserStatements.find(state->name);
        if (it != structure->ingressParserStatements.end()) {
            auto *s = const_cast<IR::ParserState *>(state);
            for (auto *stmt : it->second) s->components.push_back(stmt);
        }
    }

    auto result = new IR::BFN::TnaParser(parser->srcInfo, "ingressParserImpl"_cs, parser_type,
                                         parser->constructorParams, *parserLocals, parser->states,
                                         tnaParams, INGRESS);
    return result;
}

const IR::Node *EgressParserConverter::postorder(IR::Declaration_Variable *node) {
    return new IR::Declaration_Variable(node->srcInfo, node->name, node->type,
                                        node->initializer->apply(cloner));
}

const IR::Node *EgressParserConverter::postorder(IR::P4Parser *node) {
    auto parser = node->apply(cloner);
    auto params = parser->type->getApplyParameters();
    /// assume we are dealing with egress parser
    BUG_CHECK(params->size() == 4, "%1%: Expected 4 parameters for parser", parser);

    auto *paramList = new IR::ParameterList;
    ordered_map<cstring, cstring> tnaParams;

    auto *packetIn = params->parameters.at(0);
    tnaParams.emplace("pkt"_cs, packetIn->name);
    paramList->push_back(packetIn);

    auto *headers = params->parameters.at(1);
    tnaParams.emplace("hdr"_cs, headers->name);
    paramList->push_back(headers);

    auto *meta = params->parameters.at(2);
    auto *param = new IR::Parameter(meta->name, meta->annotations, IR::Direction::Out, meta->type);
    tnaParams.emplace("md"_cs, meta->name);
    paramList->push_back(param);

    // add eg_intr_md
    auto path = new IR::Path("egress_intrinsic_metadata_t");
    auto type = new IR::Type_Name(path);
    param = new IR::Parameter("eg_intr_md", IR::Direction::Out, type);
    tnaParams.emplace("eg_intr_md"_cs, param->name);
    paramList->push_back(param);

    // add eg_intr_md_from_prsr
    path = new IR::Path("egress_intrinsic_metadata_from_parser_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("eg_intr_md_from_prsr", IR::Direction::Out, type);
    tnaParams.emplace("eg_intr_md_from_prsr"_cs, param->name);
    paramList->push_back(param);

    // add ig_intr_md
    path = new IR::Path("ingress_intrinsic_metadata_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md", IR::Direction::InOut, type);
    tnaParams.emplace("ig_intr_md"_cs, param->name);
    paramList->push_back(param);

    // add ig_intr_md_for_tm
    path = new IR::Path("ingress_intrinsic_metadata_for_tm_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter("ig_intr_md_for_tm", IR::Direction::InOut, type);
    tnaParams.emplace("ig_intr_md_for_tm"_cs, param->name);
    paramList->push_back(param);

    // add compiler generated struct
    path = new IR::Path("compiler_generated_metadata_t");
    type = new IR::Type_Name(path);
    param = new IR::Parameter(BFN::COMPILER_META, IR::Direction::InOut, type);
    tnaParams.emplace(BFN::COMPILER_META, param->name);
    paramList->push_back(param);

    IR::IndexedVector<IR::Declaration> parserLocals;
    parserLocals.append(structure->egressParserDeclarations);
    parserLocals.append(node->parserLocals);

    for (auto *state : parser->states) {
        auto it = structure->egressParserStatements.find(state->name);
        if (it != structure->egressParserStatements.end()) {
            auto *s = const_cast<IR::ParserState *>(state);
            for (auto *stmt : it->second) s->components.push_back(stmt);
        }
    }

    auto parser_type = new IR::Type_Parser("egressParserImpl", paramList);
    auto result = new IR::BFN::TnaParser(parser->srcInfo, "egressParserImpl"_cs, parser_type,
                                         parser->constructorParams, parserLocals, parser->states,
                                         tnaParams, EGRESS);
    return result;
}

//////////////////////////////////////////////////////////////////////////////////////////////

const IR::Node *TypeNameExpressionConverter::postorder(IR::Type_Name *node) {
    auto path = node->path->to<IR::Path>();
    if (auto newName = ::get(enumsToTranslate, path->name)) {
        if (!structure->enums.count(newName)) {
            BUG("Cannot translation for type ", node);
            return node;
        }
        return new IR::Type_Name(IR::ID(node->srcInfo, newName));
    }
    return node;
}

const IR::Node *TypeNameExpressionConverter::postorder(IR::TypeNameExpression *node) {
    auto typeName = node->typeName->to<IR::Type_Name>();
    auto path = typeName->path->to<IR::Path>();
    if (auto newType = ::get(structure->enums, path->name)) {
        node->type = newType;
    }
    return node;
}

const IR::Node *TypeNameExpressionConverter::postorder(IR::Member *node) {
    if (!node->expr->is<IR::TypeNameExpression>()) return node;
    auto name = node->member;
    if (auto newName = ::get(fieldsToTranslate, name))
        return new IR::Member(node->srcInfo, node->expr, newName);
    return node;
}

/// map path expression
const IR::Node *PathExpressionConverter::postorder(IR::Member *node) {
    auto membername = node->member.name;
    auto expr = node->expr->to<IR::PathExpression>();
    if (!expr) return node;
    auto pathname = expr->path->name;
    using PathAndMember = std::pair<cstring, cstring>;
    static std::set<PathAndMember> reportedErrors;

    gress_t thread;
    if (auto *parser = findContext<IR::BFN::TnaParser>()) {
        thread = parser->thread;
    } else if (auto *control = findContext<IR::BFN::TnaControl>()) {
        thread = control->thread;
    } else if (auto *control = findContext<IR::BFN::TnaDeparser>()) {
        thread = control->thread;
    } else {
        LOG3("Member expression " << node
                                  << " is not inside a translated control; "
                                     "won't translate it");
        return node;
    }

    auto &nameMap =
        thread == INGRESS ? structure->ingressMetadataNameMap : structure->egressMetadataNameMap;
    auto &otherMap =
        thread == INGRESS ? structure->egressMetadataNameMap : structure->ingressMetadataNameMap;
    BUG_CHECK(!nameMap.empty(), "metadata translation map cannot be empty");

    if (auto type = node->type->to<IR::Type_Bits>()) {
        auto it = nameMap.find(MetadataField{pathname, membername, type->size});
        if (it != nameMap.end()) {
            auto expr = new IR::PathExpression(it->second.structName);
            auto member = new IR::Member(node->srcInfo, expr, it->second.fieldName);
            member->type = IR::Type::Bits::get(it->second.width);

            IR::Expression *result = member;

            if (it->second.offset != 0) {
                result = new IR::Slice(result, it->second.offset + it->second.width - 1,
                                       it->second.offset);
            }
            if (it->first.width != it->second.width && findOrigCtxt<IR::Operation>()) {
                result = new IR::Cast(IR::Type::Bits::get(it->first.width), result);
            }

            LOG3("Translating " << node << " to " << result);
            return result;
        } else if (otherMap.count(MetadataField{pathname, membername, type->size})) {
            auto wasReported = reportedErrors.insert(PathAndMember(pathname, membername));
            if (wasReported.second)
                error("%1% is not accessible in the %2% pipe", node, toString(thread));
        } else if (pathname == "standard_metadata") {
            error(
                "standard_metadata field %1% cannot be translated, you "
                "cannot use it in your program",
                node);
        }
    }
    LOG4("No translation found for " << node);
    return node;
}

/// The translation pass only renames intrinsic metadata. If the width of the
/// metadata is also changed after the translation, then this pass will insert
/// appropriate cast to the RHS of the assignment.
const IR::Node *PathExpressionConverter::postorder(IR::AssignmentStatement *node) {
    auto left = node->left;
    auto right = node->right;

    if (auto mem = left->to<IR::Member>()) {
        auto type = mem->type->to<IR::Type_Bits>();
        if (!type) return node;
        if (auto path = mem->expr->to<IR::PathExpression>()) {
            MetadataField field{path->path->name, mem->member.name, type->size};
            auto it = structure->targetMetadataSet.find(field);
            if (it != structure->targetMetadataSet.end()) {
                auto type = IR::Type::Bits::get(it->width);
                if (type != right->type) {
                    if (right->type->is<IR::Type_Boolean>()) {
                        if (right->to<IR::BoolLiteral>()->value == true) {
                            right = new IR::Constant(type, 1);
                        } else {
                            right = new IR::Constant(type, 0);
                        }
                    } else {
                        right = new IR::Cast(type, right);
                    }
                    return new IR::AssignmentStatement(node->srcInfo, left, right);
                }
            }
        }
    }
    return node;
}

//////////////////////////////////////////////////////////////////////////////////////////////
const IR::Node *ExternConverter::postorder(IR::Member *node) {
    auto orig = getOriginal<IR::Member>();
    auto it = structure->typeNamesToDo.find(orig);
    if (it != structure->typeNamesToDo.end()) return it->second;
    return node;
}

const IR::Expression *MeterConverter::cast_if_needed(const IR::Expression *expr, int srcWidth,
                                                     int dstWidth) {
    if (srcWidth == dstWidth) return expr;
    if (srcWidth < dstWidth) return new IR::Cast(IR::Type::Bits::get(dstWidth), expr);
    if (srcWidth > dstWidth) return new IR::Slice(expr, dstWidth - 1, 0);
    return expr;
}

const IR::Node *MeterConverter::postorder(IR::MethodCallStatement *node) {
    auto orig = getOriginal<IR::MethodCallStatement>();
    auto mce = orig->methodCall->to<IR::MethodCallExpression>();

    auto member = mce->method->to<IR::Member>();
    auto method = new IR::Member(node->srcInfo, member->expr, "execute");

    auto meter_pe = member->expr->to<IR::PathExpression>();
    BUG_CHECK(meter_pe != nullptr, "Cannot find meter %1%", member->expr);

    auto args = new IR::Vector<IR::Argument>();
    auto inst = refMap->getDeclaration(meter_pe->path);
    auto annot = inst->getAnnotation("pre_color"_cs);
    if (annot != nullptr) {
        auto size = annot->expr.at(0)->type->width_bits();
        auto expr = annot->expr.at(0);
        auto castedExpr = cast_if_needed(expr, size, 8);
        args->push_back(
            new IR::Argument("color", new IR::Cast(new IR::Type_Name("MeterColor_t"), castedExpr)));
    }

    int meter_color_index;
    if (direct)
        meter_color_index = 0;
    else
        meter_color_index = 1;

    auto meterColor = mce->arguments->at(meter_color_index)->expression;

    auto size = meterColor->type->width_bits();
    BUG_CHECK(size != 0, "meter color cannot be bit<0>");
    if (!direct) {
        auto indexArg = mce->arguments->at(0);
        args->push_back(new IR::Argument(indexArg->srcInfo, "index"_cs, indexArg->expression));
    }
    auto methodcall = new IR::MethodCallExpression(node->srcInfo, method, args);
    auto castedMethodCall = cast_if_needed(methodcall, 8, size);
    return new IR::AssignmentStatement(meterColor, castedMethodCall);
}

const IR::Node *RegisterConverter::postorder(IR::MethodCallStatement *node) {
    auto orig = getOriginal<IR::MethodCallStatement>();
    auto mce = orig->methodCall->to<IR::MethodCallExpression>();
    auto member = mce->method->to<IR::Member>();

    if (member->member != "read") return node;

    auto method = new IR::Member(node->srcInfo, member->expr, "read");
    auto args = new IR::Vector<IR::Argument>({mce->arguments->at(1)});
    auto methodcall = new IR::MethodCallExpression(node->srcInfo, method, args);
    return new IR::AssignmentStatement(mce->arguments->at(0)->expression, methodcall);
}
//////////////////////////////////////////////////////////////////////////////////////////////

const IR::Node *ParserPriorityConverter::postorder(IR::AssignmentStatement *node) {
    auto parserPriority = new IR::PathExpression("ig_prsr_ctrl_priority");
    auto member = new IR::Member(parserPriority, "set");
    auto result =
        new IR::MethodCallStatement(node->srcInfo, member, {new IR::Argument(node->right)});
    return result;
}

}  // namespace V1

}  // namespace BFN
