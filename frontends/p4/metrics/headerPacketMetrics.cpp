#include "headerPacketMetrics.h"

namespace P4 {

std::string ParserAnalyzer::getPacketType(const IR::ParserState* state) const {
    for (const auto* stmt : state->components) {
        if (const auto* methodCall = stmt->to<IR::MethodCallStatement>()) {
            const auto* method = methodCall->methodCall;
            if (const auto* member = method->method->to<IR::Member>()) {
                if (member->member.name == "extract" && !method->arguments->empty()) {
                    if (const auto* arg = method->arguments->at(0)->expression->to<IR::Member>()) {
                        return arg->member.name.string();
                    }
                }
            }
        }
    }
    return "";
}

void ParserAnalyzer::dfsCumulativeTypes(
    const IR::ParserState*                          state,
    const ParserCallGraph&                          pcg,
    const std::string&                              parentType,
    std::unordered_map<const IR::ParserState*, std::string>& types,
    std::unordered_set<const IR::ParserState*>&     currentPath) {

    // If this state was already visited on the current path, stop.
    if (!currentPath.insert(state).second) return;
        
    auto extract = getPacketType(state);
    auto cumulative = parentType.empty() ? extract : (extract.empty() ? parentType : parentType + "-" + extract);
    types[state] = cumulative;
    cumulativeTypes.insert(cumulative);

    if (auto* callees = const_cast<ParserCallGraph&>(pcg).getCallees(state)) {
        for (auto* next : *callees)
            dfsCumulativeTypes(next, pcg, cumulative, types, currentPath);
    }

    currentPath.erase(state);
}


bool ParserAnalyzer::preorder(const IR::P4Parser* parser) {
    parserCallGraph = ParserCallGraph(parser->name.name);
    ComputeParserCG buildGraph(&parserCallGraph);
    parser->apply(buildGraph);

    if (auto* start = parser->getDeclByName(IR::ParserState::start)) {
        std::unordered_map<const IR::ParserState*, std::string> types;
        std::unordered_set<const IR::ParserState*> currentPath;
        dfsCumulativeTypes(start->to<IR::ParserState>(), parserCallGraph, "", types, currentPath);
    }

    return false;
}

size_t HeaderPacketMetricsPass::getHeaderFieldSize(const IR::Type* type) const {
    const IR::Type* cur = type;
    while (cur) {
        if (auto tb = cur->to<IR::Type_Bits>())     return tb->width_bits();
        if (auto tt = cur->to<IR::Type_Type>())     cur = tt->type;
        else if (auto nt = cur->to<IR::Type_Newtype>()) cur = typeMap->getType(nt->type, true);
        else if (auto td = cur->to<IR::Type_Typedef>()) cur = typeMap->getType(td->type, true);
        else break;
    }
    return 0;
}

size_t HeaderPacketMetricsPass::getHeaderSize(const IR::Type_Header* header) const {
    size_t size = 0;
    for (auto field : header->fields) {
        const IR::Type* currentType = typeMap->getType(field->type, true);
        size += getHeaderFieldSize(currentType);
    }
    return size;
}

void HeaderPacketMetricsPass::updateMetrics(const std::string& headerName, size_t size, bool isModification) {
    for (const auto &packetType : cumulativeTypes) {
        if (packetType.find(headerName) != std::string::npos) {
            auto& target = isModification
                ? metrics.headerModificationMetrics.perPacket[packetType]
                : metrics.headerManipulationMetrics.perPacket[packetType];

            target.numOperations++;
            target.totalSize += size;
        }
    }

    auto& total = isModification
        ? metrics.headerModificationMetrics.total
        : metrics.headerManipulationMetrics.total;

    total.numOperations++;
    total.totalSize += size;
}

bool HeaderPacketMetricsPass::preorder(const IR::P4Program* program){
    ParserAnalyzer analyzer(parserCallGraph, cumulativeTypes, typeMap);
    program->apply(analyzer);
    return true;
}

bool HeaderPacketMetricsPass::preorder(const IR::ParserState* /*state*/){
    insideParserState = true;
    isValid = false;
    return true;
}

void HeaderPacketMetricsPass::postorder (const IR::ParserState* /*state*/){
    insideParserState = isValid = false;
}

bool HeaderPacketMetricsPass::preorder(const IR::AssignmentStatement* assign) {
    auto left = assign->left;
    auto member = left->to<IR::Member>();
    if (!member) return false;

    const IR::Type* leftType = typeMap->getType(left, true);
    if (!leftType) return false;

    size_t size = 0;
    std::string headerName;

    // Assignment to entire header.
    if (auto headerType = leftType->to<IR::Type_Header>()) {
        size = getHeaderSize(headerType);
        headerName = member->member.name.string();
    }
    // Assignment to a header field.
    else {
        const IR::Type* baseType = typeMap->getType(member->expr, true);
        if (!baseType || !baseType->is<IR::Type_Header>()) return false;

        size = getHeaderFieldSize(leftType);
        if (auto baseMember = member->expr->to<IR::Member>()) {
            headerName = baseMember->member.name.string();
        } else {
            return false;
        }
    }

    updateMetrics(headerName, size, true);
    return false;
}

bool HeaderPacketMetricsPass::preorder(const IR::MethodCallStatement* mcs) {
    auto methodCall = mcs->methodCall;
    const IR::Expression *baseExpr = nullptr;
    const IR::Type *baseType = nullptr;

    if (const auto* member = methodCall->method->to<IR::Member>()) {
        auto methodName = member->member.name;
        if (methodName == "setValid" || methodName == "setInvalid") {
            baseExpr = member->expr;
        } else if (methodName == "extract" && !methodCall->arguments->empty()) {
            baseExpr = methodCall->arguments->at(0)->expression;
        }

        if(insideParserState){
            if (isValid){
                if (methodName != "setInvalid") return false;
            }
            else if (methodName == "extract" || methodName == "setValid"){
                isValid = true;
            }
        }

        if (baseExpr) {
            baseType = typeMap->getType(baseExpr, true);
            if (baseType && baseType->is<IR::Type_Header>()) {
                auto headerType = baseType->to<IR::Type_Header>();
                int size = getHeaderSize(headerType);
                std::string headerName;

                if (auto* baseMember = baseExpr->to<IR::Member>()) {
                    headerName = baseMember->member.name.string();
                } else {
                    headerName = headerType->toString().string();
                }

                updateMetrics(headerName, size, false);
            }
        }
    }

    return false;
}

void HeaderPacketMetricsPass::postorder(const IR::P4Program* /*program*/) {
    auto& mod = metrics.headerModificationMetrics.perPacket;
    if (!mod.empty()) {
        metrics.headerModificationMetrics.max =
            std::max_element(mod.begin(), mod.end(),
                [](const auto& a, const auto& b) {
                    return a.second.numOperations < b.second.numOperations;
                })->second;

        metrics.headerModificationMetrics.min =
            std::min_element(mod.begin(), mod.end(),
                [](const auto& a, const auto& b) {
                    return a.second.numOperations < b.second.numOperations;
                })->second;
    }

    auto& manip = metrics.headerManipulationMetrics.perPacket;
    if (!manip.empty()) {
        metrics.headerManipulationMetrics.max =
            std::max_element(manip.begin(), manip.end(),
                [](const auto& a, const auto& b) {
                    return a.second.numOperations < b.second.numOperations;
                })->second;

        metrics.headerManipulationMetrics.min =
            std::min_element(manip.begin(), manip.end(),
                [](const auto& a, const auto& b) {
                    return a.second.numOperations < b.second.numOperations;
                })->second;
    }
}

}  // namespace P4