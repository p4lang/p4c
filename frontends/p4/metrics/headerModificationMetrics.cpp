#include "headerModificationMetrics.h"

namespace P4 {

int HeaderModificationMetricsPass::getOperationSize(const IR::AssignmentStatement* assign) {
    auto member = assign->left->to<IR::Member>();
    if (!member) {
        return -1;
    }
    // Check that the base of the member is a header instance.
    auto baseType = typeMap->getType(member->expr, true);
    if (!baseType || !baseType->is<IR::Type_Header>()) {
        return -1;
    }
    // Ensure the field itself is a bit field.
    auto leftType = typeMap->getType(assign->left, true);
    if (!leftType || !leftType->is<IR::Type_Bits>()) {
        return -1;
    }
    return leftType->to<IR::Type_Bits>()->size;
}

bool HeaderModificationMetricsPass::preorder(const IR::AssignmentStatement* assign) {
    int opSize = getOperationSize(assign);
    if (opSize >= 0) {
        metrics.headerModificationMetrics.totalModifications.numOperations++;
        metrics.headerModificationMetrics.totalModifications.totalSize += opSize;
    }
    return false;
}

std::string HeaderModificationMetricsPass::getPacketType(const IR::ParserState* state) const {
    for (const auto* stmt : state->components) {
        if (const auto* methodCall = stmt->to<IR::MethodCallStatement>()) {
            auto* method = methodCall->methodCall;
            if (const auto* member = method->method->to<IR::Member>()) {
                if (member->member.name == "extract") {
                    if (!method->arguments->empty()) {
                        if (const auto* arg = method->arguments->at(0)->expression->to<IR::Member>()) {
                            return arg->member.name.string();
                        }
                    }
                }
            }
        }
    }
    return "";
}

void HeaderModificationMetricsPass::dfsCumulativeTypes(
    const IR::ParserState* state,
    const ParserCallGraph& pcg,
    const std::string& parentType,
    PacketModification inheritedMods,
    std::unordered_map<const IR::ParserState*, std::string>& types,
    std::unordered_map<const IR::ParserState*, PacketModification>& inheritedModifications,
    const std::unordered_map<const IR::ParserState*, PacketModification>& stateModifications) {
    
    std::string extract = getPacketType(state);
    std::string cumulative = parentType.empty() ? extract : (extract.empty() ? parentType : parentType + "-" + extract);
    types[state] = cumulative; // Map state to packet type name

    // Calculate cumulative modifications for this state.
    PacketModification cumulativeMods = inheritedMods;
    cumulativeMods.numOperations += stateModifications.at(state).numOperations;
    cumulativeMods.totalSize += stateModifications.at(state).totalSize;
    inheritedModifications[state] = cumulativeMods;

    LOG3("State " << state->name << " cumulative type: " << cumulative
         << ", modifications: " << cumulativeMods.numOperations << " ops, "
         << cumulativeMods.totalSize << " bits");

    auto callees = const_cast<ParserCallGraph&>(pcg).getCallees(state);
    if (callees) {
        for (const auto* nextState : *callees) {
            dfsCumulativeTypes(nextState, pcg, cumulative, cumulativeMods,
                               types, inheritedModifications, stateModifications);
        }
    }
}

bool HeaderModificationMetricsPass::preorder(const IR::P4Parser* parser) {
    LOG3("Processing parser: " << parser->name);
    ParserCallGraph parserCallGraph(parser->name.name.string());
    ComputeParserCG cp(&parserCallGraph);
    parser->apply(cp);

    // Calculate the number and size of modifications in each state
    std::unordered_map<const IR::ParserState*, PacketModification> stateModifications;
    for (const auto* state : parser->states) {
        PacketModification mods;
        for (const auto* stmt : state->components) {
            if (const auto* assign = stmt->to<IR::AssignmentStatement>()) {
                int opSize = getOperationSize(assign);
                if (opSize >= 0) {
                    mods.numOperations++;
                    mods.totalSize += opSize;
                }
            }
        }
        stateModifications[state] = mods;
        LOG3("State " << state->name << " has " << mods.numOperations << " modifications");
    }

    // Compute cumulative packet types and inherited modifications.
    std::unordered_map<const IR::ParserState*, std::string> cumulativeTypes;
    std::unordered_map<const IR::ParserState*, PacketModification> inheritedModifications;
    auto startDecl = parser->getDeclByName(IR::ParserState::start);
    if (startDecl) {
        dfsCumulativeTypes(
            startDecl->to<IR::ParserState>(),
            parserCallGraph, 
            "", 
            PacketModification(), 
            cumulativeTypes, 
            inheritedModifications, 
            stateModifications
        );
    }

    for (const auto& cType : cumulativeTypes) {
        if (!cType.second.empty()){
            metrics.headerModificationMetrics.perPacketModifications[cType.second].numOperations = inheritedModifications[cType.first].numOperations;
            metrics.headerModificationMetrics.perPacketModifications[cType.second].totalSize = inheritedModifications[cType.first].totalSize;
        }
    }
    
    return true;
}

void HeaderModificationMetricsPass::postorder(const IR::P4Program* program) {
    if (!metrics.headerModificationMetrics.perPacketModifications.empty()) {
        auto& perPacket = metrics.headerModificationMetrics.perPacketModifications;
        metrics.headerModificationMetrics.maxModifications = 
            std::max_element(perPacket.begin(), perPacket.end(),
                [](const auto& a, const auto& b) {
                    return a.second.numOperations < b.second.numOperations;
                })->second;
        metrics.headerModificationMetrics.minModifications = 
            std::min_element(perPacket.begin(), perPacket.end(),
                [](const auto& a, const auto& b) {
                    return a.second.numOperations < b.second.numOperations;
                })->second;
    }
}

}  // namespace P4
