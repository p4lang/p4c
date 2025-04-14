#ifndef FRONTENDS_P4_HEADER_PACKET_METRICS_H_
#define FRONTENDS_P4_HEADER_PACKET_METRICS_H_

#include "../../common/resolveReferences/referenceMap.h"
#include "../callGraph.h"
#include "../parserCallGraph.h"
#include "../methodInstance.h"
#include "../ir/ir.h"
#include "../typeChecking/typeChecker.h"
#include "metricsStructure.h"

namespace P4 {

class HeaderPacketMetricsPass;

class ParserAnalyzer : public Inspector {
 private:
    ParserCallGraph& parserCallGraph;
    std::set<std::string>& cumulativeTypes;
    TypeMap* typeMap;

    std::string getPacketType(const IR::ParserState* state) const;
    void dfsCumulativeTypes(
        const IR::ParserState* state,
        const ParserCallGraph& pcg,
        const std::string& parentType,
        std::unordered_map<const IR::ParserState*, std::string>& types);

 public:
    ParserAnalyzer(ParserCallGraph& graph, std::set<std::string>& types, TypeMap* map)
        : parserCallGraph(graph), cumulativeTypes(types), typeMap(map) {
        setName("ParserAnalyzer");
    }

    bool preorder(const IR::P4Parser* parser) override;
};

class HeaderPacketMetricsPass : public Inspector {
 private:
    TypeMap* typeMap;
    Metrics& metrics;
    ParserCallGraph parserCallGraph;
    std::set<std::string> cumulativeTypes;
    bool insideParserState;
    bool isValid;

    void updateMetrics(const std::string& headerName, size_t size, bool isModification);
    size_t getHeaderFieldSize(const IR::Type* type) const;
    size_t getHeaderSize(const IR::Type_Header* header) const;

 public:
    HeaderPacketMetricsPass(TypeMap* typeMap, Metrics& metrics)
        : typeMap(typeMap), metrics(metrics), parserCallGraph("parserCallGraph") {
        setName("HeaderPacketMetricsPass");
    }
    /// Ensure the parser analyzer collects cumulative packet types first.
    bool preorder(const IR::P4Program* program) override;
    void postorder(const IR::P4Program* /*program*/) override;
    bool preorder(const IR::AssignmentStatement* assign) override;
    bool preorder(const IR::MethodCallStatement* mcs) override;
    bool preorder(const IR::ParserState* /*state*/) override;
    void postorder (const IR::ParserState* /*state*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_HEADER_PACKET_METRICS_H_ */
