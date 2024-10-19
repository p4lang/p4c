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

#ifndef BF_P4C_ARCH_INTRINSIC_METADATA_H_
#define BF_P4C_ARCH_INTRINSIC_METADATA_H_

#include "bf-p4c/arch/bridge_metadata.h"
#include "bf-p4c/device.h"
#include "bf-p4c/midend/type_checker.h"
#include "frontends/p4/cloner.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

namespace BFN {

const IR::ParserState *convertStartStateToNormalState(const IR::ParserState *state,
                                                      cstring newName);

const IR::ParserState *convertStartStateToNormalState(IR::P4Parser *parser, cstring newName);

const IR::ParserState *addNewStartState(cstring name, cstring nextState);
void addNewStartState(IR::P4Parser *parser, cstring name, cstring nextState);

const IR::ParserState *createGeneratedParserState(cstring name,
                                                  IR::IndexedVector<IR::StatOrDecl> &&statements,
                                                  const IR::Expression *selectExpression);
const IR::ParserState *createGeneratedParserState(cstring name,
                                                  IR::IndexedVector<IR::StatOrDecl> &&statements,
                                                  cstring nextState);
const IR::Statement *createAdvanceCall(cstring pkt, int bits);
const IR::SelectCase *createSelectCase(unsigned bitWidth, unsigned value, unsigned mask,
                                       const IR::ParserState *nextState);
const IR::Statement *createSetMetadata(cstring header, cstring field, int bitWidth, int constant);
const IR::Statement *createSetMetadata(cstring param, cstring header, cstring field, int bitWidth,
                                       int constant);
const IR::Statement *createSetMetadata(const IR::Expression *dest, cstring header, cstring field);
const IR::Statement *createSetValid(const Util::SourceInfo &, cstring header, cstring field);
const IR::Statement *createExtractCall(cstring pkt, cstring type, cstring hdr);
const IR::Statement *createExtractCall(cstring pkt, IR::Expression *member);
const IR::Statement *createExtractCall(cstring pkt, cstring typeName, IR::Expression *member);
const IR::Expression *createLookaheadExpr(cstring pkt, int bits);

/// Add the standard TNA ingress metadata to the given parser. The original
/// start state will remain in the program, but with a new name.
static void addIngressMetadata(IR::BFN::TnaParser *parser) {
    auto *p4EntryPointState = convertStartStateToNormalState(parser, "ingress_p4_entry_point"_cs);

    // Add a state that skips over any padding between the phase 0 data and the
    // beginning of the packet.
    const auto bitSkip = Device::pardeSpec().bitIngressPrePacketPaddingSize();
    auto packetInParam = parser->tnaParams.at("pkt"_cs);
    auto *skipToPacketState = createGeneratedParserState(
        "skip_to_packet"_cs, {createAdvanceCall(packetInParam, bitSkip)}, p4EntryPointState->name);
    parser->states.push_back(skipToPacketState);

    // Add a state that parses the phase 0 data. This is a placeholder that
    // just skips it; if we find a phase 0 table, it'll be replaced later.
    const auto bitPhase0Size = Device::pardeSpec().bitPhase0Size();
    auto *phase0State = createGeneratedParserState(
        "phase0"_cs, {createAdvanceCall(packetInParam, bitPhase0Size)}, skipToPacketState->name);
    parser->states.push_back(phase0State);

    // This state parses resubmit data. Just like phase 0, the version we're
    // generating here is a placeholder that just skips the data; we'll replace
    // it later with an actual implementation.
    const auto bitResubmitSize = Device::pardeSpec().bitResubmitSize();
    auto *resubmitState = createGeneratedParserState(
        "resubmit"_cs, {createAdvanceCall(packetInParam, bitResubmitSize)},
        skipToPacketState->name);
    parser->states.push_back(resubmitState);

    // If this is a resubmitted packet, the initial intrinsic metadata will be
    // followed by the resubmit data; otherwise, it's followed by the phase 0
    // data. This state checks the resubmit flag and branches accordingly.
    auto igIntrMd = parser->tnaParams.at("ig_intr_md"_cs);
    IR::Vector<IR::Expression> selectOn = {
        new IR::Member(new IR::PathExpression(igIntrMd), "resubmit_flag"_cs)};
    auto *checkResubmitState = createGeneratedParserState(
        "check_resubmit"_cs, {},
        new IR::SelectExpression(new IR::ListExpression(selectOn),
                                 {createSelectCase(1, 0x0, 0x1, phase0State),
                                  createSelectCase(1, 0x1, 0x1, resubmitState)}));
    parser->states.push_back(checkResubmitState);

    // This state handles the extraction of ingress intrinsic metadata.
    auto *igMetadataState = createGeneratedParserState(
        "ingress_metadata"_cs,
        {createSetMetadata("ig_intr_md_from_prsr"_cs, "parser_err"_cs, 16, 0),
         createExtractCall(packetInParam, "ingress_intrinsic_metadata_t"_cs,
                           parser->tnaParams.at("ig_intr_md"_cs))},
        checkResubmitState->name);
    parser->states.push_back(igMetadataState);

    addNewStartState(parser, "ingress_tna_entry_point"_cs, igMetadataState->name);
}

/// Add the standard TNA egress metadata to the given parser. The original
/// start state will remain in the program, but with a new name.
static void addEgressMetadata(IR::BFN::TnaParser *parser, const IR::ParserState *start_i2e_mirrored,
                              const IR::ParserState *start_e2e_mirrored,
                              const IR::ParserState *start_coalesced,
                              const IR::ParserState *start_egress,
                              std::map<cstring, const IR::SelectCase *> selMap) {
    auto *p4EntryPointState = convertStartStateToNormalState(parser, "egress_p4_entry_point"_cs);

    // Add a state that parses bridged metadata. This is just a placeholder;
    // we'll replace it once we know which metadata need to be bridged.
    auto *bridgedMetadataState = createGeneratedParserState(
        "bridged_metadata"_cs, {},
        ((start_egress) ? "start_egress"_cs : p4EntryPointState->name.name));
    parser->states.push_back(bridgedMetadataState);

    // Similarly, this state is a placeholder which will eventually hold the
    // parser for mirrored data.
    IR::Vector<IR::Expression> selectOn;
    IR::Vector<IR::SelectCase> branchTo;
    if (start_i2e_mirrored || start_e2e_mirrored || start_coalesced)
        selectOn.push_back(new IR::Member(new IR::PathExpression(new IR::Path(COMPILER_META)),
                                          "instance_type"_cs));
    if (start_i2e_mirrored) {
        BUG_CHECK(selMap.count("start_i2e_mirrored"_cs) != 0,
                  "Couldn't find the start_i2e_mirrored state?");
        branchTo.push_back(selMap.at("start_i2e_mirrored"_cs));
    }
    if (start_e2e_mirrored) {
        BUG_CHECK(selMap.count("start_e2e_mirrored"_cs) != 0,
                  "Couldn't find the start_e2e_mirrored state?");
        branchTo.push_back(selMap.at("start_e2e_mirrored"_cs));
    }
    if (start_coalesced) {
        BUG_CHECK(selMap.count("start_coalesced"_cs) != 0,
                  "Couldn't find the start_coalesced state?");
        branchTo.push_back(selMap.at("start_coalesced"_cs));
    }

    const IR::ParserState *mirroredState = nullptr;
    if (branchTo.size()) {
        mirroredState = createGeneratedParserState(
            "mirrored"_cs, {},
            new IR::SelectExpression(new IR::ListExpression(selectOn), branchTo));
    } else {
        mirroredState = createGeneratedParserState("mirrored"_cs, {}, p4EntryPointState->name);
    }
    parser->states.push_back(mirroredState);

    // If this is a mirrored packet, the hardware will have prepended the
    // contents of the mirror buffer to the actual packet data. To detect this
    // data, we add a byte to the beginning of the mirror buffer that contains a
    // flag indicating that it's a mirrored packet. We can use this flag to
    // distinguish a mirrored packet from a normal packet because we always
    // begin the bridged metadata we attach to normal packet with an extra byte
    // which has the mirror indicator flag set to zero.
    auto packetInParam = parser->tnaParams.at("pkt"_cs);
    selectOn = {createLookaheadExpr(packetInParam, 8)};
    auto *checkMirroredState = createGeneratedParserState(
        "check_mirrored"_cs, {},
        new IR::SelectExpression(new IR::ListExpression(selectOn),
                                 {createSelectCase(8, 0, 1 << 3, bridgedMetadataState),
                                  createSelectCase(8, 1 << 3, 1 << 3, mirroredState)}));
    parser->states.push_back(checkMirroredState);

    // This state handles the extraction of egress intrinsic metadata.
    // auto headerParam = parser->tnaParams.at("hdr");
    auto *egMetadataState = createGeneratedParserState(
        "egress_metadata"_cs,
        {createSetMetadata("eg_intr_md_from_prsr"_cs, "parser_err"_cs, 16, 0),
         // createSetMetadata(parser, "eg_intr_md_from_prsr"_cs, "coalesce_sample_count"_cs, 8, 0),
         createExtractCall(packetInParam, "egress_intrinsic_metadata_t"_cs,
                           parser->tnaParams.at("eg_intr_md"_cs))},
        checkMirroredState->name);
    parser->states.push_back(egMetadataState);

    addNewStartState(parser, "egress_tna_entry_point"_cs, egMetadataState->name);
}

/**
 * Rename possible P4 multientry start state (when \@packet_entry was used).
 */
class RenameP4StartState : public Transform {
    bool found_start = false;

 public:
    IR::ParserState *preorder(IR::ParserState *state) override {
        auto anno = state->getAnnotation("name"_cs);
        if (!anno) return state;
        auto name = anno->expr.at(0)->to<IR::StringLiteral>();
        // We want to check if the .start was found, which indicates that
        // the p4c indeed added start_0 and .$start and we should
        // therefore rename the generated start state
        if (name->value == ".$start") {
            LOG1("Found p4c added start state for @packet_entry");
            found_start = true;
        }
        return state;
    }

    IR::BFN::TnaParser *postorder(IR::BFN::TnaParser *parser) override {
        if (found_start) {
            LOG1("Renaming p4c generated start state for @packet_entry");
            convertStartStateToNormalState(parser, "old_p4_start_point_to_be_removed"_cs);
        }
        return parser;
    }
};

// Add parser code to extract the standard TNA intrinsic metadata.
// This pass is used by the P4-14 to V1model translation path.
class AddMetadataFields : public Transform {
    const IR::ParserState *start_i2e_mirrored = nullptr;
    const IR::ParserState *start_e2e_mirrored = nullptr;
    const IR::ParserState *start_coalesced = nullptr;
    const IR::ParserState *start_egress = nullptr;
    // map the name of 'start_i2e_mirrored' ... to the IR::SelectCase
    // that is used to transit to these state. Used to support extra
    // entry point to P4-14 parser.
    std::map<cstring, const IR::SelectCase *> selectCaseMap;
    cstring p4c_start = nullptr;

 public:
    AddMetadataFields() { setName("AddMetadataFields"); }

    IR::ParserState *preorder(IR::ParserState *state) override {
        auto anno = state->getAnnotation("packet_entry"_cs);
        if (!anno) return state;
        anno = state->getAnnotation("name"_cs);
        auto name = anno->expr.at(0)->to<IR::StringLiteral>();
        if (name->value == ".start_i2e_mirrored") {
            start_i2e_mirrored = state;
        } else if (name->value == ".start_e2e_mirrored") {
            start_e2e_mirrored = state;
        } else if (name->value == ".start_coalesced") {
            start_coalesced = state;
        } else if (name->value == ".start_egress") {
            start_egress = state;
        }
        return state;
    }

    // Delete the compiler-generated start state from frontend.
    IR::ParserState *postorder(IR::ParserState *state) override {
        auto anno = state->getAnnotation("name"_cs);
        if (!anno) return state;
        auto name = anno->expr.at(0)->to<IR::StringLiteral>();
        if (name->value == ".start") {
            LOG1("found start state ");
            // Frontend renames 'start' to 'start_0' if the '@packet_entry' pragma is used.
            // The renamed 'start' state has a "@name('.start')" annotation.
            // The translation is introduced at frontends/p4/fromv1.0/converters.cpp#L1121
            // As we will create Tofino-specific start state in this pass, we will need to ensure
            // that the frontend-generated start state is removed and the original start
            // state is restored before the logic in this pass modifies the start state.
            // Basically, the invariant here is to ensure the start state is unmodified,
            // with or w/o the @packet_entry pragma.
            p4c_start = state->name;
            state->name = IR::ParserState::start;
            return state;
        } else if (name->value == ".$start") {
            auto selExpr = state->selectExpression->to<IR::SelectExpression>();
            BUG_CHECK(selExpr != nullptr, "Couldn't find the select expression?");
            for (auto c : selExpr->selectCases) {
                if (c->state->path->name == "start_i2e_mirrored") {
                    selectCaseMap.emplace("start_i2e_mirrored"_cs, c);
                } else if (c->state->path->name == "start_e2e_mirrored") {
                    selectCaseMap.emplace("start_e2e_mirrored"_cs, c);
                } else if (c->state->path->name == "start_coalesced") {
                    selectCaseMap.emplace("start_coalesced"_cs, c);
                } else if (c->state->path->name == "start_egress") {
                    selectCaseMap.emplace("start_egress"_cs, c);
                }
            }
            return nullptr;
        }
        return state;
    }

    // Also rename paths to start_0
    IR::Path *postorder(IR::Path *path) override {
        if (path->name.name == p4c_start) path->name = IR::ParserState::start;
        return path;
    }

    IR::BFN::TnaParser *postorder(IR::BFN::TnaParser *parser) override {
        if (parser->thread == INGRESS)
            addIngressMetadata(parser);
        else
            addEgressMetadata(parser, start_i2e_mirrored, start_e2e_mirrored, start_coalesced,
                              start_egress, selectCaseMap);
        return parser;
    }
};

class AddIntrinsicMetadata : public PassManager {
 public:
    AddIntrinsicMetadata(P4::ReferenceMap *refMap, P4::TypeMap *typeMap) {
        setName("AddIntrinsicMetadata");
        addPasses({
            new RenameP4StartState(),
            new AddMetadataFields(),
            new P4::CloneExpressions(),
            new P4::ClearTypeMap(typeMap),
            new BFN::TypeChecking(refMap, typeMap, true),
        });
    }
};

}  // namespace BFN

#endif /* BF_P4C_ARCH_INTRINSIC_METADATA_H_ */
