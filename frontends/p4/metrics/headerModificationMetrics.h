#ifndef FRONTENDS_P4_HEADER_MODIFICATION_METRICS_H_
#define FRONTENDS_P4_HEADER_MODIFICATION_METRICS_H_

#include "../../common/resolveReferences/referenceMap.h"
#include "../callGraph.h"
#include "../parserCallGraph.h"
#include "../methodInstance.h"
#include "../ir/ir.h"
#include "../typeChecking/typeChecker.h"
#include "metricsStructure.h"

namespace P4 {

class HeaderModificationMetricsPass : public Inspector {
 private:
    TypeMap* typeMap;
    Metrics& metrics;
    ParserCallGraph parserCallGraph;

    /// Helper function that traverses the parser call graph.
    void dfsCumulativeTypes(
        const IR::ParserState* state,
        const ParserCallGraph& pcg,
        const std::string& parentType,
        PacketModification inheritedMods,
        std::unordered_map<const IR::ParserState*, std::string>& types,
        std::unordered_map<const IR::ParserState*, PacketModification>& inheritedModifications,
        const std::unordered_map<const IR::ParserState*, PacketModification>& stateModifications
    );
    
    /// Helper function that returns the name of a packet type based on an extract call.
    std::string getPacketType(const IR::ParserState* state) const;
    /// Helper function that returns the size of an assignment statement in bits.
    int getOperationSize(const IR::AssignmentStatement* assign);

public:
    HeaderModificationMetricsPass(TypeMap* typeMap, Metrics& metrics)
        : typeMap(typeMap), metrics(metrics), parserCallGraph("parserCallGraph") {
        setName("HeaderModificationMetricsPass");
    }
    bool preorder(const IR::AssignmentStatement* assign) override;
    bool preorder(const IR::P4Parser* parser) override;
    void postorder(const IR::P4Program* program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_HEADER_MODIFICATION_METRICS_H_ */