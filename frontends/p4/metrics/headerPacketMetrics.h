/*
Collects header manipulation metrics (extract, setValid and
setInvalid calls), and header modification metrics (assignment
statements where the left side is a header type). Before the main
pass traverses the IR, the ParserAnalyzer pass is ran to determine
cumulative packet types which can be created by the parser
(for example, "ethernet", "ethernet-ipv4", "ethernet-ipv4-tcp").
It does so by creating a parserCallGraph and then traversing it.
Only limitation is that it cannot determine packet types created
with loops completely (it stops when the same state is encountered
in the current path).

After the ParserAnalyzer finishes, the main pass collects the metrics
on a global and per-packet type basis. An operation on a header type is
added to the metrics of all packet types containing that type (so for
hdr.ipv4.ttl = 64, the operation size will be added to both "ethernet-ipv4"
and "ethernet-ipv4-tcp").
*/

#ifndef FRONTENDS_P4_METRICS_HEADERPACKETMETRICS_H_
#define FRONTENDS_P4_METRICS_HEADERPACKETMETRICS_H_

#include "../../common/resolveReferences/referenceMap.h"
#include "../callGraph.h"
#include "../ir/ir.h"
#include "../methodInstance.h"
#include "../parserCallGraph.h"
#include "../typeChecking/typeChecker.h"
#include "metricsStructure.h"

namespace P4 {

class HeaderPacketMetricsPass;

class ParserAnalyzer : public Inspector {
 private:
    ParserCallGraph &parserCallGraph;
    std::set<std::string> &cumulativeTypes;
    std::unordered_map<std::string, size_t> stateEncounters;
    TypeMap *typeMap;

    std::string getPacketType(const IR::ParserState *state) const;
    void dfsCumulativeTypes(const IR::ParserState *state, const ParserCallGraph &pcg,
                            const std::string &parentType,
                            std::unordered_map<const IR::ParserState *, std::string> &types,
                            std::unordered_set<const IR::ParserState *> &currentPath);

 public:
    ParserAnalyzer(ParserCallGraph &graph, std::set<std::string> &types, TypeMap *map)
        : parserCallGraph(graph), cumulativeTypes(types), typeMap(map) {
        setName("ParserAnalyzer");
    }

    bool preorder(const IR::P4Parser *parser) override;
};

class HeaderPacketMetricsPass : public Inspector {
 private:
    TypeMap *typeMap;
    Metrics &metrics;
    ParserCallGraph parserCallGraph;
    std::set<std::string> cumulativeTypes;
    bool insideParserState;
    bool isValid;

    void updateMetrics(const std::string &headerName, size_t size, bool isModification);
    size_t getHeaderFieldSize(const IR::Type *type) const;
    size_t getHeaderSize(const IR::Type_Header *header) const;

 public:
    HeaderPacketMetricsPass(TypeMap *typeMap, Metrics &metrics)
        : typeMap(typeMap), metrics(metrics), parserCallGraph("parserCallGraph") {
        setName("HeaderPacketMetricsPass");
    }

    bool preorder(const IR::P4Program *program) override;
    void postorder(const IR::P4Program * /*program*/) override;
    bool preorder(const IR::AssignmentStatement *assign) override;
    bool preorder(const IR::MethodCallStatement *mcs) override;
    bool preorder(const IR::ParserState * /*state*/) override;
    void postorder(const IR::ParserState * /*state*/) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_METRICS_HEADERPACKETMETRICS_H_ */
