/*
Structure definitions used by the code metric collection passes.
*/

#ifndef FRONTENDS_P4_METRICS_STRUCTURE_H_
#define FRONTENDS_P4_METRICS_STRUCTURE_H_

#include <string>
#include "lib/ordered_map.h"

namespace P4 {

struct PacketModification {
    unsigned numOperations = 0;
    unsigned totalSize = 0;
};

struct HeaderPacketMetrics {
    P4::ordered_map<std::string, PacketModification> perPacket;  // Packet type -> operations.
    PacketModification total;
    PacketModification max;
    PacketModification min;
};

struct UnusedCodeInstances {
    unsigned variables = 0;
    unsigned states = 0;
    unsigned enums = 0;
    unsigned conditionals = 0;
    unsigned actions = 0;         
    unsigned functions = 0;       
    unsigned parameters = 0;      
    unsigned returns = 0;         

    // Overloads "-" to prevent negative deltas.
    UnusedCodeInstances operator-(const UnusedCodeInstances& other) const {
        UnusedCodeInstances result;
        result.variables = (variables > other.variables) ? (variables - other.variables) : 0;
        result.states = (states > other.states) ? (states - other.states) : 0;
        result.enums = (enums > other.enums) ? (enums - other.enums) : 0;
        result.conditionals = (conditionals > other.conditionals) ? (conditionals - other.conditionals) : 0;
        result.actions = (actions > other.actions) ? (actions - other.actions) : 0;
        result.functions = (functions > other.functions) ? (functions - other.functions) : 0;
        result.parameters = (parameters > other.parameters) ? (parameters - other.parameters) : 0;
        result.returns = (returns > other.returns) ? (returns - other.returns) : 0;
        return result;
    }
};

struct ExternMetrics {
    unsigned externFunctions = 0;
    unsigned externStructures = 0;
    unsigned externFunctionUses = 0;
    unsigned externStructUses = 0;
};

struct UnusedCodeHelperVars { // Variables for storing inter-pass data.
    UnusedCodeInstances interPassCounts;
    std::vector<std::string> beforeActions;
    std::vector<std::string> afterActions;
    std::vector<std::string> beforeVariables;  
    std::vector<std::string> afterVariables;    
};

struct MatchActionTableMetrics {
    unsigned numTables = 0;
    // Table name -> value
    P4::ordered_map<std::string, unsigned> keysNum;
    P4::ordered_map<std::string, unsigned> actionsNum;
    P4::ordered_map<std::string, unsigned> keySizeSum;

    unsigned totalKeys = 0;
    unsigned totalKeySizeSum = 0;
    double avgKeySize = 0;
    double avgKeysPerTable = 0.0;
    unsigned maxKeysPerTable = 0;

    unsigned totalActions = 0;
    double avgActionsPerTable = 0.0;
    unsigned maxActionsPerTable = 0;
};

struct NestingDepthMetrics {
    P4::ordered_map<std::string, unsigned> blockNestingDepth; // Block name -> max depth.
    double avgNestingDepth = 0.0;
    unsigned maxNestingDepth = 0;
};

struct ParserMetrics {
    P4::ordered_map<std::string, unsigned> StateComplexity; // State name -> complexity.
    unsigned totalStates = 0;
};

struct HalsteadMetrics {
    unsigned uniqueOperators = 0;
    unsigned uniqueOperands = 0;
    unsigned totalOperators = 0;
    unsigned totalOperands = 0;
    unsigned vocabulary = 0.0;
    unsigned length = 0.0;
    double difficulty = 0.0;
    double volume = 0.0;
    double effort = 0.0;
    double deliveredBugs = 0.0;
};

struct HeaderMetrics {
    unsigned numHeaders = 0;
    double avgFieldsNum = 0.0;
    double avgFieldSize = 0.0;
    // Header name -> value
    P4::ordered_map<std::string, unsigned> fieldsNum;     
    P4::ordered_map<std::string, unsigned> fieldSizeSum;
};

struct Metrics {
    unsigned linesOfCode = 0;
    unsigned inlinedActions = 0;
    UnusedCodeInstances unusedCodeInstances;
    UnusedCodeHelperVars helperVars;
    NestingDepthMetrics nestingDepth;
    HalsteadMetrics halsteadMetrics;
    HeaderMetrics headerMetrics;
    HeaderPacketMetrics headerManipulationMetrics; // Header addition and removal operations.
    HeaderPacketMetrics headerModificationMetrics; // Assignment operations.
    MatchActionTableMetrics matchActionTableMetrics;
    ParserMetrics parserMetrics;
    P4::ordered_map<std::string, unsigned> cyclomaticComplexity; // Function name -> CC value.
    ExternMetrics externMetrics;
};

} // namespace P4
#endif // METRICS_STRUCTURE_H_