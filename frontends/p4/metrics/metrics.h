#ifndef FRONTENDS_P4_METRICS_H_
#define FRONTENDS_P4_METRICS_H_

#include <unordered_map>
#include <string>
#include "../ir/ir.h"
#include "lib/ordered_map.h"

namespace P4 {

// Code metric collection option
extern std::unordered_set<std::string> codeMetrics;
// List of valid code metric values
extern std::unordered_set<std::string> validMetrics;

// Define the Metrics structure that contains all the metrics
struct Metrics {
    // Function name -> CC value
    P4::ordered_map<std::string, unsigned> cyclomaticComplexity;

    unsigned duplicateCodeInstances;

    unsigned unusedCodeInstances;

    struct NestingDepth {
        double avgNestingDepth = 0.0;
        unsigned maxNestingDepth = 0;
    } nestingDepth;

    struct HalsteadMetrics {
        unsigned uniqueOperators = 0;
        unsigned uniqueOperands = 0;
        unsigned totalOperators = 0;
        unsigned totalOperands = 0;

        // Derived metrics
        double vocabulary = 0.0;
        double length = 0.0;
        double difficulty = 0.0;
        double volume = 0.0;
        double effort = 0.0;
        double deliveredBugs = 0.0;
    } halsteadMetrics;

    struct HeaderMetrics {
        unsigned numHeaders = 0;
        P4::ordered_map<std::string, unsigned> fieldsNum;     // Header name -> num fields
        P4::ordered_map<std::string, unsigned> fieldSizeSum;  // Header name -> total size
        double avgFieldsNum = 0.0;
        double avgFieldSize = 0.0;
    } headerMetrics;

    // Structure to be used only by other structures
    struct PacketModification {
        unsigned numOperations = 0;
        unsigned totalSize = 0;
    };

    // Metrics related to header addition and removal operations
    struct HeaderManipulationMetrics {
        P4::ordered_map<std::string, PacketModification> perPacketManipulations;  // Packet type -> manipulations
        PacketModification totalManipulations;
        PacketModification maxManipulations;
        PacketModification minManipulations;
    } headerManipulationMetrics;

    // Metrics related to editing operations performed on header fields
    struct HeaderModificationMetrics {
        P4::ordered_map<std::string, PacketModification> perPacketModifications;  // Packet type -> modifications
        PacketModification totalModifications;
        PacketModification maxModifications;
        PacketModification minModifications;
    } headerModificationMetrics;

    struct MatchActionTableMetrics {
        unsigned numTables = 0;
        P4::ordered_map<std::string, unsigned> keysNum;      // Table name -> num keys
        P4::ordered_map<std::string, unsigned> actionsNum;   // Table name -> num actions
        P4::ordered_map<std::string, unsigned> keySizeSum;   // Table name -> sum key sizes

        unsigned totalKeys = 0;
        unsigned totalKeySizeSum = 0;
        double avgKeySize = 0;
        double avgKeysPerTable = 0.0;
        unsigned maxKeysPerTable = 0;

        unsigned totalActions = 0;
        double avgActionsPerTable = 0.0;
        unsigned maxActionsPerTable = 0;
    } matchActionTableMetrics;

    struct ParserMetrics {
        P4::ordered_map<std::string, unsigned> StateNum;        // Parser name -> num states
        P4::ordered_map<std::string, unsigned> StateComplexity; // State name -> complexity
        unsigned totalStates = 0;
    } parserMetrics;

    unsigned inlinedActionsNum;

    unsigned externalObjectsNum;
};

class ExportMetricsPass : public Inspector {
 private:
    std::string filename;  // Output file for metrics
    Metrics &metrics;
 public:
    explicit ExportMetricsPass(const std::string &filename, Metrics &metricsRef)
        : filename(filename), metrics(metricsRef) { setName("ExportMetricsPass");}
    bool preorder(const IR::P4Program *program) override;
};

}  // namespace P4

#endif /* FRONTENDS_P4_METRICS_H_ */