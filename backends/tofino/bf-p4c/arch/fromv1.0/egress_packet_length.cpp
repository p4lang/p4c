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


#include "egress_packet_length.h"

#include "bf-p4c/midend/type_checker.h"

// Check whether egress parser parses mirror packets
class MirrorEgressParseEval : public Inspector {
    bool& egressParsesMirror;

    // Process only parser states under egress parser
    bool preorder(const IR::Node*) { return false; }
    bool preorder(const IR::P4Program*) { return true; }
    bool preorder(const IR::BFN::TnaParser* parser) { return parser->thread == EGRESS; }
    bool preorder(const IR::ParserState* state) {
        const cstring& stateName = state->getName();
        if (stateName.startsWith("__mirror_field_list_") ||
            stateName.startsWith("__parse_ingress_mirror_header_") ||
            stateName.startsWith("__parse_egress_mirror_header_")) {
            egressParsesMirror = true;
        }
        return false;
    }

 public:
    explicit MirrorEgressParseEval(bool& egressParsesMirror)
        : egressParsesMirror(egressParsesMirror) {
        egressParsesMirror = false;
    }
};

// Check whether egress control uses packet length metadata field
class PacketLengthEgressUseEval : public Inspector {
    bool& egressUsesPacketLength;
    P4::ReferenceMap* refMap;

    // Process only members in egress control's IR subtree
    bool preorder(const IR::Node*) { return findContext<IR::BFN::TnaControl>(); }
    bool preorder(const IR::P4Program*) { return true; }
    bool preorder(const IR::BFN::TnaControl* control) { return control->thread == EGRESS; }
    bool preorder(const IR::Member* member) {
        if (!member->expr->is<IR::PathExpression>() || member->member.name != "pkt_length")
            return false;
        auto* decl = refMap->getDeclaration(member->expr->to<IR::PathExpression>()->path);
        auto* control = findContext<IR::BFN::TnaControl>();
        if (decl && control && decl->getName().name == control->tnaParams.at("eg_intr_md"_cs)) {
            egressUsesPacketLength = true;
        }
        return false;
    }

 public:
    PacketLengthEgressUseEval(bool& egressUsesPacketLength, P4::ReferenceMap* refMap)
        : egressUsesPacketLength(egressUsesPacketLength), refMap(refMap) {
        egressUsesPacketLength = false;
    }
};

// Adjust egress packet length for mirror packets
class EgressPacketLengthAdjust : public Transform {
    // Add "egress_pkt_len_adjust" field to "compiler_generated_metadata_t" structure
    const IR::Node* preorder(IR::Type_Struct* typeStruct) {
        // Ignore other type structures
        if (typeStruct->name != "compiler_generated_metadata_t" || !getParent<IR::P4Program>())
            return typeStruct;

        // Add "egress_pkt_len_adjust" field to type structure
        auto* newCompGenMetaStruct = typeStruct->clone();
        newCompGenMetaStruct->fields.push_back(new IR::StructField(IR::ID("egress_pkt_len_adjust"),
                                               IR::Type::Bits::get(16)));
        return newCompGenMetaStruct;
    }

    // Add "egress_pkt_len_adjust = sizeInBytes(<mirror header>)" to egress mirror parser states
    const IR::Node* preorder(IR::ParserState* state) {
        // Ignore other parsers (ingress) and parser states
        const auto* parser = getParent<IR::BFN::TnaParser>();
        if (!parser || parser->thread != EGRESS)
            return state;
        const cstring& stateName = state->getName();
        if (!stateName.startsWith("__mirror_field_list_") &&
            !stateName.startsWith("__parse_ingress_mirror_header_") &&
            !stateName.startsWith("__parse_egress_mirror_header_"))
            return state;

        // Find extracted mirror header and assign "egress_pkt_len_adjust"
        auto* newEgressMirrorParserState = state->clone();
        for (auto* component : state->components) {
            // Ignore non-extract statements
            auto* methodCallStatement = component->to<IR::MethodCallStatement>();
            if (!methodCallStatement || methodCallStatement->methodCall->arguments->size() < 1)
                continue;
            auto* member = methodCallStatement->methodCall->method->to<IR::Member>();
            if (!member || member->member.name != "extract")
                continue;
            auto* pathExpr = member->expr->to<IR::PathExpression>();
            if (!pathExpr || pathExpr->path->name.name != parser->tnaParams.at("pkt"_cs))
                continue;

            // Extracted mirror header
            auto* argument = methodCallStatement->methodCall->arguments->at(0)->expression;
            // Add "egress_pkt_len_adjust" assignment to parse state
            auto& compGenMeta = parser->tnaParams.at("__bfp4c_compiler_generated_meta"_cs);
            auto* egPktLenAdjust = new IR::Member(new IR::PathExpression(compGenMeta),
                                                  IR::ID("egress_pkt_len_adjust"));
            auto* method = new IR::PathExpression("sizeInBytes");
            auto* typeArgs = new IR::Vector<IR::Type>({ argument->type });
            auto* args = new IR::Vector<IR::Argument>({ new IR::Argument(argument) });
            auto* sizeInBytes = new IR::Cast(IR::Type::Bits::get(16),
                                             new IR::MethodCallExpression(method, typeArgs, args));
            auto* egPktLenAdjustAssignment = new IR::AssignmentStatement(egPktLenAdjust,
                                                                         sizeInBytes);
            newEgressMirrorParserState->components.push_back(egPktLenAdjustAssignment);
            // Only one assignment is expected/allowed
            break;
        }
        return newEgressMirrorParserState;
    }

    // Add "pkt_length = pkt_length - egress_pkt_len_adjust" to egress control block
    const IR::Node* preorder(IR::BlockStatement* blockStatement) {
        // Ignore other control blocks (ingress)
        const auto* control = getParent<IR::BFN::TnaControl>();
        if (!control || control->thread != EGRESS)
            return blockStatement;

        // Add packet length adjustment action/assignment to control block
        auto* newEgressControlBlock = blockStatement->clone();
        auto& compGenMeta = control->tnaParams.at("__bfp4c_compiler_generated_meta"_cs);
        auto* egPktLenAdjust = new IR::Member(new IR::PathExpression(compGenMeta),
                                                IR::ID("egress_pkt_len_adjust"));
        auto& egIntMeta = control->tnaParams.at("eg_intr_md"_cs);
        auto* egPktLen = new IR::Member(new IR::PathExpression(egIntMeta),
                                        IR::ID("pkt_length"));
        auto* egPktLenAdjustAction = new IR::AssignmentStatement(egPktLen,
                                                                 new IR::Sub(egPktLen,
                                                                             egPktLenAdjust));
        newEgressControlBlock->components.insert(newEgressControlBlock->components.begin(),
                                                 egPktLenAdjustAction);
        return newEgressControlBlock;
    }
};

AdjustEgressPacketLength::AdjustEgressPacketLength(P4::ReferenceMap* refMap, P4::TypeMap* typeMap) {
    addPasses({
        new MirrorEgressParseEval(egressParsesMirror),
        new PacketLengthEgressUseEval(egressUsesPacketLength, refMap),
        new PassIf([this]() { return egressParsesMirror && egressUsesPacketLength; }, {
            new EgressPacketLengthAdjust(),
            new P4::ClearTypeMap(typeMap),
            // BFN::TypeChecking with read-write BFN::TypeInference:
            new P4::ResolveReferences(refMap),
            new BFN::TypeInference(typeMap, false),
            new P4::ApplyTypesToExpressions(typeMap),
            new P4::ResolveReferences(refMap) })
    });
}
