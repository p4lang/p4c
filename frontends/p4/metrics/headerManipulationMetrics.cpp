#include "headerManipulationMetrics.h"

namespace P4 {

int HeaderManipulationMetricsPass::getHeaderSize(const IR::Type_Header* header) const {
    int size = 0;
    // Sum the sizes of each field in the header.
    for (const auto* field : header->fields) {
        auto fieldType = typeMap->getType(field, true);
        if (fieldType && fieldType->is<IR::Type_Bits>()) {
            size += fieldType->to<IR::Type_Bits>()->size;
        }
    }
    return size;
}

bool HeaderManipulationMetricsPass::preorder(const IR::MethodCallStatement* mcs) {
    auto methodCall = mcs->methodCall;
    if (const auto* member = methodCall->method->to<IR::Member>()) {
        auto methodName = member->member.name;
        if (methodName == "setValid" || methodName == "setInvalid") {
            // Get the header instance on which the method is invoked.
            auto baseExpr = member->expr;
            auto baseType = typeMap->getType(baseExpr, true);
            if (baseType && baseType->is<IR::Type_Header>()) {
                auto headerType = baseType->to<IR::Type_Header>();
                int opSize = getHeaderSize(headerType);
                std::string packetTypeName;
                if (auto* baseMember = baseExpr->to<IR::Member>()) {
                    packetTypeName = baseMember->member.name.string();
                } else {
                    packetTypeName = headerType->toString().string();
                }
                metrics.headerManipulationMetrics.perPacketManipulations[packetTypeName].numOperations++;
                metrics.headerManipulationMetrics.perPacketManipulations[packetTypeName].totalSize += opSize;
                metrics.headerManipulationMetrics.totalManipulations.numOperations++;
                metrics.headerManipulationMetrics.totalManipulations.totalSize += opSize;
            }
        }
    }
    return false;
}

void HeaderManipulationMetricsPass::postorder(const IR::P4Program* /*program*/) {
    auto& perPacket = metrics.headerManipulationMetrics.perPacketManipulations;
    if (!perPacket.empty()) {
        metrics.headerManipulationMetrics.maxManipulations =
            std::max_element(perPacket.begin(), perPacket.end(),
                [](const auto& a, const auto& b) {
                    return a.second.numOperations < b.second.numOperations;
                })->second;

        metrics.headerManipulationMetrics.minManipulations =
            std::min_element(perPacket.begin(), perPacket.end(),
                [](const auto& a, const auto& b) {
                    return a.second.numOperations < b.second.numOperations;
                })->second;
    }
}

}  // namespace P4
