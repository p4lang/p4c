#ifndef FRONTENDS_P4_METRICS_STRUCTURE_H_
#define FRONTENDS_P4_METRICS_STRUCTURE_H_

#include <string>
#include "lib/ordered_map.h"

namespace P4 {

struct PacketModification {
    unsigned numOperations = 0;
    unsigned totalSize = 0;
};

struct UnusedCodeInstances {
    unsigned variables = 0;
    unsigned states = 0;
    unsigned enums = 0;
    unsigned blocks = 0;
    unsigned conditionals = 0;
    unsigned switches = 0;
    unsigned actions = 0;         
    unsigned functions = 0;       
    unsigned parameters = 0;      
    unsigned returns = 0;         

    // Overload "-" and prevent negative deltas
    UnusedCodeInstances operator-(const UnusedCodeInstances& other) const {
        UnusedCodeInstances result;
        result.variables = (variables > other.variables) ? (variables - other.variables) : 0;
        result.states = (states > other.states) ? (states - other.states) : 0;
        result.enums = (enums > other.enums) ? (enums - other.enums) : 0;
        result.blocks = (blocks > other.blocks) ? (blocks - other.blocks) : 0;
        result.conditionals = (conditionals > other.conditionals) ? (conditionals - other.conditionals) : 0;
        result.switches = (switches > other.switches) ? (switches - other.switches) : 0;
        result.actions = (actions > other.actions) ? (actions - other.actions) : 0;
        result.functions = (functions > other.functions) ? (functions - other.functions) : 0;
        result.parameters = (parameters > other.parameters) ? (parameters - other.parameters) : 0;
        result.returns = (returns > other.returns) ? (returns - other.returns) : 0;
        return result;
    }
};
    
struct Metrics {
    
    struct NestingDepth {
        P4::ordered_map<std::string, unsigned> blockNestingDepth; // Block name -> max depth
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
        P4::ordered_map<std::string, unsigned> StateComplexity; // State name -> complexity
        unsigned totalStates = 0;
    } parserMetrics;

    P4::ordered_map<std::string, unsigned> cyclomaticComplexity; // Function name -> CC value

    unsigned duplicateCodeInstances = 0;

    unsigned inlinedActionsNum = 0;

    unsigned externalObjectsNum = 0;

    UnusedCodeInstances unusedCodeInstances;
    
    // Variables for storing inter-pass data.
    UnusedCodeInstances interPassCounts;
    std::vector<std::string> beforeActions;
    std::vector<std::string> afterActions;
    std::vector<std::string> beforeVariables;  
    std::vector<std::string> afterVariables;    
};

} // namespace P4
#endif // METRICS_STRUCTURE_H_