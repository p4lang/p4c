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

#include "frontends/common/resolveReferences/referenceMap.h"
#include "frontends/p4/callGraph.h"
#include "frontends/p4/methodInstance.h"
#include "frontends/p4/metrics/metricsStructure.h"
#include "frontends/p4/parserCallGraph.h"
#include "frontends/p4/typeChecking/typeChecker.h"
#include "ir/ir.h"

using namespace P4::literals;

namespace P4 {

class HeaderPacketMetricsPass;

class ParserAnalyzer : public Inspector {
 private:
    ParserCallGraph &parserCallGraph;
    std::set<cstring> &cumulativeTypes;
    std::unordered_map<cstring, size_t> stateEncounters;

    cstring getPacketType(const IR::ParserState *state) const;
    void dfsCumulativeTypes(const IR::ParserState *state, const ParserCallGraph &pcg,
                            const cstring &parentType,
                            std::unordered_map<const IR::ParserState *, cstring> &types,
                            std::unordered_set<const IR::ParserState *> &currentPath);

 public:
    ParserAnalyzer(ParserCallGraph &graph, std::set<cstring> &types)
        : parserCallGraph(graph), cumulativeTypes(types) {
        setName("ParserAnalyzer");
    }

    bool preorder(const IR::P4Parser *parser) override;
};

class HeaderPacketMetricsPass : public Inspector {
 private:
    TypeMap *typeMap;
    Metrics &metrics;
    ParserCallGraph parserCallGraph;
    std::set<cstring> cumulativeTypes;
    bool insideParserState;
    bool isValid;

    void updateMetrics(const cstring &headerName, size_t size, bool isModification);
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
