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

#include "bf-p4c/midend/initialize_mirror_io_select.h"

namespace BFN {

// Add mirror_io_select initialization to egress parser
const IR::Node* DoInitializeMirrorIOSelect::preorder(IR::BFN::TnaParser* parser) {
    // Process egress parser only, skip others
    if (parser->thread != EGRESS) {
        prune();
        return parser;
    }

    // If available, use existing eg_intr_md_for_dprsr metadata parameter
    if (parser->tnaParams.find("eg_intr_md_for_dprsr"_cs) != parser->tnaParams.end()) {
        // Save existing eg_intr_md_for_dprsr parameter name for later use
        egIntrMdForDprsrName = parser->tnaParams.at("eg_intr_md_for_dprsr"_cs);
        return parser;
    }
    // Else, add missing eg_intr_md_for_dprsr (and other necessary optional metadata parameters)
    IR::IndexedVector<IR::Parameter> newParameters(parser->type->applyParams->parameters);
    ordered_map<cstring, cstring> newTnaParams(parser->tnaParams);
    if (parser->tnaParams.find("eg_intr_md_from_prsr"_cs) == parser->tnaParams.end()) {
        LOG3("InitializeMirrorIOSelect : Adding __eg_intr_md_from_prsr parameter to egress parser");
        const auto* newEgIntrMdFromPrsrParam =
            new IR::Parameter(
                "__eg_intr_md_from_prsr"_cs,
                IR::Direction::Out,
                new IR::Type_Name(new IR::Path("egress_intrinsic_metadata_from_parser_t")));
        newTnaParams.emplace("eg_intr_md_from_prsr"_cs, newEgIntrMdFromPrsrParam->name);
        newParameters.push_back(newEgIntrMdFromPrsrParam);
    }
    LOG3("InitializeMirrorIOSelect : Adding __eg_intr_md_for_dprsr parameter to egress parser");
    const auto* newEgIntrMdForDprsrParam =
        new IR::Parameter(
            "__eg_intr_md_for_dprsr"_cs,
            IR::Direction::Out,
            new IR::Type_Name(new IR::Path("egress_intrinsic_metadata_for_deparser_t"_cs)));
    newTnaParams.emplace("eg_intr_md_for_dprsr"_cs, newEgIntrMdForDprsrParam->name);
    newParameters.push_back(newEgIntrMdForDprsrParam);
    const auto* newTypeParams = new IR::ParameterList(parser->type->applyParams->srcInfo,
                                                      newParameters);
    const auto* newType = new IR::Type_Parser(parser->type->srcInfo,
                                              parser->type->name,
                                              parser->type->annotations,
                                              parser->type->typeParameters,
                                              newTypeParams);
    const auto* newParser = new IR::BFN::TnaParser(parser->srcInfo,
                                                   parser->name,
                                                   newType,
                                                   parser->constructorParams,
                                                   parser->parserLocals,
                                                   parser->states,
                                                   newTnaParams,
                                                   parser->thread,
                                                   parser->phase0,
                                                   parser->pipeName,
                                                   parser->portmap);
    // Save new eg_intr_md_for_dprsr parameter name for later use
    egIntrMdForDprsrName = newEgIntrMdForDprsrParam->name;
    return newParser;
}
const IR::Node* DoInitializeMirrorIOSelect::preorder(IR::ParserState* state) {
    prune();
    // Process start state only, skip others
    if (state->name.name != "start") {
        return state;
    }

    LOG3("InitializeMirrorIOSelect : Initializing " <<
         egIntrMdForDprsrName << ".mirror_io_select to 1");
    const auto* mirrorIOSelectInit =
        new IR::AssignmentStatement(
            new IR::Member(
                new IR::PathExpression(new IR::Path(egIntrMdForDprsrName)),
                "mirror_io_select"_cs),
            new IR::Constant(IR::Type_Bits::get(1), 1));
    IR::IndexedVector<IR::StatOrDecl> newStateComponents;
    newStateComponents.push_back(mirrorIOSelectInit);
    for (const auto* component : state->components)
        newStateComponents.push_back(component);
    const auto* newState = new IR::ParserState(state->srcInfo,
                                               state->name,
                                               state->annotations,
                                               newStateComponents,
                                               state->selectExpression);
    return newState;
}

}  // namespace BFN
