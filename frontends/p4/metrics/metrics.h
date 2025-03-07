#ifndef FRONTENDS_P4_METRICS_H_
#define FRONTENDS_P4_METRICS_H_

#include <unordered_map>
#include <string>
#include "../ir/ir.h"

namespace P4 {

// Code metric collection option
extern std::unordered_set<std::string> codeMetrics;
// List of valid code metric values
extern std::unordered_set<std::string> validMetrics;
// Function name -> CC value
extern std::unordered_map<std::string, int> cyclomaticComplexity; 

extern int duplicateCodeInstances;

extern int unusedCodeInstances;

struct NestingDepth {
    double avgNestingDepth = 0.0;
    int maxNestingDepth = 0;
};
extern NestingDepth nestingDepth;

struct HalsteadMetrics {
    int uniqueOperators = 0;
    int uniqueOperands = 0;
    int totalOperators = 0;
    int totalOperands = 0;

    // Derived metrics
    double vocabulary = 0.0;
    double length = 0.0;
    double difficulty = 0.0;
    double volume = 0.0;
    double effort = 0.0;
    double deliveredBugs = 0.0;
};
extern HalsteadMetrics halsteadMetrics;
 
struct HeaderMetrics {
    int numHeaders = 0;
    std::unordered_map<std::string, int> fieldsNum;     // Header name -> num fields
    std::unordered_map<std::string, int> fieldSizeSum;  // Header name -> total size
    double avgFieldsNum = 0.0;
    double avgFieldSize = 0.0;    
};
extern HeaderMetrics headerMetrics;

// Structure to be used only by other structures
struct PacketModification {
    int numOperations = 0;
    int totalSize = 0;
};

// Metrics related to header addition and removal operations 
struct HeaderManipulationMetrics {
    std::unordered_map<std::string, PacketModification> perPacketManipulations;  // Packet type -> manipulations
    PacketModification totalManipulations;
    PacketModification maxManipulations;
    PacketModification minManipulations;
};
extern HeaderManipulationMetrics headerManipulationMetrics;

// Metrics related to editing operations performed on header fields
struct HeaderModificationMetrics{
    std::unordered_map<std::string, PacketModification> perPacketModifications;  // Packet type -> modifications
    PacketModification totalModifications;
    PacketModification maxModifications;
    PacketModification minModifications;
};
extern HeaderModificationMetrics headerModificationMetrics;

struct MatchActionTableMetrics {
    int numTables = 0;
    std::unordered_map<std::string, int> keysNum;      // Table name -> num keys
    std::unordered_map<std::string, int> actionsNum;   // Table name -> num actions
    std::unordered_map<std::string, int> keySizeSum;   // Table name -> sum key sizes
    double avgKeySizePerTable = 0.0;

    int totalKeys = 0;
    int totalActions = 0;
    double avgActionsPerTable = 0.0;
    int maxActionsPerTable = 0;
    double avgKeysPerTable = 0.0;
    int maxKeysPerTable = 0;
};
extern MatchActionTableMetrics matchActionTableMetrics;

struct ParserMetrics {
    std::unordered_map<std::string, int> StateNum;        // Parser name -> num states
    std::unordered_map<std::string, int> StateComplexity; // State name -> complexity
    int totalStates = 0;
};
extern ParserMetrics parserMetrics;

extern int inlinedActionsNum;

extern int externalObjectsNum;

class ExportMetricsPass : public Inspector {
 public:
    explicit ExportMetricsPass(const std::string &filename) : filename(filename) {
        setName("ExportMetricsPass");
    }

    bool preorder(const IR::P4Program *program) override;

 private:
    std::string filename;  // Output file for metrics
};

}  // namespace P4

#endif /* FRONTENDS_P4_METRICS_H_ */
